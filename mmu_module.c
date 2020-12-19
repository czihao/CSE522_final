#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <asm/pgtable.h>
#include <asm/pgtable-2level.h>
#include "paging.h"
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/gfp.h>
#include <linux/memory.h>
#include <linux/mm_types.h>
#include <linux/sched.h>


// Fault variables
static unsigned int num_fault = 0U;
static unsigned int fault_addr = 0U; 


#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
static int
paging_vma_fault(struct vm_area_struct * vma,
                 struct vm_fault       * vmf)
{
    unsigned long fault_address = (unsigned long)vmf->virtual_address
#else
static unsigned int // I need to change to unsigned to compile
paging_vma_fault(struct vm_fault * vmf)

{
    struct vm_area_struct * vma = vmf->vma;
    unsigned long fault_address = (unsigned long)vmf->address;
#endif
	struct page *page = NULL;
	unsigned long offset;
	u32 pgd_addr, pgd_val, pte_addr, new_pte_val;
	
	//map physical page to vma stored in vm_private_data
	offset = (unsigned long)(vmf->pgoff << PAGE_SHIFT);
	page = virt_to_page(vmf->vma->vm_private_data + offset);
	vmf->page = page;
	get_page(page);
	//calculate total page for vma
	u32 total_pages = (vmf->vma->vm_end - vmf->vma->vm_start) / 0x1000;
	u32 num_page = 0U;
	
	if (num_fault == 1) {
		//get first fault's address
		asm("mrc p15, 0, %0, c6, c0, 0" : "=r" (fault_addr) ::);
	} else if (num_fault >= 2) {
		// get the first fault's values
		asm("mrc p15, 0, %0, c2, c0, 0" : "=r" (pgd_addr) :: );
		pgd_addr &= ~(0x3FFFU);
		pgd_addr |= (u32)(((fault_addr >> 20) & 0xFFFU) << 2);
		pgd_addr = phys_to_virt(pgd_addr);
		
		pgd_val = *(u32*)pgd_addr;
		pte_addr = pgd_val;
		pte_addr &= ~(0x3FFU);
		pte_addr |= (((fault_addr >> 12) & 0xFFU) << 2);
		pte_addr = phys_to_virt(pte_addr);
		fault_addr = *(u32*)pte_addr;
		
		// get the page table addresses associated with the vma
		asm("mrc p15, 0, %0, c2, c0, 0" : "=r" (pgd_addr) :: );
		pgd_addr &= ~(0x3FFFU);
		pgd_addr |= (u32)((((u32)vmf->vma->vm_start >> 20) & 0xFFFU) << 2);
		pgd_addr = phys_to_virt(pgd_addr);

		pgd_val = *(u32*)pgd_addr;
		pte_addr = pgd_val;
		pte_addr &= ~(0x3FFU);
		pte_addr |= ((((u32)vmf->vma->vm_start >> 12) & 0xFFU) << 2);
		pte_addr = phys_to_virt(pte_addr);

		// update the pte content to point to larger memory region
		new_pte_val = fault_addr;
		u32 tex = (new_pte_val >> 6) & 3U;               
		u32 xn  = new_pte_val & 1U;                      
		new_pte_val &= 0xFFFU;                   		  
		new_pte_val &= ~(3U);                            
		new_pte_val |= virt_to_phys(vmf->vma->vm_private_data) | 1U | (tex << 12) | (xn << 15);    
		u32* large_pte = (u32*)pte_addr;
		*large_pte = new_pte_val;

		while (num_page < total_pages) {
			//match base address to new physical page
			*large_pte = new_pte_val + ((num_page/16U) << 16); 
			//flush newly updated vma
			asm("mcr p15, 0, %0, c7, c10, 1" :: "r"(large_pte) :);	 
			large_pte++;
			num_page++;
		}
	}
	num_fault++;

	return 0;
}

void paging_vma_open(struct vm_area_struct* vma)
{
	printk(KERN_INFO "paging_vma_open() invoked\n");
}

void paging_vma_close(struct vm_area_struct* vma)
{
	printk(KERN_INFO "paging_vma_close() invoked\n");
}

static struct vm_operations_struct paging_vm_ops = {
	.fault = paging_vma_fault,
	.open  = paging_vma_open,
	.close = paging_vma_close
};

//customized mmap
static int paging_mmap(struct file* filp, struct vm_area_struct *vma)
{
	printk(KERN_INFO "Driver mmap\n");
	vma->vm_private_data = filp->private_data;
	vma->vm_ops = &paging_vm_ops;
	return 0;
}

static int paging_release(struct inode* inodep, struct file* filep)
{
	printk(KERN_INFO "Driver released\n");
	kfree(filep->private_data);
	return 0;
}
//Default allocation, re-map upon fault
static int paging_open(struct inode* inodep, struct file* filep)
{
	printk(KERN_INFO "Driver opened\n");
	filep->private_data = kmalloc(0x100000, GFP_KERNEL);
	if (!filep->private_data)
		return -1;
	return 0;
}

static const struct file_operations 
dev_ops = 
{
	.mmap  = paging_mmap,
	.open  = paging_open,
	.release = paging_release,
};

static struct miscdevice
dev_handle =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = PAGING_MODULE_NAME,
    .fops = &dev_ops,
};
 
/*** Kernel module initialization and teardown ***/
static int
kmod_paging_init(void)
{
    int status;

    /* Create a character device to communicate with user-space via file I/O operations */
    status = misc_register(&dev_handle);
    if (status != 0) {
        printk(KERN_ERR "Failed to register misc. device for module\n");
        return status;
    }

    printk(KERN_INFO "Loaded kmod_paging module\n");

    return 0;
}

static void
kmod_paging_exit(void)
{
    /* Deregister our device file */
    misc_deregister(&dev_handle);

    printk(KERN_INFO "Unloaded kmod_paging module\n");
}

module_init(kmod_paging_init);
module_exit(kmod_paging_exit);

MODULE_LICENSE ("GPL");

