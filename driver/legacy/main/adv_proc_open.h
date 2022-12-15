#ifndef ADV_PROC_OPEN_H
#define ADV_PROC_OPEN_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0)

#else
int adv_proc_open(struct inode *inode, struct file *filp)
{
	struct adv_vcom * data;
#if defined(RHEL_RELEASE_CODE)
#	if RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9,1)
	data = pde_data(inode);
#	else
	data = PDE_DATA(inode);
#	endif
#else
	data = PDE_DATA(inode);
#endif

	filp->private_data = data;
	return 0;
}

int adv_proc_release(struct inode *inode, struct file *filp)
{
	struct adv_vcom * data;

#if defined(RHEL_RELEASE_CODE)
#	if RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9,1)
	data = pde_data(inode);
#	else
	data = PDE_DATA(inode);
#	endif
#else
	data = PDE_DATA(inode);
#endif

	return 0;
}
#endif
#endif
