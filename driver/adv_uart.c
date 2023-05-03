#include <linux/module.h>
#include <linux/tty.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/tty_flip.h>
#include <linux/serial_reg.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/version.h>

#include <asm/bitops.h>
#include <asm/byteorder.h>
#include <asm/serial.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include "advvcom.h"
#include "adv_uart.h"

LIST_HEAD(uart_list);

void adv_uart_xmit(struct uart_port *);

void adv_uart_update_xmit(struct uart_port *port)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct ring_buf * tx = up->tx;
	struct adv_port_att * attr = up->attr;
	struct adv_port_info * info = &attr->_attr;
	int is_open;
	
	spin_lock(&port->lock);

	spin_lock(&(attr->lock));
	is_open = info->is_open;
	spin_unlock(&(attr->lock));

	if(is_open){
		spin_lock(&(tx->lock));
		if(is_rb_empty(*tx)){
			adv_uart_xmit(port);
		}
		spin_unlock(&(tx->lock));

		uart_write_wakeup(port);
	}

	spin_unlock(&port->lock);
}


unsigned int adv_uart_ms(struct uart_port *port, unsigned int status)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct adv_port_att * attr = up->attr;
	unsigned int ms;
	
	spin_lock(&port->lock);

	spin_lock(&(attr->lock));
	ms = attr->mctrl;
	attr->mctrl = status;
	spin_unlock(&(attr->lock));
	
	if((status ^ ms) & ADV_MS_CTS){
		//uart_handle_cts_change(port, status);
		port->icount.cts++;
	}

	if((status ^ ms) & ADV_MS_DSR){
		port->icount.dsr++;
	}
	
	if((status ^ ms) & ADV_MS_CAR){
		//uart_handle_dcd_change(port, status & ADV_MS_CAR);
		port->icount.dcd++;
	}

	if((status ^ ms) & ADV_MS_RI){
		port->icount.rng++;
	}

	wake_up_interruptible(&port->state->port.delta_msr_wait);

	spin_unlock(&port->lock);

	return status;
	
}

static void adv_uart_stop_tx(struct uart_port *port)
{
	struct adv_uart_port * adv_port;
	struct ring_buf * tx;

	adv_port = (struct adv_uart_port *)port;
	tx = adv_port->tx;

	spin_lock(&(tx->lock));
	tx->status &= ~(ADV_RING_BUF_ENABLED);
	spin_unlock(&(tx->lock));
	//dump_stack();
}

static void adv_uart_start_tx(struct uart_port *port)
{
	struct adv_uart_port * adv_port;
	struct ring_buf * tx;

	adv_port = (struct adv_uart_port *)port;

	tx = adv_port->tx;

	spin_lock(&(tx->lock));
	tx->status |= ADV_RING_BUF_ENABLED;
	adv_uart_xmit(port);
	spin_unlock(&(tx->lock));
	
	if(waitqueue_active(&tx->wait)){
		wake_up_interruptible(&tx->wait);
	}

}

static void adv_uart_stop_rx(struct uart_port *port)
{
	struct adv_uart_port * adv_port;
	struct ring_buf * rx;

	adv_port = (struct adv_uart_port *)port;
	rx = adv_port->rx;
	
	spin_lock(&(rx->lock));
	rx->status &= ~(ADV_RING_BUF_ENABLED);
	spin_unlock(&(rx->lock));
	//dump_stack();
}

static void adv_uart_enable_ms(struct uart_port *port)
{
//	printk("%s(%d)\n", __func__, __LINE__);
}

void adv_uart_recv_chars(struct uart_port *port)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct adv_port_att * attr = up->attr;
	struct ring_buf * rx = up->rx;
	struct tty_port *tty = &(port->state->port);
	int max_retry;
	int throttled;
	int count;
	int ret;
	int rx_l, rx_ll, rx_hd;
	char * rx_base = (char *)rx->data;

	spin_lock(&(attr->lock));
	throttled = attr->throttled;
	spin_unlock(&(attr->lock));
	if(throttled){
		return;
	}


	spin_lock(&rx->lock);
		
	if((rx->status & ADV_RING_BUF_ENABLED) == 0){
		spin_unlock(&rx->lock);
		return;
	}
	
	rx_l = get_rb_length(*rx);


	count = rx_l;
	max_retry = 3;


	while(count > 0 && max_retry > 0){
		rx_ll = get_rb_llength(*rx);
		rx_hd = get_rb_head(*rx);

		ret = tty_insert_flip_string(tty, &rx_base[rx_hd], rx_ll);

		if(ret > 0){
			count -= ret;
			move_rb_head(rx, ret);
		}else{
			break;
		}
		max_retry--;
	}

	spin_unlock(&rx->lock);

	if(count != rx_l){
		tty_flip_buffer_push(tty);
		if(waitqueue_active(&rx->wait)){
			wake_up_interruptible(&rx->wait);
		}
	}
}

