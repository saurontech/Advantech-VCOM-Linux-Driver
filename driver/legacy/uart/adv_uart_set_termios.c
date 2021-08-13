#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
//@ current
#else
static void
adv_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old)
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
	}else if(termios->c_iflag & IXOFF){
		attr->flowctl = ADV_FLOW_XONXOFF;

	}else{
		attr->flowctl = ADV_FLOW_NONE;
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
#endif
