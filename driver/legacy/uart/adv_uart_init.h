#ifndef _ADV_UART_INIT_H
#define _ADV_UART_INIT_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,5,0)
//@ current
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
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
#else
int adv_uart_init(struct adv_vcom * vcomdata, int index);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)

#endif //_ADV_UART_INIT_H
