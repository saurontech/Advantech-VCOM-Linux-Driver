#ifndef _ADV_UART_RELEASE_H
#define _ADV_UART_RELEASE_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,5,0)
//@ current
#else
int adv_uart_release(void)
{
	uart_unregister_driver(&adv_uart_driver);

	return 0;
}
#endif
#endif
