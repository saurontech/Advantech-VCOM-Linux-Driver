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
#include <linux/serial_core.h>
#include "advioctl.h"
#include "advvcom.h"
#include "adv_mmap.h"
#include "adv_uart.h"

//int _adv_portcount = 10;
//int _adv_pagecount = 1;

//extern int adv_uart_init(struct adv_vcom *, int );
//extern int adv_uart_register(void);
//extern int adv_uart_release(void);
//extern int adv_uart_rm_port(int);
//extern void adv_uart_update_xmit(struct uart_port *);
//extern void adv_uart_recv_chars(struct uart_port *);
void adv_main_interrupt(struct adv_vcom *, int);
void adv_main_clear(struct adv_vcom * data, int mask);
//extern unsigned int adv_uart_ms(struct uart_port *, unsigned int);

//module_param(_adv_portcount, int, S_IRUGO|S_IWUSR);
//module_param(_adv_pagecount, int, 1);

MODULE_LICENSE("GPL");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0)
static long adv_proc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
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

	if (_IOC_DIR(cmd) & _IOC_READ){
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	}else if (_IOC_DIR(cmd) & _IOC_WRITE){
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
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
#else
#include "./legacy/main/adv_proc_ioctl.h"
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0)
static int adv_proc_open(struct inode *inode, struct file *filp)
{
	struct adv_vcom * data;
	
	data = pde_data(inode);
	filp->private_data = data;

	return 0;
}
#else
#include "./legacy/main/adv_proc_open.h"
#endif

static int adv_proc_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static unsigned int adv_proc_poll(struct file *filp, poll_table *wait)
{
	struct adv_vcom * data;
	unsigned int mask;
	struct adv_port_info * attr_base;
	struct adv_port_info * attr_usr;

	data = filp->private_data;
	mask = 0;

	poll_wait(filp, &(data->tx.wait), wait);
	poll_wait(filp, &(data->rx.wait), wait);
	poll_wait(filp, &(data->attr.wait), wait);

	
	
	spin_lock(&(data->rx.lock));
	if(data->rx.status & ADV_RING_BUF_ENABLED){
		mask |= is_rb_empty(data->rx)?0x0:POLLIN;
	}
	spin_unlock(&(data->rx.lock));

	spin_lock(&(data->tx.lock));
	if(data->tx.status & ADV_RING_BUF_ENABLED){
		mask |= is_rb_empty(data->tx)?POLLOUT:0x0;
	//	mask |= POLLOUT;
	}
	spin_unlock(&(data->tx.lock));

	spin_lock(&(data->attr.lock));
	if(data->attr.throttled){
			mask &= (~POLLOUT);
	}
	attr_base = (struct adv_port_info *)data->attr.data;
	attr_usr = &attr_base[data->attr.usr_ptr];
	if(memcmp(&data->attr._attr, attr_usr, sizeof(struct adv_port_info))){
		if(data->attr._attr.is_open == 1){
			mask |= POLLPRI;
		}else if(data->attr._attr.is_open != attr_usr->is_open){
			mask |= POLLPRI;
		}
	}
	spin_unlock(&(data->attr.lock));

	return mask;
}

//extern int adv_proc_mmap(struct file *filp, struct vm_area_struct *vma);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
static const struct proc_ops adv_proc_fops = {
	.proc_open 	= adv_proc_open,
	.proc_release	= adv_proc_release,
	.proc_mmap	= adv_proc_mmap,
	.proc_ioctl	= adv_proc_ioctl,
	.proc_poll	= adv_proc_poll,
};
#else
#include "./legacy/main/adv_proc_fops.h"
#endif

void adv_main_interrupt(struct adv_vcom * data, int mask)
{
	if(mask & ADV_INT_RX){
		adv_uart_update_xmit(data->adv_uart);		
	}
	if(mask & ADV_INT_TX){
		adv_uart_recv_chars(data->adv_uart);
	}

}

