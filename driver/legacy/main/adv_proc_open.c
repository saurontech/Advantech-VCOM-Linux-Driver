#include <linux/module.h>                           
#include <linux/moduleparam.h>                      
#include <linux/init.h>                             
#include <linux/kernel.h>       /* printk() */      
#include <linux/slab.h>         /* kmalloc() */     
#include <linux/fs.h>           /* everything... */ 
#include <linux/errno.h>        /* error codes */   
#include <linux/types.h>        /* size_t */        
#include <linux/proc_fs.h>                          
#include <linux/fcntl.h>        /* O_ACCMODE */     
#include <linux/aio.h>          
#include <linux/poll.h>      
#include <linux/moduleparam.h>  
#include <linux/init.h>         
#include <linux/kernel.h>       /* printk() */      
#include <linux/slab.h>         /* kmalloc() */     
#include <linux/fs.h>           /* everything... */ 
#include <linux/errno.h>        /* error codes */   
#include <linux/types.h>        /* size_t */        
#include <linux/proc_fs.h>                          
#include <linux/fcntl.h>        /* O_ACCMODE */ 
#include <linux/aio.h>                              
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/version.h>

#include "../../advvcom.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0)
//@ current
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
#endif