void adv_uart_xmit(struct uart_port *port)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct circ_buf *xmit = &up->port.state->xmit;
	struct ring_buf *tx = up->tx;
	char * tx_base = up->tx->data;
	int tx_size, tx_lsize, count;
	int mcplen;
	int tx_tail;
	int max_retry;
	
	if(uart_circ_empty(xmit)){
		return;	
	}
	if(get_rb_room(*tx) == 0){
		return;
	}

	count = uart_circ_chars_pending(xmit);
	tx_size = get_rb_room(*tx);
	tx_lsize = get_rb_lroom(*tx);
	
	count = (count < tx_size)?count:tx_size;

	max_retry = 3;

	do{
		tx_tail = get_rb_tail(*tx);

		if(count > tx_lsize){
			mcplen = tx_lsize;
		}else{
			mcplen = count;
		}

		if(((xmit->tail + mcplen) & (UART_XMIT_SIZE - 1)) < xmit->tail){
			mcplen = UART_XMIT_SIZE - xmit->tail;
		}

		memcpy(&tx_base[tx_tail], &(xmit->buf[xmit->tail]), mcplen);

		count -= mcplen;
		move_rb_tail(tx, mcplen);
		xmit->tail += mcplen;
		xmit->tail &= (UART_XMIT_SIZE - 1);
		tx_lsize = get_rb_lroom(*tx);

		max_retry--;

	}while(count > 0 && max_retry > 0);

}


static unsigned int adv_uart_tx_empty(struct uart_port *port)
{
	struct adv_uart_port * adv_port;
	struct ring_buf * tx;
	int empty;

	adv_port = (struct adv_uart_port *)port;
	tx = adv_port->tx;

	spin_lock(&(tx->lock));
	empty = is_rb_empty(*tx);
	spin_unlock(&(tx->lock));

	return empty ? TIOCSER_TEMT : 0;
}

static unsigned int adv_uart_get_mctrl(struct uart_port *port)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct adv_port_att * attr = up->attr;
	unsigned int status;

	status = 0;
	spin_lock(&attr->lock);
	if(attr->mctrl & ADV_MS_CTS){
		status |= TIOCM_CTS;
	}
	if(attr->mctrl & ADV_MS_DSR){
		status |= TIOCM_DSR;
	}
	if(attr->mctrl & ADV_MS_CAR){
		status |= TIOCM_CAR;
	}
	if(attr->mctrl & ADV_MS_RI){
		status |= TIOCM_RI;
	}
	spin_unlock(&attr->lock);

	return status;
}

static void adv_uart_set_mctrl(struct uart_port *port, unsigned int mctrl)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct adv_port_att * attr = up->attr;
	struct adv_port_info * info = &attr->_attr;
	unsigned int status;

	status = 0;
	
	if (mctrl & TIOCM_RTS)
		status |= ADV_MS_RTS;
	if (mctrl & TIOCM_DTR)
		status |= ADV_MS_DTR;

	spin_lock(&attr->lock);
	info->ms = status;	
	spin_unlock(&attr->lock);

	if(waitqueue_active(&attr->wait)){
		wake_up_interruptible(&attr->wait);
	}
}

static void adv_uart_break_ctl(struct uart_port *port, int break_state)
{
	printk("%s(%d)\n", __func__, __LINE__);

}
int adv_uart_startup(struct uart_port *port)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct adv_port_att * attr = up->attr;
	struct adv_port_info * info = &attr->_attr;
	struct ring_buf * rx;
	struct ring_buf * tx;


	rx = up->rx;
	tx = up->tx;

	spin_lock(&attr->lock);
	info->is_open = 1;	
	attr->throttled = 0;
	spin_unlock(&attr->lock);

	spin_lock(&rx->lock);
	rx->status |= ADV_RING_BUF_ENABLED;
	spin_unlock(&rx->lock);
	
	spin_lock(&tx->lock);
	tx->status |= ADV_RING_BUF_ENABLED;
	spin_unlock(&tx->lock);

	if(waitqueue_active(&attr->wait)){
		wake_up_interruptible(&attr->wait);
	}
	if(waitqueue_active(&rx->wait)){
		wake_up_interruptible(&rx->wait);
	}
	if(waitqueue_active(&tx->wait)){
		wake_up_interruptible(&tx->wait);
	}
	//adv_uart_recv_chars(port);

	return 0;
}

