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
#include "../../advioctl.h"
#include "../../advvcom.h"


extern void adv_uart_update_xmit(struct uart_port *);
extern void adv_uart_recv_chars(struct uart_port *);
extern void adv_main_interrupt(struct adv_vcom *, int);
extern void adv_main_clear(struct adv_vcom * data, int mask);
extern unsigned int adv_uart_ms(struct uart_port *, unsigned int);


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
//@ current
#else
long adv_proc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct adv_vcom * data;
	int err = 0;
	int ret = -EFAULT;
	int tmp;


	if(_IOC_TYPE(cmd) != ADVVCOM_IOC_MAGIC){	
		printk("%s(%d) cmd = %x\n", __func__, __LINE__, cmd);
		return -ENOTTY;
	}
	if(_IOC_NR(cmd) > ADVVCOM_IOCMAX){
		printk("%s(%d) cmd = %x\n", __func__, __LINE__, cmd);
		return -ENOTTY;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
#	define __ACCESS_OK_5_0_0
#elif defined(RHEL_RELEASE_CODE)
#	if RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8,1)
#		define __ACCESS_OK_5_0_0
#	endif
#endif

	if (_IOC_DIR(cmd) & _IOC_READ){
#ifdef	__ACCESS_OK_5_0_0
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
#else
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
#endif
	}else if (_IOC_DIR(cmd) & _IOC_WRITE){
#ifdef	__ACCESS_OK_5_0_0
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
#else
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
#endif
	}
	if (err)
		return -EFAULT;

	data = filp->private_data;

	switch(cmd){

	case ADVVCOM_IOCGTXHEAD:
		
//		adv_uart_recv_chars(data->adv_uart);

		spin_lock((&data->tx.lock));
		tmp = get_rb_head(data->tx);
		ret = __put_user(tmp, (int __user *)arg);
		spin_unlock(&(data->tx.lock));
		break;

        case ADVVCOM_IOCGTXTAIL:
		spin_lock(&(data->tx.lock));
		tmp = get_rb_tail(data->tx);
		ret = __put_user(tmp, (int __user *)arg);
		spin_unlock(&(data->tx.lock));
		break;

	case ADVVCOM_IOCGTXSIZE:
		ret = __put_user(data->tx.size, (int __user *)arg);
		break;

	case ADVVCOM_IOCGTXBEGIN:
		ret = __put_user(data->tx.begin, (int __user *)arg);
		break;

        case ADVVCOM_IOCGRXHEAD:
//		adv_uart_update_xmit(data->adv_uart);
		spin_lock(&(data->rx.lock));
		tmp = get_rb_head(data->rx);
		ret = __put_user(tmp, (int __user *)arg);
		spin_unlock(&(data->rx.lock));
		break;

	case ADVVCOM_IOCGRXTAIL:
		spin_lock(&(data->rx.lock));
		tmp = get_rb_tail(data->rx);
		ret = __put_user(tmp, (int __user *)arg);
		spin_unlock(&(data->rx.lock));
		break;

        case ADVVCOM_IOCGRXSIZE:
		ret = __put_user(data->rx.size, (int __user *)arg);
		break;

	case ADVVCOM_IOCGRXBEGIN:
		ret = __put_user(data->rx.begin, (int __user *)arg);
		break;

        case ADVVCOM_IOCSTXTAIL:
		spin_lock(&(data->tx.lock));
		ret = __get_user(tmp, (int __user *)arg);
		move_rb_tail(&data->tx, tmp);
		spin_unlock(&(data->tx.lock));

		adv_uart_recv_chars(data->adv_uart);
		break;

        case ADVVCOM_IOCSRXHEAD:
		spin_lock(&(data->rx.lock));
		ret = __get_user(tmp, (int __user *)arg);
		move_rb_head(&data->rx, tmp);
		spin_unlock(&(data->rx.lock));
		break;

	case ADVVCOM_IOCGATTRBEGIN:
		ret = __put_user(data->attr.begin, (int __user *)arg);
		break;

	case ADVVCOM_IOCGATTRPTR:
		spin_lock(&(data->attr.lock));
		tmp = flush_attr_info(&(data->attr));
		ret = __put_user(tmp, (int __user *)arg);
		spin_unlock(&(data->attr.lock));
		break;

	case ADVVCOM_IOCSINTER:
		ret = __get_user(tmp, (int __user *)arg);
		adv_main_interrupt(data, tmp);
		break;

	case ADVVCOM_IOCSMCTRL:
		ret = __get_user(tmp, (int __user *)arg);
		adv_uart_ms(data->adv_uart, (unsigned int)tmp);
		break;

	case ADVVCOM_IOCSCLR:
		ret = __get_user(tmp, (int __user *)arg);
		adv_main_clear(data, tmp);
		break;
	}

	return (long)ret;
}
#endif
