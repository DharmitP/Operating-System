#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>
#include <thread.h>
#include <curthread.h>
#include <machine/spl.h>
#include <machine/tlb.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

//struct addrspace *
//as_create(void)
//{
//	struct addrspace *as = kmalloc(sizeof(struct addrspace));
//	if (as==NULL) {
//		return NULL;
//	}
//
//	/*
//	 * Initialize as needed.
//	 */
//
//	return as;
//}
//
//int
//as_copy(struct addrspace *old, struct addrspace **ret)
//{
//	struct addrspace *newas;
//
//	newas = as_create();
//	if (newas==NULL) {
//		return ENOMEM;
//	}
//
//	/*
//	 * Write this.
//	 */
//
//	(void)old;
//	
//	*ret = newas;
//	return 0;
//}
//
//void
//as_destroy(struct addrspace *as)
//{
//	/*
//	 * Clean up as needed.
//	 */
//	
//	kfree(as);
//}
//
//void
//as_activate(struct addrspace *as)
//{
//	/*
//	 * Write this.
//	 */
//
//	(void)as;  // suppress warning until code gets written
//}
//
///*
// * Set up a segment at virtual address VADDR of size MEMSIZE. The
// * segment in memory extends from VADDR up to (but not including)
// * VADDR+MEMSIZE.
// *
// * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
// * write, or execute permission should be set on the segment. At the
// * moment, these are ignored. When you write the VM system, you may
// * want to implement them.
// */
//int
//as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
//		 int readable, int writeable, int executable)
//{
//	/*
//	 * Write this.
//	 */
//
//	(void)as;
//	(void)vaddr;
//	(void)sz;
//	(void)readable;
//	(void)writeable;
//	(void)executable;
//	return EUNIMP;
//}
//
//int
//as_prepare_load(struct addrspace *as)
//{
//	/*
//	 * Write this.
//	 */
//
//	(void)as;
//	return 0;
//}
//
//int
//as_complete_load(struct addrspace *as)
//{
//	/*
//	 * Write this.
//	 */
//
//	(void)as;
//	return 0;
//}
//
//int
//as_define_stack(struct addrspace *as, vaddr_t *stackptr)
//{
//	/*
//	 * Write this.
//	 */
//
//	(void)as;
//
//	/* Initial user-level stack pointer */
//	*stackptr = USERSTACK;
//	
//	return 0;
//}


struct addrspace *
as_create(void) {
    struct addrspace *as = kmalloc(sizeof (struct addrspace));
    if (as == NULL) {
        kfree(as);
        return NULL;
    }

    as->as_vbase_text = 0;
    as->as_npages_text = 0;
    as->as_vbase_data = 0;
    as->as_npages_data = 0;
    as->as_vbase_stack = 0;
    as->as_npages_stack = 0;
    as->as_vbase_heap = 0;
    as->as_vtop_heap = 0;
    as->num_pt_entries = 0;
      
    //create a new page table with double the size of RAM and initialize all entries
    as->pagetable = kmalloc(sizeof (struct page_entry) * num_page_frames * 2);
    if(as->pagetable == NULL){
        kfree(as);
        kfree(as->pagetable);
        return NULL;
    }
    int i;
    for (i = 0; i < num_page_frames * 2; i++) {
        as->pagetable[i].present = 0;
        as->pagetable[i].virt_frame_num = -1; 
        as->pagetable[i].page_frame_num = -1; //initially not valid page frame number
        as->pagetable[i].in_valid_region = 0;
    }

    return as;
}

void
as_destroy(struct addrspace *as) {
    int i;
    for(i = 0; i < num_page_frames * 2; i++){
        if(as->pagetable[i].present == 1) {
            coremap[as->pagetable[i].page_frame_num].v_page = NULL;
            coremap[as->pagetable[i].page_frame_num].starting_page = 0;
            coremap[as->pagetable[i].page_frame_num].num_page_frames = 1;
            coremap[as->pagetable[i].page_frame_num].valid = 0;
//            kprintf("free %d", as->pagetable[i].page_frame_num);
        }
    }
    kfree(as->pagetable); //free page table before freeing the entire address space struct
    kfree(as);
}

void
as_activate(struct addrspace *as) {
    int i, spl;

    (void) as;

    spl = splhigh();

    for (i = 0; i < NUM_TLB; i++) {
        TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    }

    splx(spl);
}

int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
        int readable, int writeable, int executable) {
    size_t npages;
    
    size_t temp_sz = sz;

    /* Align the region. First, the base... */
    sz += vaddr & ~(vaddr_t) PAGE_FRAME;
    vaddr &= PAGE_FRAME;

    /* ...and now the length. */
    //increase size by rounding up to nearest PAGE_FRAME multiple
    sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

    npages = sz / PAGE_SIZE;

    /* We don't use these - all pages are read-write */
    (void) readable;
    (void) writeable;
    (void) executable;

    if (as->as_vbase_text == 0) {
        as->as_vbase_text = vaddr;
        as->as_npages_text = npages;
        as->as_sizeof_text = temp_sz;

//        //loop through page table and validate appropriate entries
//        int i;
//        int vm_num = vaddr / PAGE_SIZE;
//
//        for(i = 0; i < npages; i++){
//            as->pagetable[i].in_valid_region = 1;
//            as->pagetable[i].virt_frame_num = vm_num + i;
//            as->num_pt_entries++;
//        }

        return 0;
    }

    if (as->as_vbase_data == 0) {
        as->as_vbase_data = vaddr;
        as->as_npages_data = npages;
        as->as_sizeof_data = temp_sz;

//        //loop through page table and validate appropriate entries
//        int i;
//        int vm_num = vaddr / PAGE_SIZE;
//
//        for(i = as->as_npages_text; i < as->as_npages_text + npages; i++){
//            as->pagetable[i].in_valid_region = 1;
//            as->pagetable[i].virt_frame_num = vm_num + (i - as->as_npages_text);
//            as->num_pt_entries++;            
//        }

        //define heap as starting right after data section; initially heap base and top are pointing to same vaddr
        as->as_vbase_heap = as->as_vbase_data + (as->as_npages_data * PAGE_SIZE);
        as->as_vtop_heap = as->as_vbase_heap;

        return 0;
    }
    

    /*
     * Support for more than two regions is not available.
     */
    kprintf("dumbvm: Warning: too many regions\n");
    return EUNIMP;
}

