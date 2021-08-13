#ifndef _ADV_UART_H
#define _ADV_UART_H
struct adv_uart_port {
	struct uart_port	port;
	struct adv_port_att *	attr;
	struct ring_buf	*	rx;
	struct ring_buf *	tx;
	struct list_head	list;
};
#endif
