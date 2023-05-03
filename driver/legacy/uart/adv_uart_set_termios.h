#ifndef _ADV_UART_SET_TERMIOS_H
#define _ADV_UART_SET_TERMIOS_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)
//@ current
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
void adv_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old);
#else
void adv_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)

#endif //_ADV_UART_SET_TERMIOS_H
