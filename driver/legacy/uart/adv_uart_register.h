#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,5,0)
// @ current
#else
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
#endif
