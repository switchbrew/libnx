#define __LINUX_ERRNO_EXTENSIONS__
#include <errno.h>

#define EMEDIUMTYPE  0
#define ENOKEY       0
#define EKEYEXPIRED  0
#define EKEYREVOKED  0
#define EKEYREJECTED 0
#define ENOPROTOOP   0
#define EUCLEAN      0
#define ENOTNAM      0
#define ENAVAIL      0
#define EISNAM       0
#define EREMOTEIO    0
#define ERFKILL      0
#define EHWPOISON    0
#define ERESTART     0

static unsigned char _error_code_map[] = {
    0               , // 0
    EPERM           ,
    ENOENT          ,
    ESRCH           ,
    EINTR           ,
    EIO             , // 5
    ENXIO           ,
    E2BIG           ,
    ENOEXEC         ,
    EBADF           ,
    ECHILD          , // 10
    EAGAIN          ,
    ENOMEM          ,
    EACCES          ,
    EFAULT          ,
    ENOTBLK         , // 15
    EBUSY           ,
    EEXIST          ,
    EXDEV           ,
    ENODEV          ,
    ENOTDIR         , // 20
    EISDIR          ,
    EINVAL          ,
    ENFILE          ,
    EMFILE          ,
    ENOTTY          , // 25
    ETXTBSY         ,
    EFBIG           ,
    ENOSPC          ,
    ESPIPE          ,
    EROFS           ,  // 30
    EMLINK          ,
    EPIPE           ,
    EDOM            ,
    ERANGE          ,
    EDEADLK         , // 35
    ENAMETOOLONG    ,
    ENOLCK          ,
    ENOSYS          ,
    ENOTEMPTY       ,
    ELOOP           , // 40
    EAGAIN          ,
    ENOMSG          ,
    EIDRM           ,
    ECHRNG          ,
    EL2NSYNC        , // 45
    EL3HLT          ,
    EL3RST          ,
    ELNRNG          ,
    EUNATCH         ,
    ENOCSI          , // 50
    EL2HLT          ,
    EBADE           ,
    EBADR           ,
    EXFULL          ,
    ENOANO          , // 55
    EBADRQC         ,
    EBADSLT         ,
    EDEADLK         ,
    EBFONT          ,
    ENOSTR          , // 60
    ENODATA         ,
    ETIME           ,
    ENOSR           ,
    ENONET          ,
    ENOPKG          , // 65
    EREMOTE         ,
    ENOLINK         ,
    EADV            ,
    ESRMNT          ,
    ECOMM           , // 70
    EPROTO          ,
    EMULTIHOP       ,
    EDOTDOT         ,
    EBADMSG         ,
    EOVERFLOW       , // 75
    ENOTUNIQ        ,
    EBADFD          ,
    EREMCHG         ,
    ELIBACC         ,
    ELIBBAD         , // 80
    ELIBSCN         ,
    ELIBMAX         ,
    ELIBEXEC        ,
    EILSEQ          ,
    ERESTART        , // 85
    ESTRPIPE        ,
    EUSERS          ,
    ENOTSOCK        ,
    EDESTADDRREQ    ,
    EMSGSIZE        , // 90
    EPROTOTYPE      ,
    ENOPROTOOP      ,
    EPROTONOSUPPORT ,
    ESOCKTNOSUPPORT ,
    EOPNOTSUPP      , // 95
    EPFNOSUPPORT    ,
    EAFNOSUPPORT    ,
    EADDRINUSE      ,
    EADDRNOTAVAIL   ,
    ENETDOWN        , // 100
    ENETUNREACH     ,
    ENETRESET       ,
    ECONNABORTED    ,
    ECONNRESET      ,
    ENOBUFS         , // 105
    EISCONN         ,
    ENOTCONN        ,
    ESHUTDOWN       ,
    ETOOMANYREFS    ,
    ETIMEDOUT       , // 110
    ECONNREFUSED    ,
    EHOSTDOWN       ,
    EHOSTUNREACH    ,
    EALREADY        ,
    EINPROGRESS     , // 115
    ESTALE          ,
    EUCLEAN         ,
    ENOTNAM         ,
    ENAVAIL         ,
    EISNAM          , // 120
    EREMOTEIO       ,
    EDQUOT          ,
    ENOMEDIUM       ,
    EMEDIUMTYPE     ,
    ECANCELED       , // 125
    ENOKEY          ,
    EKEYEXPIRED     ,
    EKEYREVOKED     ,
    EKEYREJECTED    ,
    EOWNERDEAD      , // 130
    ENOTRECOVERABLE ,
    ERFKILL         ,
    EHWPOISON       ,
};

#define _UNKNOWN_ERROR_OFFSET	10000

int _convert_errno(int bsdErrno)
{

    if (bsdErrno < 0 || bsdErrno >= sizeof(_error_code_map) || _error_code_map[bsdErrno] == 0)
		return _UNKNOWN_ERROR_OFFSET + bsdErrno;

	return _error_code_map[bsdErrno];
}