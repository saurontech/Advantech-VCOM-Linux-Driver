#ifndef _LEGACY_MMAP_OPS_FAULT_H
#define _LEGACY_MMAP_OPS_FAULT_H
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0)
//@ current
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
int adv_vma_nopage(struct vm_fault *vmf);
#else
int adv_vma_nopage(struct vm_area_struct *vma, struct vm_fault *vmf);
#endif //LINUX_VERSION_CODE
#endif //_LEGACY_MMAP_OPS_FAULT_H
