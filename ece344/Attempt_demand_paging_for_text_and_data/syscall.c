#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>
#include <uio.h>
#include <kern/unistd.h>
#include <thread.h>
#include <addrspace.h>
#include <curthread.h>
#include <synch.h>
#include <clock.h>

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void
mips_syscall(struct trapframe *tf) {
    int callno;
    int32_t retval;
    int err;

    assert(curspl == 0);

    callno = tf->tf_v0;

    /*
     * Initialize retval to 0. Many of the system calls don't
     * really return a value, just 0 for success and -1 on
     * error. Since retval is the value returned on success,
     * initialize it to 0 by default; thus it's not necessary to
     * deal with it except for calls that return other values, 
     * like write.
     */

    retval = 0;

    switch (callno) {
        case SYS_reboot:
            err = sys_reboot(tf->tf_a0);
            break;

            /* Add stuff here */
        case SYS_write:
            err = sys_write(tf->tf_a0, (void *) tf->tf_a1, (size_t) tf->tf_a2, &retval);
            break;

        case SYS_read:
            err = sys_read(tf->tf_a0, (void *) tf->tf_a1, (size_t) tf->tf_a2, &retval);
            break;

        case SYS_fork:
            err = sys_fork(tf, &retval);
            break;

        case SYS_getpid:
            err = 0;
            retval = sys_getpid();
            break;

        case SYS__exit:
            sys_exit(tf->tf_a0);
            err = 0;
            break;

        case SYS_waitpid:
            err = sys_waitpid((pid_t) tf->tf_a0, (int *) tf->tf_a1, tf->tf_a2, &retval);
            break;

        case SYS_execv:
            err = sys_execv((const char *) tf->tf_a0, (char *const *) tf->tf_a1);
            break;

        case SYS___time:
            retval = sys_time((time_t *) tf->tf_a0, (unsigned long *) tf->tf_a1);
            err = 0;
            if (retval == -1)
                err = EFAULT;
            break;

        case SYS_sbrk:
            err = sys_sbrk((int) tf->tf_a0, &retval);
            break;

        default:
            kprintf("Unknown syscall %d\n", callno);
            err = ENOSYS;
            break;
    }


    if (err) {
        /*
         * Return the error code. This gets converted at
         * userlevel to a return value of -1 and the error
         * code in errno.
         */
        tf->tf_v0 = err;
        tf->tf_a3 = 1; /* signal an error */
    } else {
        /* Success. */
        tf->tf_v0 = retval;
        tf->tf_a3 = 0; /* signal no error */
    }

    /*
     * Now, advance the program counter, to avoid restarting
     * the syscall over and over again.
     */

    tf->tf_epc += 4;

    /* Make sure the syscall code didn't forget to lower spl */
    assert(curspl == 0);
}

int sys_write(int filehandle, const void *buf, size_t size, int* retval) {
    P(write_sem);

    int result;
    char *kbuf;

    // check if filehandles are invalid
    // must be either console or error console
    if (filehandle != STDOUT_FILENO && filehandle != STDERR_FILENO) {
        V(write_sem);
        return EBADF;
    }

    if (size == 0) {
        V(write_sem);
        return EFAULT;
    }

    // check if buffer address is invalid (i.e. not assigned)
    if (buf == NULL) {
        V(write_sem);
        return EFAULT;
    }

    kbuf = kmalloc(sizeof (*buf) * size);

    // check if kmalloc work (if space is available)
    if (kbuf == NULL) {
        kfree(kbuf);
        V(write_sem);
        return ENOSPC;
    }

    result = copyin((const_userptr_t) buf, kbuf, size);

    //EFAULT
    if (result > 0) {
        kfree(kbuf);
        V(write_sem);
        return EFAULT;
    }

    unsigned i;
    for (i = 0; i < size; i++) {
        putch(kbuf[i]);
    }

    *retval = size;
    kfree(kbuf);

    V(write_sem);

    return 0;
}

