#ifndef _ADV_UART_INIT_H
#define _ADV_UART_INIT_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)
//@ current
#else
int adv_uart_init(struct adv_vcom * vcomdata, int index);
#endif //LINUX_VERSION_CODE >= KERNEL_VERSION(4,0,0)

#endif //_ADV_UART_INIT_H
