#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <machine/trapframe.h>
#include <kern/types.h>
/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys_write(int filehandle, const void *buf, size_t size,  int *retval);
int sys_read(int filehandle, void *buf, size_t size, int *retval);
pid_t sys_fork(struct trapframe *tf, int *retval);
int sys_getpid(void);
int sys_waitpid(pid_t pid, int *returncode, int flags, int *retval);
void sys_exit(int code);
int sys_execv(const char *progname, char *const *args);
time_t sys_time(time_t *seconds, unsigned long *nanoseconds);
int sys_sbrk(int change, int *retval);

#endif /* _SYSCALL_H_ */