int sys_read(int filehandle, void *buf, size_t size, int* retval) {
    P(read_sem);

    int result = 0;
    char *kbuf;

    if (filehandle != STDIN_FILENO /*&& filehandle != STDERR_FILENO*/) {
        V(read_sem);
        return EBADF;
    }

    if (size == 0) {
        V(read_sem);
        return EFAULT;
    }

    // check if buffer address is invalid (i.e. not assigned)
    if (buf == NULL) {
        V(read_sem);
        return EFAULT;
    }

    //    if(size < 0){
    //        V(read_sem);
    //        return EINVAL;
    //    }

    kbuf = kmalloc(sizeof (*buf) * size);

    if (kbuf == NULL) {
        kfree(kbuf);
        V(read_sem);
        return ENOSPC;
    }

    unsigned i;
    for (i = 0; i < size; i++) {
        kbuf[i] = getch();
    }


    result = copyout(kbuf, buf, size);
    //EFAULT
    if (result > 0) {
        kfree(kbuf);
        V(read_sem);
        return EFAULT;
    }

    *retval = size;
    kfree(kbuf);
    V(read_sem);

    return 0;
}

int sys_getpid() {
    return curthread->pid;
}

pid_t sys_fork(struct trapframe *tf, int *retval) {

    //variable declarations
    int has_error;
    struct addrspace *child_as;
    struct trapframe *child_tf;
    struct thread *child_thread;

    //Create a new address space, and copy parent's addrspace to child's
    has_error = as_copy(curthread->t_vmspace, &child_as); //return ENOMEM if fail
    if (has_error != 0) {
        return ENOMEM; //error code if no memory available
    }

    //Copy the trapframe to child's trapframe
    child_tf = kmalloc(sizeof (struct trapframe));
    if (child_tf == NULL) { //if no memory allocated, return error ENOMEM
        return ENOMEM;
    }

    //copy trapframe data
    *child_tf = *tf;

    //update child trapframe to return correct values and no error
    child_tf->tf_v0 = 0; // child returns 0 for fork syscall
    child_tf->tf_a3 = 0; // no error for syscall (success)
    child_tf->tf_epc = child_tf->tf_epc + 4; // increment program counter for child so it skips over thread_fork()

    //create a new child thread and fork
    //returns the relevant error code from thread fork
    has_error = thread_fork("Ch_thread",
            child_tf, (unsigned long) child_as,
            md_forkentry,
            &child_thread);
    if (has_error != 0) {
        return has_error;
    }

    //parent returns child's pid for fork syscall
    *retval = child_thread->pid;

    return 0;
}

void
md_forkentry(struct trapframe *tf, struct addrspace *child_as) {
    /*
     * This function is provided as a reminder. You need to write
     * both it and the code that calls it.
     *
     * Thus, you can trash it and do things another way if you prefer.
     */

    //Need to create a trapframe in child thread to be able to call mips_usermode
    //trapframe needs to be on stack to work (not heap)
    struct trapframe child_tf = *tf;

    kfree(tf);

    curthread->t_vmspace = child_as;
    as_activate(child_as);
    mips_usermode(&child_tf);

}

void sys_exit(int code) {
    // gain access to pid linked list
    P(procLL_sem);

    struct process* curproc;
    curproc = find_process(curthread->pid);

    curproc->exit_code = code; //set exit code in current process
    curproc->has_exited = 1; // process has now exited


    V(procLL_sem);
    cv_broadcast(waitpid_cv, waitpid_lock);

    // exit current thread
    thread_exit();
}