static void adv_uart_shutdown(struct uart_port *port)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct adv_port_att * attr = up->attr;
	struct adv_port_info * info = &attr->_attr;

	spin_lock(&attr->lock);
	info->is_open = 0;	
	spin_unlock(&attr->lock);

	if(waitqueue_active(&attr->wait)){
		wake_up_interruptible(&attr->wait);
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)
static void
adv_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       const struct ktermios *old)
{
	struct adv_uart_port * up = (struct adv_uart_port *)port;
	struct adv_port_att * adv_attr = up->attr;
	struct adv_port_info * attr = &adv_attr->_attr;

	
	spin_lock(&adv_attr->lock);

	attr->baud = uart_get_baud_rate(port, termios, old, 50, 921600);//port->uartclk);
//	uart_update_timeout();

	switch (termios->c_cflag & CSIZE) {
		case CS5:
			attr->byte = 5;
			break;
		case CS6:
			attr->byte = 6;
			break;
		case CS7:
			attr->byte = 7;
			break;
		default:
		case CS8:
			attr->byte = 8;
			break;
	}

	//flow control
	if(termios->c_cflag & CRTSCTS){
		attr->flowctl = ADV_FLOW_RTSCTS;
		port->status |= UPSTAT_AUTOCTS;
		port->status |= UPSTAT_AUTORTS;
		port->status &= ~UPSTAT_AUTOXOFF;
	}else if(termios->c_iflag & IXOFF){
		attr->flowctl = ADV_FLOW_XONXOFF;
		port->status |= UPSTAT_AUTOXOFF;
		port->status &= ~UPSTAT_AUTOCTS;
		port->status &= ~UPSTAT_AUTORTS;
	}else{
		attr->flowctl = ADV_FLOW_NONE;
		port->status &= ~UPSTAT_AUTOCTS;
		port->status &= ~UPSTAT_AUTORTS;
		port->status &= ~UPSTAT_AUTOXOFF;
	}
	//pairity
	switch(termios->c_cflag & (PARODD|CMSPAR|PARENB)){
		case PARENB:
			attr->pair = ADV_PAIR_EVEN;
			break;
		case (PARODD|PARENB):
			attr->pair = ADV_PAIR_ODD;
			break;
		case (CMSPAR|PARENB):
			attr->pair = ADV_PAIR_SPACE;
			break;
		case (PARODD|CMSPAR|PARENB):
			attr->pair = ADV_PAIR_MARK;
			break;
		default:
			attr->pair = ADV_PAIR_NONE;
			break;
	} 
	//stop bit
	if(termios->c_cflag & CSTOPB){
		attr->stop = ADV_STOP_2;
	}else{
		attr->stop = ADV_STOP_1;
	}
	
	spin_unlock(&adv_attr->lock);

	uart_update_timeout(port, termios->c_cflag, attr->baud);

	if(waitqueue_active(&adv_attr->wait)){
		wake_up_interruptible(&adv_attr->wait);
	}
}
#else
#include "./legacy/uart/adv_uart_set_termios.h"
#endif


static int
adv_uart_ioctl(struct uart_port *port, unsigned int cmd, unsigned long arg)
{
//	printk("cmd %x arg %x\n", cmd, arg);
	return -ENOIOCTLCMD;
}
	      

static void adv_uart_release_port(struct uart_port *port)
{
//	printk("%s(%d)\n", __func__, __LINE__);	
}

static int adv_uart_request_port(struct uart_port *port)
{
//	printk("%s(%d)\n", __func__, __LINE__);
	return 0;
}

static void adv_uart_config_port(struct uart_port *port, int flags)
{
//	printk("%s(%d)\n", __func__, __LINE__);
}

static const char *
adv_uart_type(struct uart_port *port)
{
	return "Advantech VCOM";
}

void adv_uart_throttle(struct uart_port *port)
{
	struct adv_uart_port * adv_port;
	struct adv_port_att * attr;

	adv_port = (struct adv_uart_port *)port;
	attr = adv_port->attr;

	spin_lock(&(attr->lock));
	attr->throttled = 1;
	spin_unlock(&(attr->lock));

	if(waitqueue_active(&attr->wait)){
		wake_up_interruptible(&attr->wait);
	}
}