void adv_main_clear(struct adv_vcom * data, int mask)
{
	if(mask & ADV_CLR_RX){
		spin_lock((&data->rx.lock));
		data->rx.status |= ADV_RING_BUF_CLEAN;
		spin_unlock((&data->rx.lock));
	}
	if(mask & ADV_CLR_TX){
		spin_lock((&data->tx.lock));
		data->tx.status |= ADV_RING_BUF_CLEAN;
		move_rb_head(&data->tx, 0);
		spin_unlock((&data->tx.lock));
	}

}



LIST_HEAD(vcom_list);
struct proc_dir_entry * proc_root = 0;

static struct adv_vcom * adv_main_init(int port)
{
	struct adv_vcom * vcomdata;
	char filename[128];

	sprintf(filename, "advproc%d", port);
	vcomdata = kmalloc(sizeof(struct adv_vcom), GFP_KERNEL);

	vcomdata->tx.head = 0;
	vcomdata->tx.tail = 0;
	vcomdata->tx.begin = 0;
	vcomdata->tx.size = PAGE_SIZE;
	vcomdata->tx.status = 0;
	vcomdata->tx.data = (void *)__get_free_pages(GFP_KERNEL, 0);

	vcomdata->rx.head = 0;
	vcomdata->rx.tail = 0;
	vcomdata->rx.begin = PAGE_SIZE;
	vcomdata->rx.size = PAGE_SIZE;
	vcomdata->rx.status = 0;
	vcomdata->rx.data = (void *)__get_free_pages(GFP_KERNEL, 0);

	
	vcomdata->attr.index = port;
	vcomdata->attr.begin = 2 * PAGE_SIZE;
	vcomdata->attr.size = PAGE_SIZE;
	vcomdata->attr.usr_ptr = 0;
	vcomdata->attr._newbie = 1;
	vcomdata->attr.throttled = 0;
	vcomdata->attr.data = (void *)__get_free_pages(GFP_KERNEL, 0);
	
	memset(&vcomdata->attr._attr, 0, sizeof(struct adv_port_info));

	memset(vcomdata->attr.data, 0, (ADV_ATTRBUF_NUM *sizeof(struct adv_port_info)));

	spin_lock_init(&(vcomdata->tx.lock));
	spin_lock_init(&(vcomdata->rx.lock));
	spin_lock_init(&(vcomdata->attr.lock));

	init_waitqueue_head(&(vcomdata->tx.wait));
	init_waitqueue_head(&(vcomdata->rx.wait));
	init_waitqueue_head(&(vcomdata->attr.wait));

	
	vcomdata->entry = proc_create_data(filename, 0777, proc_root, &adv_proc_fops, vcomdata);

	INIT_LIST_HEAD(&vcomdata->list);
	list_add(&vcomdata->list, &vcom_list);
	
	return vcomdata;
}

static int adv_main_release(int port)
{
	struct adv_vcom * vcomdata;
	
	list_for_each_entry(vcomdata, &vcom_list, list){
		if(vcomdata->attr.index == port){
			break;
		}
	}

	list_del(&vcomdata->list);

	if(vcomdata->attr.index != port){
		printk("port %d at not found\n", port);
		return 0;
	}
	
	proc_remove(vcomdata->entry);
	

	if(vcomdata->rx.data){
		free_pages((unsigned long)vcomdata->rx.data, 0);
	}

	if(vcomdata->tx.data){
		free_pages((unsigned long)vcomdata->tx.data, 0);
	}

	if(vcomdata->attr.data){
		free_pages((unsigned long)vcomdata->attr.data, 0);
	}

	kfree(vcomdata);

	return 0;
}

static int adv_vcom_init(void)
{
	struct adv_vcom * data;
	int i;

	proc_root = proc_mkdir_mode("vcom", 0777, 0);
	adv_uart_register();
	
	for(i = 0; i < VCOM_PORTS; i++){
		data = adv_main_init(i);
		adv_uart_init(data, i);
	}
	
	return 0;
}

static void adv_vcom_exit(void)
{
	int i;

	for(i = 0; i < VCOM_PORTS; i++){
		adv_uart_rm_port(i);
		adv_main_release(i);
	}

	adv_uart_release();
	proc_remove(proc_root);
}

module_init(adv_vcom_init);
module_exit(adv_vcom_exit);
