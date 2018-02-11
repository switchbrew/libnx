#pragma once

//The below defines are based on Linux kernel ioctl.h.

#define _NV_IOC_NRBITS	8
#define _NV_IOC_TYPEBITS	8
#define _NV_IOC_SIZEBITS	14
#define _NV_IOC_DIRBITS	2

#define _NV_IOC_NRMASK	((1 << _NV_IOC_NRBITS)-1)
#define _NV_IOC_TYPEMASK	((1 << _NV_IOC_TYPEBITS)-1)
#define _NV_IOC_SIZEMASK	((1 << _NV_IOC_SIZEBITS)-1)
#define _NV_IOC_DIRMASK	((1 << _NV_IOC_DIRBITS)-1)

#define _NV_IOC_NRSHIFT	0
#define _NV_IOC_TYPESHIFT	(_NV_IOC_NRSHIFT+_NV_IOC_NRBITS)
#define _NV_IOC_SIZESHIFT	(_NV_IOC_TYPESHIFT+_NV_IOC_TYPEBITS)
#define _NV_IOC_DIRSHIFT	(_NV_IOC_SIZESHIFT+_NV_IOC_SIZEBITS)

/*
 * Direction bits.
 */
#define _NV_IOC_NONE	0U
#define _NV_IOC_WRITE	1U
#define _NV_IOC_READ	2U

#define _NV_IOC(dir,type,nr,size) \
	(((dir)  << _NV_IOC_DIRSHIFT) | \
	 ((type) << _NV_IOC_TYPESHIFT) | \
	 ((nr)   << _NV_IOC_NRSHIFT) | \
	 ((size) << _NV_IOC_SIZESHIFT))

/* used to create numbers */
#define _NV_IO(type,nr)		_NV_IOC(_NV_IOC_NONE,(type),(nr),0)
#define _NV_IOR(type,nr,size)	_NV_IOC(_NV_IOC_READ,(type),(nr),sizeof(size))
#define _NV_IOW(type,nr,size)	_NV_IOC(_NV_IOC_WRITE,(type),(nr),sizeof(size))
#define _NV_IOWR(type,nr,size)	_NV_IOC(_NV_IOC_READ|_NV_IOC_WRITE,(type),(nr),sizeof(size))

/* used to decode ioctl numbers.. */
#define _NV_IOC_DIR(nr)		(((nr) >> _NV_IOC_DIRSHIFT) & _NV_IOC_DIRMASK)
#define _NV_IOC_TYPE(nr)		(((nr) >> _NV_IOC_TYPESHIFT) & _NV_IOC_TYPEMASK)
#define _NV_IOC_NR(nr)		(((nr) >> _NV_IOC_NRSHIFT) & _NV_IOC_NRMASK)
#define _NV_IOC_SIZE(nr)		(((nr) >> _NV_IOC_SIZESHIFT) & _NV_IOC_SIZEMASK)

#define __nv_in
#define __nv_out
#define __nv_inout