int
as_prepare_load(struct addrspace *as) {
    int i;
    paddr_t physical_addr;

//    physical_addr = getppages(as->as_npages_text);
//    if (physical_addr == 0) {
//        return ENOMEM;
//    }
//kprintf("paddr text %d\n", physical_addr);
//    //loop through page table to store appropriate physical frame number for each page table entry
//    for(i = 0; i < as->as_npages_text; i++){
//        as->pagetable[i].present = 1;
//        as->pagetable[i].page_frame_num = ((physical_addr - first_pageframe_addr) / PAGE_SIZE) + i;
////kprintf("pfnum text %d\n" , as->pagetable[i].page_frame_num);   
// }

//    physical_addr = getppages(as->as_npages_data);
//    if (physical_addr == 0) {
//        return ENOMEM;
//    }
////kprintf("paddr data %d\n", physical_addr);
//    //loop through page table to store appropriate physical frame number for each page table entry
//    for(i = as->as_npages_text; i < as->as_npages_text + as->as_npages_data; i++){
//        as->pagetable[i].present = 1;
//        as->pagetable[i].page_frame_num = ((physical_addr - first_pageframe_addr) / PAGE_SIZE) + (i - as->as_npages_text);        
////kprintf("pfnum data %d\n", as->pagetable[i].page_frame_num);
//}

//    as->as_vbase_stack = USERTOP - PAGE_SIZE;
//    as->as_npages_stack = 1;
//    int first_stack_virt_page_frame = (USERTOP / PAGE_SIZE) - 1;
//    int temp = as->num_pt_entries;
//    as->pagetable[temp].in_valid_region = 1;
//    as->pagetable[temp].virt_frame_num = first_stack_virt_page_frame;
//    
//    physical_addr = getppages(1);
//    if (physical_addr == 0) {
//        return ENOMEM;
//    }
//    as->num_pt_entries++;  
//    as->pagetable[temp].present = 1;
//    as->pagetable[temp].page_frame_num = physical_addr / PAGE_SIZE;  
    
    as->as_vbase_stack = USERTOP;
    as->as_npages_stack = 0;
    
    return 0;
}

int
as_complete_load(struct addrspace *as) {
    (void) as;
    return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
    int i;
    for(i = 0; i < num_page_frames * 2; i++){
        if(as->pagetable[i].virt_frame_num == (USERTOP / PAGE_SIZE) - 1)
            break;
    }
    assert((as->pagetable[i].page_frame_num * PAGE_SIZE) + first_pageframe_addr != 0);

    *stackptr = USERSTACK;
    return 0;
}


//started to implement; need to copy the pagetable entries one by one and figure out the memmove function
// do we need to do PADDR_TO_KVADDR? do we need to get coremap entries for everything?
int
as_copy(struct addrspace *old, struct addrspace **ret) {
    struct addrspace *new;

    new = as_create();
    if (new == NULL) {
        return ENOMEM;
    }
    
    new->as_vbase_text = old->as_vbase_text;
    new->as_npages_text = old->as_npages_text;
    new->as_vbase_data = old->as_vbase_data;
    new->as_npages_data = old->as_npages_data;
    new->as_vbase_stack = old->as_vbase_stack;
    new->as_npages_stack = old->as_npages_stack;
    new->as_vbase_heap = old->as_vbase_heap;
    new->as_vtop_heap = old->as_vtop_heap;
    new->num_pt_entries = old->num_pt_entries; //max index where pte is in pagetable

    int i;
    int j;
    paddr_t paddr;
    //loop through old pagetable
    for(i = 0; i < num_page_frames * 2; i++){
        // if entry in old pagetable exists in ram, copy values of pagetable from old to new, except
        // page_frame_num; should allocate new coremap entry and set appropriate page_frame_num
        if(old->pagetable[i].present == 1){
            //copy values: present, in_valid_region, virt_frame_num
            new->pagetable[i].present = old->pagetable[i].present;
            new->pagetable[i].virt_frame_num = old->pagetable[i].virt_frame_num;
            new->pagetable[i].in_valid_region = old->pagetable[i].in_valid_region;
            
            //get page frame from coremap
            paddr = getppages(1);
            if(paddr == 0){
                as_destroy(new);
                return ENOMEM;
            }
            new->pagetable[i].page_frame_num = (paddr - first_pageframe_addr) / PAGE_SIZE;
            
            //copy over contents from old pagetable to new in physical memory
            memmove((void *) PADDR_TO_KVADDR((new->pagetable[i].page_frame_num * PAGE_SIZE) + first_pageframe_addr),
                    (const void *) PADDR_TO_KVADDR((old->pagetable[i].page_frame_num * PAGE_SIZE) + first_pageframe_addr),
                     PAGE_SIZE);
        }
    }
    
    *ret = new;
    return 0;
}


