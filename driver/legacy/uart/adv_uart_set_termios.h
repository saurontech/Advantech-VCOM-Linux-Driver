#ifndef _ADV_UART_SET_TERMIOS_H
#define _ADV_UART_SET_TERMIOS_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)
//@ current
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
#	if defined(RHEL_RELEASE_CODE)
#		if RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9,2)
#			define _BACK_PORT_6_1_0
#		endif
#	endif
#	ifdef _BACK_PORT_6_1_0
void adv_uart_set_termios(struct uart_port *port, struct ktermios *termios,
				const struct ktermios *old);
#	else
void adv_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old);
#	endif
#	undef _BACK_PORT_6_1_0
#else
void adv_uart_set_termios(struct uart_port *port, struct ktermios *termios,
		       struct ktermios *old);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)

#endif //_ADV_UART_SET_TERMIOS_H
