#include <linux/module.h>

#include <linux/mm.h>           /* everything */
#include <linux/errno.h>        /* error codes */
#include <linux/fs.h>
#include <asm/pgtable.h>
#include <linux/version.h>
#include "advvcom.h"
#include "adv_mmap.h"

static void adv_vma_open(struct vm_area_struct *vma)
{
//	printk("%s(%d)\n", __func__, __LINE__);
}

static void adv_vma_close(struct vm_area_struct *vma)
{
//	printk("%s(%d)\n", __func__, __LINE__);
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
static vm_fault_t adv_vma_nopage(struct vm_fault *vmf)
{
	unsigned long offset;
	struct adv_vcom * data;
	struct vm_area_struct *vma = vmf->vma;
	struct page *page = NULL;
//	void * pageptr = NULL;
	vm_fault_t ret = 0;

//	printk("%s(%d)\n", __func__, __LINE__);
	data = vma->vm_private_data;

//	printk("data = %x\n", data);
	offset = (unsigned long)(vmf->address - vma->vm_start) + (vma->vm_pgoff << PAGE_SHIFT);
//	printk("offset = %u \n", offset);
//	printk("totalsize = %d\n", (data->rx.size + data->tx.size));


//	remove this code not because of bug,
//	but because it is redundent to the following code,
//	and it is also confusiong
/*	if(offset > (data->rx.size + data->tx.size) ){
		printk("%s(%d)\n", __func__, __LINE__);
		goto out;
	}*/
	
//	page = virt_to_page(data->data);
	if(offset < (data->tx.size + data->tx.begin)){
//		printk("mapping tx page\n");
		page = virt_to_page(data->tx.data);
	}else if(offset < (data->rx.size + data->rx.begin)){
//		printk("mapping rx page\n");
		page = virt_to_page(data->rx.data);
	}else if(offset < (data->attr.size + data->attr.begin)){
//		printk("mapping attr page\n");
		page = virt_to_page(data->attr.data);
	}else{
//		printk("%s(%d)\n", __func__, __LINE__);
		goto out;
	}

	get_page(page);
	vmf->page = page;
		
	out:
	return ret;
}
#else
#include "legacy/mmap/ops_fault.h"
#endif

struct vm_operations_struct adv_vm_ops = {
        .open =     adv_vma_open,
        .close =    adv_vma_close,
        .fault =    adv_vma_nopage,
};

int adv_proc_mmap(struct file *filp, struct vm_area_struct *vma)
{
//	printk("%s(%d)\n", __func__, __LINE__);

//	struct inode *inode = filp->f_dentry->d_inode;

	/* refuse to map if order is not 0 */
//	if (scullp_devices[iminor(inode)].order)
//		return -ENODEV;

	/* don't do anything here: "nopage" will set up page table entries */
	vma->vm_ops = &adv_vm_ops;
	vma->vm_private_data = filp->private_data;
//	adv_vma_open(vma);
	return 0;

}
