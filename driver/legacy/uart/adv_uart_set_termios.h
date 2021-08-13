#ifndef _ADV_UART_SET_TERMIOS_H
#define _ADV_UART_SET_TERMIOS_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
//@ current
#else
static void
adv_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)

#endif //_ADV_UART_SET_TERMIOS_H
