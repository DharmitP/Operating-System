#ifndef _VM_H_
#define _VM_H_

#include <machine/vm.h>
#include <kern/types.h>
#include <thread.h>

/*
 * VM system-related definitions.
 *
 * You'll probably want to add stuff here.
 */


/* Fault-type arguments to vm_fault() */
#define VM_FAULT_READ        0    /* A read was attempted */
#define VM_FAULT_WRITE       1    /* A write was attempted */
#define VM_FAULT_READONLY    2    /* A write to a readonly page was attempted*/


/* Initialization function */
void vm_bootstrap(void);

/* Fault handling function called by trap code */
int vm_fault(int faulttype, vaddr_t faultaddress);

/* Allocate/free kernel heap pages (called by kmalloc/kfree) */
vaddr_t alloc_kpages(int npages);
void free_kpages(vaddr_t addr);

struct coremap_entry{
    struct page_entry *v_page; //pointer to page entry in page table
    paddr_t physical_addr;
    int num_page_frames; //the # of page frames used (>1 if continguous)
    int starting_page; //1 = start, 0 = not start
    int valid;  //whether page frame is being used or not; 0 = not used, 1 = used
};

extern struct coremap_entry *coremap;
extern int num_page_frames;
extern paddr_t first_pageframe_addr;

paddr_t getppages(unsigned long npages);
int check_getppages(unsigned long npages); //checks if there are non-consecutive npages available in coremap
#endif /* _VM_H_ */
