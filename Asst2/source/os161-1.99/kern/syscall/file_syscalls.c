#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <kern/stat.h>
#include <kern/unistd.h>
#include <limits.h>
#include <lib.h>
#include <uio.h>
#include <syscall.h>
#include <vnode.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include <copyinout.h>
#include <addrspace.h>
#include <synch.h>
#include "opt-A2.h"

#if OPT_A2
static
void
filehandle_decref(struct filehandle *fh)
{
	lock_acquire(fh->lock);
	fh->refcount--;
	if (fh->refcount == 0) {
		lock_release(fh->lock);
		vfs_close(fh->vn);
		lock_destroy(fh->lock);
		kfree(fh);
	}
	else {
		lock_release(fh->lock);
	}
}

static
int
fd_is_valid(int fd)
{
	return fd >= 0 && fd < OPEN_MAX && curproc->p_filetable[fd] != NULL;
}

static
int
fd_alloc(void)
{
	int fd;

	for (fd = STDERR_FILENO + 1; fd < OPEN_MAX; fd++) {
		if (curproc->p_filetable[fd] == NULL) {
			return fd;
		}
	}
	return -1;
}

static
int
flags_are_valid(int flags)
{
	int accmode;
	int valid;

	accmode = flags & O_ACCMODE;
	if (accmode != O_RDONLY && accmode != O_WRONLY && accmode != O_RDWR) {
		return 0;
	}

	valid = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND | O_NOCTTY;
	return (flags & ~valid) == 0;
}

static
int
can_read(int flags)
{
	int accmode;

	accmode = flags & O_ACCMODE;
	return accmode == O_RDONLY || accmode == O_RDWR;
}

static
int
can_write(int flags)
{
	int accmode;

	accmode = flags & O_ACCMODE;
	return accmode == O_WRONLY || accmode == O_RDWR;
}

int
sys_open(userptr_t filename, int flags, mode_t mode, int *retval)
{
	struct vnode *vn;
	struct filehandle *fh;
	char *kfilename;
	int fd;
	int result;

	if (filename == NULL) {
		return EFAULT;
	}
	if (!flags_are_valid(flags)) {
		return EINVAL;
	}

	fd = fd_alloc();
	if (fd < 0) {
		return EMFILE;
	}

	kfilename = kmalloc(PATH_MAX);
	if (kfilename == NULL) {
		return ENOMEM;
	}
	result = copyinstr((const_userptr_t)filename, kfilename, PATH_MAX, NULL);
	if (result) {
		kfree(kfilename);
		return result;
	}

	result = vfs_open(kfilename, flags, mode, &vn);
	kfree(kfilename);
	if (result) {
		return result;
	}

	fh = kmalloc(sizeof(*fh));
	if (fh == NULL) {
		vfs_close(vn);
		return ENOMEM;
	}
	fh->lock = lock_create("open file");
	if (fh->lock == NULL) {
		vfs_close(vn);
		kfree(fh);
		return ENOMEM;
	}
	fh->vn = vn;
	fh->offset = 0;
	fh->flags = flags;
	fh->refcount = 1;

	curproc->p_filetable[fd] = fh;
	*retval = fd;
	return 0;
}

int
sys_close(int fd)
{
	struct filehandle *fh;

	if (!fd_is_valid(fd)) {
		return EBADF;
	}

	fh = curproc->p_filetable[fd];
	curproc->p_filetable[fd] = NULL;
	filehandle_decref(fh);
	return 0;
}

int
sys_remove(userptr_t pathname)
{
	char *kpathname;
	int result;

	if (pathname == NULL) {
		return EFAULT;
	}

	kpathname = kmalloc(PATH_MAX);
	if (kpathname == NULL) {
		return ENOMEM;
	}

	result = copyinstr((const_userptr_t)pathname, kpathname, PATH_MAX, NULL);
	if (result) {
		kfree(kpathname);
		return result;
	}

	result = vfs_remove(kpathname);
	kfree(kpathname);
	return result;
}

int
sys_read(int fd, userptr_t ubuf, size_t buflen, int *retval)
{
	struct filehandle *fh;
	struct iovec iov;
	struct uio u;
	int result;

	if (ubuf == NULL) {
		return EFAULT;
	}
	if (!fd_is_valid(fd) || !can_read(curproc->p_filetable[fd]->flags)) {
		return EBADF;
	}

	fh = curproc->p_filetable[fd];
	lock_acquire(fh->lock);

	iov.iov_ubase = ubuf;
	iov.iov_len = buflen;
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_offset = fh->offset;
	u.uio_resid = buflen;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = curproc->p_addrspace;

	result = VOP_READ(fh->vn, &u);
	if (result == 0) {
		fh->offset = u.uio_offset;
		*retval = buflen - u.uio_resid;
	}

	lock_release(fh->lock);
	return result;
}

int
sys_write(int fd, userptr_t ubuf, unsigned int nbytes, int *retval)
{
	struct filehandle *fh;
	struct stat st;
	struct iovec iov;
	struct uio u;
	int result;

	if (ubuf == NULL) {
		return EFAULT;
	}
	if (!fd_is_valid(fd) || !can_write(curproc->p_filetable[fd]->flags)) {
		return EBADF;
	}

	fh = curproc->p_filetable[fd];
	lock_acquire(fh->lock);

	if ((fh->flags & O_APPEND) != 0) {
		result = VOP_STAT(fh->vn, &st);
		if (result) {
			lock_release(fh->lock);
			return result;
		}
		fh->offset = st.st_size;
	}

	iov.iov_ubase = ubuf;
	iov.iov_len = nbytes;
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_offset = fh->offset;
	u.uio_resid = nbytes;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_WRITE;
	u.uio_space = curproc->p_addrspace;

	result = VOP_WRITE(fh->vn, &u);
	if (result == 0) {
		fh->offset = u.uio_offset;
		*retval = nbytes - u.uio_resid;
	}

	lock_release(fh->lock);
	return result;
}
#else
int
sys_write(int fdesc,userptr_t ubuf,unsigned int nbytes,int *retval)
{
  struct iovec iov;
  struct uio u;
  int res;

  DEBUG(DB_SYSCALL,"Syscall: write(%d,%x,%d)\n",fdesc,(unsigned int)ubuf,nbytes);
  
  if (!((fdesc==STDOUT_FILENO)||(fdesc==STDERR_FILENO))) {
    return EUNIMP;
  }
  KASSERT(curproc != NULL);
  KASSERT(curproc->console != NULL);
  KASSERT(curproc->p_addrspace != NULL);

  iov.iov_ubase = ubuf;
  iov.iov_len = nbytes;
  u.uio_iov = &iov;
  u.uio_iovcnt = 1;
  u.uio_offset = 0;
  u.uio_resid = nbytes;
  u.uio_segflg = UIO_USERSPACE;
  u.uio_rw = UIO_WRITE;
  u.uio_space = curproc->p_addrspace;

  res = VOP_WRITE(curproc->console,&u);
  if (res) {
    return res;
  }

  *retval = nbytes - u.uio_resid;
  KASSERT(*retval >= 0);
  return 0;
}
#endif /* OPT_A2 */