int sys_waitpid(pid_t pid, int *returncode, int flags, int *retval) {
    // error checking arugments
    if (flags != 0)
        return EINVAL;
    if (pid == curthread->pid)
        return EINVAL;
    if (returncode == NULL)
        return EFAULT;

    //Check if address where returncode is stored is aligned
    if (((int) returncode % 4) != 0)
        return EFAULT;

    // if called from usermode, check for invalid pointer or kernel pointer for returncode arg
    if (calling_waitpid_from_kern == 0) {
        int *badptr_check = kmalloc(sizeof (int));
        if (badptr_check == NULL) {
            kfree(badptr_check);
            return ENOMEM;
        }
        int result;
        result = copyin((const_userptr_t) returncode, badptr_check, sizeof (int *));
        kfree(badptr_check);
        if (result > 0)
            return result;
    } else
        calling_waitpid_from_kern = 0; //set back to usermode

    //Need to add restriction that only parents can wait on their children
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    //find child process
    struct process *waitee_process;
    waitee_process = find_process(pid);
    // if given pid is not found, return error
    if (waitee_process == NULL)
        return EINVAL;

    //check if parent process is waiting on child (only parent can wait on child)
    if (curthread->pid != waitee_process->parent_pid)
        return EINVAL;

    //check if child has already been waited on by parent
    if (waitee_process->was_waited_on == 1)
        return EINVAL;

    waitee_process->was_waited_on = 1;

    if (waitee_process->has_exited == 1) {
        *returncode = waitee_process->exit_code;
        return 0;
    }


    lock_acquire(waitpid_lock);
    while (waitee_process->has_exited == 0) { //process is still running
        cv_wait(waitpid_cv, waitpid_lock);
    }
    *retval = pid;
    *returncode = waitee_process->exit_code;
    lock_release(waitpid_lock);

    return 0;
}

