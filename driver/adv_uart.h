#ifndef _ADV_UART_H
#define _ADV_UART_H
struct adv_uart_port {
	struct uart_port	port;
	struct adv_port_att *	attr;
	struct ring_buf	*	rx;
	struct ring_buf *	tx;
	struct list_head	list;
};

int adv_uart_init(struct adv_vcom *, int );
int adv_uart_register(void);
int adv_uart_release(void);
int adv_uart_rm_port(int);
void adv_uart_update_xmit(struct uart_port *);
void adv_uart_recv_chars(struct uart_port *);
unsigned int adv_uart_ms(struct uart_port *, unsigned int);

#endif