void adv_uart_unthrottle(struct uart_port *port)
{
	struct adv_uart_port * adv_port;
	struct adv_port_att * attr;

	adv_port = (struct adv_uart_port *)port;
	attr = adv_port->attr;

	spin_lock(&(attr->lock));
	attr->throttled = 0;
	spin_unlock(&(attr->lock));
	
	adv_uart_recv_chars(port);
}

static struct uart_ops adv_uart_ops = {
	.tx_empty	= adv_uart_tx_empty,
	.set_mctrl	= adv_uart_set_mctrl,
	.get_mctrl	= adv_uart_get_mctrl,
	.stop_tx	= adv_uart_stop_tx,
	.start_tx	= adv_uart_start_tx,
	.stop_rx	= adv_uart_stop_rx,
	.enable_ms	= adv_uart_enable_ms,
	.break_ctl	= adv_uart_break_ctl,
	.startup	= adv_uart_startup,
	.shutdown	= adv_uart_shutdown,
	.set_termios	= adv_uart_set_termios,
//	.pm		= adv_uart_pm,
	.type		= adv_uart_type,
	.release_port	= adv_uart_release_port,
	.request_port	= adv_uart_request_port,
	.config_port	= adv_uart_config_port,
	.ioctl		= adv_uart_ioctl,
	.throttle	= adv_uart_throttle,
	.unthrottle	= adv_uart_unthrottle,
};

static DEFINE_MUTEX(serial_mutex);

//struct adv_uart_port adv_serial_ports[1];

#define VCOM_MAJOR 38
#define VCOM_MINOR 0
extern int vcom_port_num;

static struct uart_driver adv_uart_driver = {
	.owner			= THIS_MODULE,
	.driver_name		= "advvcom",
	.dev_name		= "ttyADV",
	.major			= VCOM_MAJOR,
	.minor			= VCOM_MINOR,
	.nr			= VCOM_PORTS,
	.cons			= NULL,
};

int adv_uart_register(void)
{
	int ret;

	ret = uart_register_driver(&adv_uart_driver);
//	printk("uart_list =%x\n", &uart_list);

	if (ret < 0){
		printk("ret < 0\n");
		uart_unregister_driver(&adv_uart_driver);
	}

	return ret;
}

int adv_uart_init(struct adv_vcom * vcomdata, int index)
{
	int ret;
	struct adv_uart_port * adv_serial_port;

//	printk("%s(%d)\n", __func__, __LINE__);
//	adv_serial_port = &adv_serial_ports[index];
	adv_serial_port = kmalloc(sizeof(struct adv_uart_port), GFP_KERNEL);

	memset(adv_serial_port, 0, sizeof(struct adv_uart_port));

	spin_lock_init(&adv_serial_port->port.lock);

	adv_serial_port->port.flags = UPF_SKIP_TEST|UPF_HARD_FLOW|UPF_SOFT_FLOW;
	adv_serial_port->port.type = PORT_16850;
	adv_serial_port->port.ops = &adv_uart_ops;
	adv_serial_port->port.line = index;
	adv_serial_port->port.fifosize = 2048;
//	these values are set in set_termios, don't need to set here
//	otherwise, we would need to make adv_uart_ops global, which we really don't want.
//	adv_serial_port->port.status |= (UPSTAT_AUTOCTS|UPSTAT_AUTORTS|UPSTAT_AUTOXOFF);
	
	ret = uart_add_one_port(&adv_uart_driver, &adv_serial_port->port);

	vcomdata->adv_uart = (struct uart_port *)adv_serial_port;
	adv_serial_port->attr = &vcomdata->attr;
	adv_serial_port->tx = &vcomdata->rx;
	adv_serial_port->rx = &vcomdata->tx;

	INIT_LIST_HEAD(&adv_serial_port->list);
	list_add(&adv_serial_port->list, &uart_list);

	return ret;	
}

int adv_uart_rm_port(int index)
{
	struct adv_uart_port * adv_serial_port;

	list_for_each_entry(adv_serial_port, &uart_list, list){
		if(adv_serial_port->port.line == index){
//			printk("found uart %d at addr 0x%x\n", index, adv_serial_port);
			break;
		}
	}

	if(adv_serial_port->port.line != index){
		printk("uart %d not found\n", index);
		return 0;
	}

	uart_remove_one_port(&adv_uart_driver, &adv_serial_port->port);

	return 0;
}

int adv_uart_release(void)
{
	uart_unregister_driver(&adv_uart_driver);

	return 0;
}