int sys_execv(const char *progname, char *const *args) {
    int result;
    int j;

    //Check for validity of arguments
    if (args == NULL || progname == NULL)
        return EFAULT;

    //Check if progname is invalid pointer or kernel pointer
    char *badptr_check = kmalloc(sizeof (char *));
    if (badptr_check == NULL) {
        kfree(badptr_check);
        return ENOMEM;
    }
    result = copyin((const_userptr_t) progname, badptr_check, sizeof (char *));
    kfree(badptr_check);
    if (result > 0)
        return result;

    char **badptr_args_check = kmalloc(sizeof (char **));
    if (badptr_args_check == NULL) {
        kfree(badptr_args_check);
        return ENOMEM;
    }
    result = copyin((const_userptr_t) args, badptr_args_check, sizeof (char **));
    kfree(badptr_args_check);
    if (result > 0)
        return result;

    if (strcmp(progname, "") == 0)
        return EINVAL;

    //Count number of arguments
    int i = 0;
    int nargs = 0;
    while (args[i] != NULL) {
        nargs++;
        i++;
    }
    //    nargs++; //include progname into nargs

    //Count the number of characters
    int numChars = 0;
    for (i = 0; i < nargs; i++) {
        numChars += strlen(args[i]);
    }

    //Copy arguments into kernel space
    char **kargs;
    kargs = kmalloc(sizeof (char*)*nargs);
    if (kargs == NULL) {
        kfree(kargs);
        return ENOMEM;
    }

    //    char *kargs_check;
    //    kargs_check = kmalloc(sizeof(char*));
    //    if (kargs_check == NULL){
    //        kfree(kargs);
    //        kfree(kargs_check);
    //        return ENOMEM;
    //    }
    //
    //    //Check if argument pointers is invalid pointer or kernel pointer
    //    for (i = 0; i < nargs; i++) {
    //        result = copyin((const_userptr_t)&args[i], kargs_check, sizeof(char *));
    //            kfree(kargs_check);
    //        if (result > 0){
    //            kfree(kargs);
    //            return result;
    //        }
    //        kargs_check = kmalloc(sizeof(char*));
    //        if (kargs_check == NULL) {
    //            kfree(kargs);
    //            kfree(kargs_check);
    //            return ENOMEM;
    //        }
    //    }
    //    
    //allocate space for each argument in kargs
    for (i = 0; i < nargs; i++) {
        kargs[i] = kmalloc((sizeof (args[i]) * strlen(args[i]) + 1)); //add one to add space for null terminating char
        if (kargs[i] == NULL) {
            for (j = 0; j < i; j++)
                kfree(kargs[j]);
            kfree(kargs);
            return ENOMEM;
        }
    }

    //copying in each argument into kargs
    for (i = 0; i < nargs; i++) {
        result = copyin((const_userptr_t) args[i], kargs[i], strlen(args[i]) + 1);
        if (result > 0) {
            for (j = 0; j < nargs; j++)
                kfree(kargs[j]);
            kfree(kargs);
            return result;
        }
    }


    struct vnode *v;
    vaddr_t entrypoint, stackptr;

    /* Open the file. */
    result = vfs_open(progname, O_RDONLY, &v);
    if (result) {
        for (j = 0; j < nargs; j++)
            kfree(kargs[j]);
        kfree(kargs);
        return result;
    }

    //Destroy current thread's address space, and set to NULL before proceeding
    as_destroy(curthread->t_vmspace);
    curthread->t_vmspace = NULL;

    /* We should be a new thread. */
    assert(curthread->t_vmspace == NULL);

    /* Create a new address space. */
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace == NULL) {
        for (j = 0; j < nargs; j++)
            kfree(kargs[j]);
        kfree(kargs);
        vfs_close(v);
        return ENOMEM;
    }

    /* Activate it. */
    as_activate(curthread->t_vmspace);

    //copy the progname to addrspace struct
    curthread->t_vmspace->progname = kmalloc(sizeof (char)*strlen(progname));
    strcpy(curthread->t_vmspace->progname, progname);

    /* Load the executable. */
    result = load_elf(v, &entrypoint);
    if (result) {
        for (j = 0; j < nargs; j++)
            kfree(kargs[j]);
        kfree(kargs);
        /* thread_exit destroys curthread->t_vmspace */
        vfs_close(v);
        return result;
    }

    /* Done with the file now. */
    vfs_close(v);

    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
        for (j = 0; j < nargs; j++)
            kfree(kargs[j]);
        kfree(kargs);
        /* thread_exit destroys curthread->t_vmspace */
        return result;
    }


    //Count total number of characters for all arguments
    //        int numChars = 0;
    vaddr_t *kargs_addrs = kmalloc(sizeof (vaddr_t)*(nargs + 1));
    kargs_addrs[nargs] = NULL;
    //        for(i = 0; i < nargs; i++){
    //            numChars += strlen(args[i]);
    //        }

    vaddr_t stack_index = stackptr;
    //make sure stack_index is aligned by 4 bytes
    while ((int) stack_index % 4 != 0)
        stack_index--;

    //Copy each pointer and value of argument to user space
    int copyout_result;
    //        stack_index = stackptr - (((nargs + 1) * sizeof(char *)) + numChars);  //move stack pointer down by total size of arguments plus null char*

    //store value of args onto stack
    for (i = (int) nargs - 1; i >= 0; i--) {
        stack_index -= (strlen(kargs[i]) + 1);
        while ((int) stack_index % 4 != 0) //make sure stack_index is aligned by 4 bytes
            stack_index--;
        copyout_result = copyout(kargs[i], stack_index, strlen(kargs[i]) + 1);
        if (copyout_result > 0) {
            for (j = 0; j < nargs; j++)
                kfree(kargs[j]);
            kfree(kargs);
            kfree(kargs_addrs);
            return copyout_result;
        }
        kargs_addrs[i] = stack_index; //store address of arg into array
    }

    //        //store null onto stack after arg pointers
    //        char* null_ptr = NULL;
    //        stack_index -= sizeof(char*);
    //        copyout_result = copyout(null_ptr, stack_index, sizeof(char*));

    while ((int) stack_index % 4 != 0) //make sure stack_index is aligned by 4 bytes
        stack_index--;

    //store pointers to args onto stack
    for (i = (int) nargs; i >= 0; i--) {
        stack_index -= sizeof (vaddr_t);
        copyout_result = copyout(&kargs_addrs[i], stack_index, sizeof (vaddr_t));
        if (copyout_result > 0) {
            for (j = 0; j < nargs; j++)
                kfree(kargs[j]);
            kfree(kargs);
            kfree(kargs_addrs);
            return copyout_result;
        }
    }
    for (j = 0; j < nargs; j++)
        kfree(kargs[j]);
    kfree(kargs);
    kfree(kargs_addrs);

    //        stack_index = stackptr - (nargs * sizeof(char *) + numChars);  //move stack pointer down by total size of arguments

    /* Warp to user mode. */
    md_usermode(nargs /*argc*/, stack_index /*userspace addr of argv*/,
            stack_index, entrypoint);

    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}

