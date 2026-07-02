/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <limits.h>
#include <lib.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <copyinout.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include "opt-A2.h"

#if OPT_A2
#define A2_MAX_ARGS 64

static
int
runprogram_copy_args(int nargs, char **args, vaddr_t *stackptr,
    userptr_t *uargv_ret)
{
	userptr_t uargv[A2_MAX_ARGS + 1];
	size_t len;
	vaddr_t sp;
	int i, result;

	if (nargs < 0 || nargs > A2_MAX_ARGS) {
		return E2BIG;
	}

	sp = *stackptr;
	uargv[nargs] = NULL;

	for (i = nargs - 1; i >= 0; i--) {
		len = strlen(args[i]) + 1;
		if (len > ARG_MAX) {
			return E2BIG;
		}
		sp -= len;
		sp &= ~3;
		result = copyoutstr(args[i], (userptr_t)sp, len, NULL);
		if (result) {
			return result;
		}
		uargv[i] = (userptr_t)sp;
	}

	sp -= (nargs + 1) * sizeof(userptr_t);
	sp &= ~3;
	result = copyout(uargv, (userptr_t)sp,
	    (nargs + 1) * sizeof(userptr_t));
	if (result) {
		return result;
	}

	*stackptr = sp;
	*uargv_ret = (userptr_t)sp;
	return 0;
}

int
runprogram_args(int nargs, char **args)
{
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	userptr_t uargv;
	int result;

	KASSERT(nargs >= 1);

	result = vfs_open(args[0], O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	KASSERT(curproc_getas() == NULL);

	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	curproc_setas(as);
	as_activate();

	result = load_elf(v, &entrypoint);
	if (result) {
		vfs_close(v);
		return result;
	}

	vfs_close(v);

	result = as_define_stack(as, &stackptr);
	if (result) {
		return result;
	}

	result = runprogram_copy_args(nargs, args, &stackptr, &uargv);
	if (result) {
		return result;
	}

	enter_new_process(nargs, uargv, stackptr, entrypoint);
	panic("enter_new_process returned\n");
	return EINVAL;
}
#endif /* OPT_A2 */

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname)
{
#if OPT_A2
	char *args[2];

	args[0] = progname;
	args[1] = NULL;
	return runprogram_args(1, args);
#else
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new process. */
	KASSERT(curproc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	/* Warp to user mode. */
	enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
			  stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
#endif /* OPT_A2 */
}
