#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <limits.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <mips/trapframe.h>
#include <vfs.h>
#include <vm.h>
#include "opt-A2.h"

#if OPT_A2
#define A2_MAX_ARGS 64
#define A2_MAX_ARGLEN 1024

struct a2_argpack {
	int argc;
	char *argv[A2_MAX_ARGS + 1];
	size_t len[A2_MAX_ARGS];
};

static
void
a2_argpack_cleanup(struct a2_argpack *args)
{
	int i;

	for (i = 0; i < args->argc; i++) {
		kfree(args->argv[i]);
		args->argv[i] = NULL;
	}
	args->argc = 0;
}

static
int
a2_copy_user_args(userptr_t uargv, struct a2_argpack *args)
{
	userptr_t uarg;
	size_t got;
	int result;

	args->argc = 0;
	if (uargv == NULL) {
		return EFAULT;
	}

	while (args->argc < A2_MAX_ARGS) {
		result = copyin((const_userptr_t)((vaddr_t)uargv +
		    args->argc * sizeof(userptr_t)), &uarg, sizeof(uarg));
		if (result) {
			a2_argpack_cleanup(args);
			return result;
		}
		if (uarg == NULL) {
			args->argv[args->argc] = NULL;
			return 0;
		}

		args->argv[args->argc] = kmalloc(A2_MAX_ARGLEN);
		if (args->argv[args->argc] == NULL) {
			a2_argpack_cleanup(args);
			return ENOMEM;
		}
		result = copyinstr((const_userptr_t)uarg, args->argv[args->argc],
		    A2_MAX_ARGLEN, &got);
		if (result) {
			a2_argpack_cleanup(args);
			return result == ENAMETOOLONG ? E2BIG : result;
		}
		args->len[args->argc] = got;
		args->argc++;
	}

	a2_argpack_cleanup(args);
	return E2BIG;
}

static
int
a2_copy_args_to_stack(struct a2_argpack *args, vaddr_t *stackptr,
    userptr_t *uargv_ret)
{
	userptr_t uargv[A2_MAX_ARGS + 1];
	int i, result;
	vaddr_t sp;

	sp = *stackptr;
	uargv[args->argc] = NULL;

	for (i = args->argc - 1; i >= 0; i--) {
		sp -= args->len[i];
		sp &= ~3;
		result = copyoutstr(args->argv[i], (userptr_t)sp,
		    args->len[i], NULL);
		if (result) {
			return result;
		}
		uargv[i] = (userptr_t)sp;
	}

	sp -= (args->argc + 1) * sizeof(userptr_t);
	sp &= ~3;
	result = copyout(uargv, (userptr_t)sp,
	    (args->argc + 1) * sizeof(userptr_t));
	if (result) {
		return result;
	}

	*stackptr = sp;
	*uargv_ret = (userptr_t)sp;
	return 0;
}
#endif /* OPT_A2 */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

#if OPT_A2
  proc_record_exit(_MKWAIT_EXIT(exitcode));
#endif /* OPT_A2 */

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
#if OPT_A2
  *retval = proc_getpid(curproc);
#else
  *retval = 1;
#endif /* OPT_A2 */
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  if (options != 0) {
    return(EINVAL);
  }
#if OPT_A2
  if (status == NULL) {
    return(EFAULT);
  }
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  result = proc_wait(pid, &exitstatus);
  if (result) {
    return(result);
  }
#else
  exitstatus = 0;
#endif /* OPT_A2 */
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

#if OPT_A2
struct fork_info {
	struct trapframe tf;
	struct addrspace *as;
};

int
sys_fork(struct trapframe *tf, pid_t *retval)
{
	struct fork_info *info;
	struct proc *child;
	int result;

	info = kmalloc(sizeof(*info));
	if (info == NULL) {
		return ENOMEM;
	}
	info->tf = *tf;

	result = as_copy(curproc_getas(), &info->as);
	if (result) {
		kfree(info);
		return result;
	}

	child = proc_create_runprogram(curproc->p_name);
	if (child == NULL) {
		as_destroy(info->as);
		kfree(info);
		return ENPROC;
	}
	proc_close_filetable(child);
	result = proc_copy_filetable(child, curproc);
	if (result) {
		as_destroy(info->as);
		proc_destroy(child);
		kfree(info);
		return result;
	}
	child->p_addrspace = info->as;

	result = thread_fork(child->p_name, child, enter_forked_process,
	    info, 0);
	if (result) {
		child->p_addrspace = NULL;
		as_destroy(info->as);
		proc_destroy(child);
		kfree(info);
		return result;
	}

	*retval = proc_getpid(child);
	return 0;
}

int
sys_execv(userptr_t program, userptr_t uargv)
{
	struct a2_argpack args;
	struct addrspace *oldas, *newas;
	struct vnode *v;
	char *kprogram;
	vaddr_t entrypoint, stackptr;
	userptr_t user_argv;
	int result;
	int argc;

	args.argc = 0;
	if (program == NULL || uargv == NULL) {
		return EFAULT;
	}

	kprogram = kmalloc(PATH_MAX);
	if (kprogram == NULL) {
		return ENOMEM;
	}
	result = copyinstr((const_userptr_t)program, kprogram, PATH_MAX, NULL);
	if (result) {
		kfree(kprogram);
		return result;
	}

	result = a2_copy_user_args(uargv, &args);
	if (result) {
		kfree(kprogram);
		return result;
	}

	result = vfs_open(kprogram, O_RDONLY, 0, &v);
	if (result) {
		a2_argpack_cleanup(&args);
		kfree(kprogram);
		return result;
	}

	newas = as_create();
	if (newas == NULL) {
		vfs_close(v);
		a2_argpack_cleanup(&args);
		kfree(kprogram);
		return ENOMEM;
	}

	oldas = curproc_setas(newas);
	as_activate();

	result = load_elf(v, &entrypoint);
	vfs_close(v);
	if (result) {
		curproc_setas(oldas);
		as_activate();
		as_destroy(newas);
		a2_argpack_cleanup(&args);
		kfree(kprogram);
		return result;
	}

	result = as_define_stack(newas, &stackptr);
	if (result == 0) {
		result = a2_copy_args_to_stack(&args, &stackptr, &user_argv);
	}
	if (result) {
		curproc_setas(oldas);
		as_activate();
		as_destroy(newas);
		a2_argpack_cleanup(&args);
		kfree(kprogram);
		return result;
	}

	argc = args.argc;
	as_destroy(oldas);
	a2_argpack_cleanup(&args);
	kfree(kprogram);
	enter_new_process(argc, user_argv, stackptr, entrypoint);
	panic("enter_new_process returned from execv\n");
	return EINVAL;
}
#endif /* OPT_A2 */