time_t sys_time(time_t *seconds, unsigned long *nanoseconds) {
    //kernel space copy
    time_t *ksecs = kmalloc(sizeof (time_t));
    unsigned long *knsecs = kmalloc(sizeof (unsigned long));
    int result;

    //get time
    gettime(ksecs, knsecs);

    //copy out of kernel space
    if (seconds != NULL) {
        //check for invalid non-NULL address and copy out to userspace
        result = copyout((const_userptr_t) ksecs, seconds, sizeof (time_t));
        if (result > 0) {
            kfree(ksecs);
            kfree(knsecs);
            return -1;
        }
    }

    if (nanoseconds != NULL) {
        //check for invalid non-NULL address and copy out to userspace
        result = copyout((const_userptr_t) knsecs, nanoseconds, sizeof (unsigned long));
        if (result > 0) {
            kfree(ksecs);
            kfree(knsecs);
            return -1;
        }
    }

    time_t secs = *ksecs;

    kfree(knsecs);
    kfree(ksecs);

    return secs;
}

int sys_sbrk(int change, int *retval) {
    vaddr_t vbase_text = curthread->t_vmspace->as_vbase_text;
    size_t npages_text = curthread->t_vmspace->as_npages_text;
    vaddr_t vbase_data = curthread->t_vmspace->as_vbase_data;
    size_t npages_data = curthread->t_vmspace->as_npages_data;
    vaddr_t vbase_stack = curthread->t_vmspace->as_vbase_stack;
    size_t npages_stack = curthread->t_vmspace->as_npages_stack;
    vaddr_t vbase_heap = curthread->t_vmspace->as_vbase_heap;
    vaddr_t *vtop_heap = &(curthread->t_vmspace->as_vtop_heap);
    int num_pt_entries = curthread->t_vmspace->num_pt_entries;
    
    int npages;
    paddr_t paddr;
    int i;
    int j;
    int index;
    
    
    //store the old break value
    *retval = curthread->t_vmspace->as_vtop_heap;
    
    //if change == 0, return current heap top addr
    if (change == 0) {
        *retval = *vtop_heap;
        return 0;
    }
    

    //if change is positive or negative, get allocate necessary pages to increase or decrease heap top
    if (change < 0) {
        //check if change is word aligned, only allow page aligned

//        if (*vtop_heap - change % PAGE_SIZE != 0) {
//                *retval = (void *) - 1;
//                return EINVAL;
//            }

        //check change does not push past heap base
        if (*vtop_heap - change < vbase_heap) {
            *retval = (void *) - 1;
            return EINVAL;
        }

        //calculate # of pages needed
        npages = (((change*-1) - 1) / PAGE_SIZE) + 1;

        //free pagetable entries and coremap entries
        for(j = 0; j < npages; j++){
            for (i = 0; i < num_page_frames * 2; i++) {
                        //if pagetable entry is for heap then invalidate (free) it
                if (curthread->t_vmspace->pagetable[i].virt_frame_num * PAGE_SIZE == *vtop_heap - PAGE_SIZE) {
                    
                    //freeing entry in coremap
                    coremap[curthread->t_vmspace->pagetable[i].page_frame_num].v_page = NULL;
                    coremap[curthread->t_vmspace->pagetable[i].page_frame_num].starting_page = 0;
                    coremap[curthread->t_vmspace->pagetable[i].page_frame_num].num_page_frames = 1;
                    coremap[curthread->t_vmspace->pagetable[i].page_frame_num].valid = 0;
                    
                    //freeing entry in pagetable
                    curthread->t_vmspace->pagetable[i].present = 0;
                    curthread->t_vmspace->pagetable[i].virt_frame_num = -1;
                    curthread->t_vmspace->pagetable[i].page_frame_num = -1;
                    curthread->t_vmspace->pagetable[i].in_valid_region = 0;
                    break;
                }
            }
//            *vtop_heap -= PAGE_SIZE;
            *vtop_heap -= change;
        }
        return 0;
    }

    if (change > 0) {
        //check if change is word aligned, only allow page aligned
//        if (*vtop_heap + change % PAGE_SIZE != 0) {
//                *retval = (void *) - 1;
//                return EINVAL;
//            }

        //check change does not push past stack base
        if (*vtop_heap + change > vbase_stack) {
            *retval = (void *) - 1;
            return ENOMEM;
        }
        
        //check if change does not grow the heap past 16MB limit
        if((*vtop_heap + change) - vbase_heap > HEAPLIMIT){
            *retval = (void *) - 1;
            return ENOMEM;          
        }
        
        //calculate # of pages needed
        npages = ((change - 1) / PAGE_SIZE)  + 1;
        
//        //check if space available in coremap
//        if(check_getppages(npages) == 0){
//            *retval = (void *) - 1;
//            return ENOMEM;
//        }
        
        
//        //space is available in coremap, so allocate and update pagetable
//        for(i = 0; i < npages; i++){
//            paddr = getppages(1); // allocate appropriate pages in pagetable and coremap
//            if (paddr == 0) { //no mem
//                *retval = (void *) - 1;
//                return ENOMEM;
//            }
//
//            for (index = 0; index < num_page_frames * 2; index++) {
//                if (curthread->t_vmspace->pagetable[index].present == 0) {
//                    curthread->t_vmspace->num_pt_entries++;
//                            curthread->t_vmspace->pagetable[index].page_frame_num = (paddr - first_pageframe_addr) / PAGE_SIZE;
//                            curthread->t_vmspace->pagetable[index].present = 1;
//                            curthread->t_vmspace->pagetable[index].in_valid_region = 1;
//                            curthread->t_vmspace->pagetable[index].virt_frame_num = *vtop_heap / PAGE_SIZE;
//                            *vtop_heap += PAGE_SIZE;
//                    break;
//                }
//            }
//            
//        }

        //space is available in coremap, so allocate and update pagetable
//        paddr = getppages(1); // allocate appropriate pages in pagetable and coremap
//        if (paddr == 0) { //no mem
//            *retval = (void *) - 1;
//            return ENOMEM;
//        }
//
//        for (index = 0; index < num_page_frames * 2; index++) {
//                if (curthread->t_vmspace->pagetable[index].present == 0) {
//                    curthread->t_vmspace->num_pt_entries++;
//                    curthread->t_vmspace->pagetable[index].page_frame_num = (paddr - first_pageframe_addr) / PAGE_SIZE;
//                    curthread->t_vmspace->pagetable[index].present = 1;
//                    curthread->t_vmspace->pagetable[index].in_valid_region = 1;
//                    curthread->t_vmspace->pagetable[index].virt_frame_num = *vtop_heap / PAGE_SIZE;
//                    *vtop_heap += npages * PAGE_SIZE;
//                    break;
//                }
//        }
//        *vtop_heap += npages * PAGE_SIZE;
        *vtop_heap += change;
        return 0;
    }
}



