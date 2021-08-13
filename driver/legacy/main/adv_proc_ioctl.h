#ifndef 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
//@ current
#elif defined(RHEL_RELEASE_CODE)
#	if RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,1)
//	@current
	#else
		long adv_proc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#	endif
#else//oldest
		long adv_proc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#endif
