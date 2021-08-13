#ifndef _ADV_PROC_ICOTL_H
#define _ADV_PROC_IOCTL_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
//@ current
#else//oldest
long adv_proc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#endif
#endif
