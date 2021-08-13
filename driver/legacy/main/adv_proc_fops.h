#ifndef _ADV_PROC_FOPS_H
#define _ADV_PROC_FOPS_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
//@ current
#else
static const struct file_operations adv_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= adv_proc_open,
	.release	= adv_proc_release,
	.mmap		= adv_proc_mmap,
	.unlocked_ioctl	= adv_proc_ioctl,
	.poll		= adv_proc_poll,
};
#endif

#endif
