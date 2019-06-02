#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <unistd.h>

#include "runtime/devices/fs_dev.h"
#include "runtime/util/utf.h"
#include "services/fs.h"


/*! @internal
 *
 *  @file fsdev_dev.c
 *
 *  FS Device
 */

static int fsdev_translate_error(Result error);

static int       fsdev_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
static int       fsdev_close(struct _reent *r, void *fd);
static ssize_t   fsdev_write(struct _reent *r, void *fd, const char *ptr, size_t len);
static ssize_t   fsdev_write_safe(struct _reent *r, void *fd, const char *ptr, size_t len);
static ssize_t   fsdev_read(struct _reent *r, void *fd, char *ptr, size_t len);
static ssize_t   fsdev_read_safe(struct _reent *r, void *fd, char *ptr, size_t len);
static off_t     fsdev_seek(struct _reent *r, void *fd, off_t pos, int dir);
static int       fsdev_fstat(struct _reent *r, void *fd, struct stat *st);
static int       fsdev_stat(struct _reent *r, const char *file, struct stat *st);
static int       fsdev_link(struct _reent *r, const char *existing, const char  *newLink);
static int       fsdev_unlink(struct _reent *r, const char *name);
static int       fsdev_chdir(struct _reent *r, const char *name);
static int       fsdev_rename(struct _reent *r, const char *oldName, const char *newName);
static int       fsdev_mkdir(struct _reent *r, const char *path, int mode);
static DIR_ITER* fsdev_diropen(struct _reent *r, DIR_ITER *dirState, const char *path);
static int       fsdev_dirreset(struct _reent *r, DIR_ITER *dirState);
static int       fsdev_dirnext(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat);
static int       fsdev_dirclose(struct _reent *r, DIR_ITER *dirState);
static int       fsdev_statvfs(struct _reent *r, const char *path, struct statvfs *buf);
static int       fsdev_ftruncate(struct _reent *r, void *fd, off_t len);
static int       fsdev_fsync(struct _reent *r, void *fd);
static int       fsdev_chmod(struct _reent *r, const char *path, mode_t mode);
static int       fsdev_fchmod(struct _reent *r, void *fd, mode_t mode);
static int       fsdev_rmdir(struct _reent *r, const char *name);

/*! @cond INTERNAL */

/*! Open file struct */
typedef struct
{
  FsFile fd;
  int    flags;  /*! Flags used in open(2) */
  u64    offset; /*! Current file offset */
  FsTimeStampRaw timestamps;
} fsdev_file_t;

/*! fsdev devoptab */
static devoptab_t
fsdev_devoptab =
{
  .structSize   = sizeof(fsdev_file_t),
  .open_r       = fsdev_open,
  .close_r      = fsdev_close,
  .write_r      = fsdev_write,
  .read_r       = fsdev_read,
  .seek_r       = fsdev_seek,
  .fstat_r      = fsdev_fstat,
  .stat_r       = fsdev_stat,
  .link_r       = fsdev_link,
  .unlink_r     = fsdev_unlink,
  .chdir_r      = fsdev_chdir,
  .rename_r     = fsdev_rename,
  .mkdir_r      = fsdev_mkdir,
  .dirStateSize = sizeof(fsdev_dir_t),
  .diropen_r    = fsdev_diropen,
  .dirreset_r   = fsdev_dirreset,
  .dirnext_r    = fsdev_dirnext,
  .dirclose_r   = fsdev_dirclose,
  .statvfs_r    = fsdev_statvfs,
  .ftruncate_r  = fsdev_ftruncate,
  .fsync_r      = fsdev_fsync,
  .deviceData   = 0,
  .chmod_r      = fsdev_chmod,
  .fchmod_r     = fsdev_fchmod,
  .rmdir_r      = fsdev_rmdir,
};

typedef struct
{
    bool setup;
    s32 id;
    devoptab_t device;
    FsFileSystem fs;
    char name[32];
} fsdev_fsdevice;

static bool fsdev_initialised = false;
static s32 fsdev_fsdevice_default = -1;
static s32 fsdev_fsdevice_cwd = -1;
static __thread Result fsdev_last_result = 0;
static fsdev_fsdevice fsdev_fsdevices[32];

/*! @endcond */

static char     __cwd[PATH_MAX+1] = "/";
static __thread char     __fixedpath[PATH_MAX+1];

static fsdev_fsdevice *fsdevFindDevice(const char *name)
{
  u32 i;
  u32 total = sizeof(fsdev_fsdevices) / sizeof(fsdev_fsdevice);
  fsdev_fsdevice *device = NULL;

  if(!fsdev_initialised)
    return NULL;

  if(name && name[0] == '/') //Return the default device.
  {
    if(fsdev_fsdevice_default==-1)
      return NULL;

    device = &fsdev_fsdevices[fsdev_fsdevice_default];
    if(!device->setup)
      device = NULL;

    return device;
  }

  for(i=0; i<total; i++)
  {
    device = &fsdev_fsdevices[i];

    if(name==NULL) //Find an unused device entry.
    {
      if(!device->setup)
        return device;
    }
    else if(device->setup) //Find the device with the input name.
    {
      if(strncmp(device->name, name, strlen(device->name))==0)
        return device;
    }
  }

  return NULL;
}

static const char*
fsdev_fixpath(struct _reent *r,
             const char    *path,
             fsdev_fsdevice **device)
{
  ssize_t       units;
  uint32_t      code;
  const uint8_t *p = (const uint8_t*)path;
  const char *device_path = path;

  // Move the path pointer to the start of the actual path
  do
  {
    units = decode_utf8(&code, p);
    if(units < 0)
    {
      r->_errno = EILSEQ;
      return NULL;
    }

    p += units;
  } while(code != ':' && code != 0);

  // We found a colon; p points to the actual path
  if(code == ':')
    path = (const char*)p;

  // Make sure there are no more colons and that the
  // remainder of the filename is valid UTF-8
  p = (const uint8_t*)path;
  do
  {
    units = decode_utf8(&code, p);
    if(units < 0)
    {
      r->_errno = EILSEQ;
      return NULL;
    }

    if(code == ':')
    {
      r->_errno = EINVAL;
      return NULL;
    }

    p += units;
  } while(code != 0);

  if(path[0] == '/')
    strncpy(__fixedpath, path, PATH_MAX);
  else
  {
    strncpy(__fixedpath, __cwd, PATH_MAX);
    __fixedpath[PATH_MAX] = '\0';
    strncat(__fixedpath, path, PATH_MAX - strlen(__cwd));
  }

  if(__fixedpath[PATH_MAX] != 0)
  {
    __fixedpath[PATH_MAX] = 0;
    r->_errno = ENAMETOOLONG;
    return NULL;
  }

  if(device)
  {
    if(path[0] == '/')
    {
      *device = fsdevFindDevice(device_path);
    }
    else
    {
      *device = NULL;
      if(fsdev_fsdevice_cwd != -1)
        *device = &fsdev_fsdevices[fsdev_fsdevice_cwd];
    }
    if(*device == NULL)
    {
      r->_errno = ENODEV;
      return NULL;
    }
  }

  return __fixedpath;
}

static int
fsdev_getfspath(struct _reent *r,
               const char     *path,
               fsdev_fsdevice **device,
               char           *outpath)
{
  if(fsdev_fixpath(r, path, device) == NULL)
    return -1;

  memcpy(outpath, __fixedpath,FS_MAX_PATH-1);
  outpath[FS_MAX_PATH-1] = '\0';

  return 0;
}

static ssize_t fsdev_convertfromfspath(uint8_t *out, uint8_t *in, size_t len)
{
  strncpy((char*)out, (char*)in, len);
  return strnlen((char*)out, len);
}

extern int __system_argc;
extern char** __system_argv;

static void _fsdevInit(void)
{
  u32 i;
  u32 total = sizeof(fsdev_fsdevices) / sizeof(fsdev_fsdevice);

  if(!fsdev_initialised)
  {
    memset(fsdev_fsdevices, 0, sizeof(fsdev_fsdevices));

    for(i=0; i<total; i++)
    {
      memcpy(&fsdev_fsdevices[i].device, &fsdev_devoptab, sizeof(fsdev_devoptab));
      fsdev_fsdevices[i].device.name = fsdev_fsdevices[i].name;
      fsdev_fsdevices[i].id = i;
    }

    fsdev_fsdevice_default = -1;
    fsdev_initialised = true;
  }
}

static int _fsdevMountDevice(const char *name, FsFileSystem fs, fsdev_fsdevice **out_device)
{
  fsdev_fsdevice *device = NULL;

  _fsdevInit(); //Ensure fsdev is initialized

  if(fsdevFindDevice(name)) //Device is already mounted with the same name.
  {
    fsFsClose(&fs);
    return -1;
  }

  device = fsdevFindDevice(NULL);
  if(device==NULL)
  {
    fsFsClose(&fs);
    return -1;
  }

  device->fs = fs;
  memset(device->name, 0, sizeof(device->name));
  strncpy(device->name, name, sizeof(device->name)-1);

  int dev = AddDevice(&device->device);
  if(dev==-1)
  {
    fsFsClose(&device->fs);
    return dev;
  }

  device->setup = 1;

  if(out_device)
    *out_device = device;

  return dev;
}

int fsdevMountDevice(const char *name, FsFileSystem fs)
{
  return _fsdevMountDevice(name, fs, NULL);
}

static int _fsdevUnmountDeviceStruct(fsdev_fsdevice *device)
{
  char name[34];

  if(!device->setup)
    return 0;

  memset(name, 0, sizeof(name));
  strncpy(name, device->name, sizeof(name)-2);
  strncat(name, ":", sizeof(name)-strlen(name)-1);

  RemoveDevice(name);
  fsFsClose(&device->fs);

  if(device->id == fsdev_fsdevice_default)
    fsdev_fsdevice_default = -1;

  if(device->id == fsdev_fsdevice_cwd)
    fsdev_fsdevice_cwd = fsdev_fsdevice_default;

  device->setup = 0;
  memset(device->name, 0, sizeof(device->name));

  return 0;
}

int fsdevUnmountDevice(const char *name)
{
  fsdev_fsdevice *device;

  device = fsdevFindDevice(name);
  if(device==NULL)
    return -1;

  return _fsdevUnmountDeviceStruct(device);
}

Result fsdevCommitDevice(const char *name)
{
  fsdev_fsdevice *device;

  device = fsdevFindDevice(name);
  if(device==NULL)
    return MAKERESULT(Module_Libnx, LibnxError_NotFound);

  return fsFsCommit(&device->fs);
}

Result fsdevSetArchiveBit(const char *path) {
  char          fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(_REENT, path, &device, fs_path)==-1)
    return MAKERESULT(Module_Libnx, LibnxError_NotFound);

  return fsFsSetArchiveBit(&device->fs, fs_path);
}

Result fsdevCreateFile(const char* path, size_t size, int flags) {
  char          fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(_REENT, path, &device, fs_path)==-1)
    return MAKERESULT(Module_Libnx, LibnxError_NotFound);

  return fsFsCreateFile(&device->fs, fs_path, size, flags);
}

Result fsdevDeleteDirectoryRecursively(const char *path) {
  char          fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(_REENT, path, &device, fs_path)==-1)
    return MAKERESULT(Module_Libnx, LibnxError_NotFound);

  return fsFsDeleteDirectoryRecursively(&device->fs, fs_path);
}

/*! Initialize SDMC device */
Result fsdevMountSdmc(void)
{
  ssize_t  units;
  uint32_t code;
  char     *p;
  Result   rc = 0;
  FsFileSystem fs;
  fsdev_fsdevice *device = NULL;

  if(fsdevFindDevice("sdmc"))
    return 0;

  rc = fsMountSdcard(&fs);
  if(R_SUCCEEDED(rc))
  {
    int dev = _fsdevMountDevice("sdmc", fs, &device);

    if(dev != -1)
    {
      setDefaultDevice(dev);
      if(device)
      {
        fsdev_fsdevice_default = device->id;
        fsdev_fsdevice_cwd = fsdev_fsdevice_default;
      }

      if(__system_argc != 0 && __system_argv[0] != NULL)
      {
        if(FindDevice(__system_argv[0]) == dev)
        {
          strncpy(__fixedpath,__system_argv[0],PATH_MAX);
          if(__fixedpath[PATH_MAX] != 0)
          {
            __fixedpath[PATH_MAX] = 0;
          }
          else
          {
            char *last_slash = NULL;
            p = __fixedpath;
            do
            {
              units = decode_utf8(&code, (const uint8_t*)p);
              if(units < 0)
              {
                last_slash = NULL;
                break;
              }

              if(code == '/')
                last_slash = p;

              p += units;
            } while(code != 0);

            if(last_slash != NULL)
            {
              last_slash[0] = 0;
              chdir(__fixedpath);
            }
          }
        }
      }
    }
  }

  return rc;
}

/*! Clean up fsdev devices */
Result fsdevUnmountAll(void)
{
  u32 i;
  u32 total = sizeof(fsdev_fsdevices) / sizeof(fsdev_fsdevice);
  Result rc=0;

  if(!fsdev_initialised) return rc;

  for(i=0; i<total; i++)
  {
    _fsdevUnmountDeviceStruct(&fsdev_fsdevices[i]);
  }

  fsdev_initialised = false;

  return 0;
}

FsFileSystem* fsdevGetDeviceFileSystem(const char *name)
{
  fsdev_fsdevice *device;

  device = fsdevFindDevice(name);
  if(device==NULL)
    return NULL;

  return &device->fs;
}

FsFileSystem* fsdevGetDefaultFileSystem(void)
{
  if(!fsdev_initialised) return NULL;
  if(fsdev_fsdevice_default==-1) return NULL;

  return &fsdev_fsdevices[fsdev_fsdevice_default].fs;
}

int fsdevTranslatePath(const char *path, FsFileSystem** device, char *outpath)
{
  fsdev_fsdevice *tmpdev = NULL;

  int ret = fsdev_getfspath(_REENT, path, &tmpdev, outpath);
  if(ret==-1)return ret;

  if(device)*device = &tmpdev->fs;
  return ret;
}

/*! Open a file
 *
 *  @param[in,out] r          newlib reentrancy struct
 *  @param[out]    fileStruct Pointer to file struct to fill in
 *  @param[in]     path       Path to open
 *  @param[in]     flags      Open flags from open(2)
 *  @param[in]     mode       Permissions to set on create
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_open(struct _reent *r,
          void          *fileStruct,
          const char    *path,
          int           flags,
          int           mode)
{
  FsFile        fd;
  Result        rc;
  u32           fsdev_flags = 0;
  u32           attributes = 0;
  char          fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(r, path, &device, fs_path)==-1)
    return -1;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fileStruct;

  /* check access mode */
  switch(flags & O_ACCMODE)
  {
    /* read-only: do not allow O_APPEND */
    case O_RDONLY:
      fsdev_flags |= FS_OPEN_READ;
      if(flags & O_APPEND)
      {
        r->_errno = EINVAL;
        return -1;
      }
      break;

    /* write-only */
    case O_WRONLY:
      fsdev_flags |= FS_OPEN_WRITE | FS_OPEN_APPEND;
      break;

    /* read and write */
    case O_RDWR:
      fsdev_flags |= (FS_OPEN_READ | FS_OPEN_WRITE | FS_OPEN_APPEND);
      break;

    /* an invalid option was supplied */
    default:
      r->_errno = EINVAL;
      return -1;
  }

  /* Test O_EXCL. */
  if((flags & O_CREAT))
  {
    rc = fsFsCreateFile(&device->fs, fs_path, 0, attributes);
    if(flags & O_EXCL)
    {
      if(R_FAILED(rc))
      {
        r->_errno = fsdev_translate_error(rc);
        return -1;
      }
    }
  }

  /* set attributes */
  /*if(!(mode & S_IWUSR))
    attributes |= FS_ATTRIBUTE_READONLY;*/

  /* open the file */
  rc = fsFsOpenFile(&device->fs, fs_path, fsdev_flags, &fd);
  if(R_SUCCEEDED(rc))
  {
    if((flags & O_ACCMODE) != O_RDONLY && (flags & O_TRUNC))
    {
      rc = fsFileSetSize(&fd, 0);
      if(R_FAILED(rc))
      {
        fsFileClose(&fd);
        r->_errno = fsdev_translate_error(rc);
        return -1;
      }
    }

    file->fd     = fd;
    file->flags  = (flags & (O_ACCMODE|O_APPEND|O_SYNC));
    file->offset = 0;

    memset(&file->timestamps, 0, sizeof(file->timestamps));
    rc = fsFsGetFileTimeStampRaw(&device->fs, fs_path, &file->timestamps);//Result can be ignored since output is only set on success, etc.

    return 0;
  }

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Close an open file
 *
 *  @param[in,out] r  newlib reentrancy struct
 *  @param[in]     fd Pointer to fsdev_file_t
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_close(struct _reent *r,
           void          *fd)
{
  Result      rc=0;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  fsFileClose(&file->fd);
  if(R_SUCCEEDED(rc))
    return 0;

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Write to an open file
 *
 *  @param[in,out] r   newlib reentrancy struct
 *  @param[in,out] fd  Pointer to fsdev_file_t
 *  @param[in]     ptr Pointer to data to write
 *  @param[in]     len Length of data to write
 *
 *  @returns number of bytes written
 *  @returns -1 for error
 */
static ssize_t
fsdev_write(struct _reent *r,
           void          *fd,
           const char    *ptr,
           size_t        len)
{
  Result      rc;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  /* check that the file was opened with write access */
  if((file->flags & O_ACCMODE) == O_RDONLY)
  {
    r->_errno = EBADF;
    return -1;
  }

  if(file->flags & O_APPEND)
  {
    /* append means write from the end of the file */
    rc = fsFileGetSize(&file->fd, &file->offset);
    if(R_FAILED(rc))
    {
      r->_errno = fsdev_translate_error(rc);
      return -1;
    }
  }

  rc = fsFileWrite(&file->fd, file->offset, ptr, len, FS_WRITEOPTION_NONE);
  if(rc == 0xD401)
    return fsdev_write_safe(r, fd, ptr, len);
  if(R_FAILED(rc))
  {
    r->_errno = fsdev_translate_error(rc);
    return -1;
  }

  file->offset += len;

  /* check if this is synchronous or not */
  if(file->flags & O_SYNC)
    fsFileFlush(&file->fd);

  return len;
}

/*! Write to an open file
 *
 *  @param[in,out] r   newlib reentrancy struct
 *  @param[in,out] fd  Pointer to fsdev_file_t
 *  @param[in]     ptr Pointer to data to write
 *  @param[in]     len Length of data to write
 *
 *  @returns number of bytes written
 *  @returns -1 for error
 */
static ssize_t
fsdev_write_safe(struct _reent *r,
                void          *fd,
                const char    *ptr,
                size_t        len)
{
  Result      rc;
  size_t      bytesWritten = 0;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  /* Copy to internal buffer and transfer in chunks.
   * You cannot use FS read/write with certain memory.
   */
  static __thread char tmp_buffer[8192];
  while(len > 0)
  {
    size_t toWrite = len;
    if(toWrite > sizeof(tmp_buffer))
      toWrite = sizeof(tmp_buffer);

    /* copy to internal buffer */
    memcpy(tmp_buffer, ptr, toWrite);

    /* write the data */
    rc = fsFileWrite(&file->fd, file->offset, tmp_buffer, toWrite, FS_WRITEOPTION_NONE);

    if(R_FAILED(rc))
    {
      /* return partial transfer */
      if(bytesWritten > 0)
        return bytesWritten;

      r->_errno = fsdev_translate_error(rc);
      return -1;
    }

    /* check if this is synchronous or not */
    if(file->flags & O_SYNC)
      fsFileFlush(&file->fd);

    file->offset += toWrite;
    bytesWritten += toWrite;
    ptr          += toWrite;
    len          -= toWrite;
  }

  return bytesWritten;
}

/*! Read from an open file
 *
 *  @param[in,out] r   newlib reentrancy struct
 *  @param[in,out] fd  Pointer to fsdev_file_t
 *  @param[out]    ptr Pointer to buffer to read into
 *  @param[in]     len Length of data to read
 *
 *  @returns number of bytes read
 *  @returns -1 for error
 */
static ssize_t
fsdev_read(struct _reent *r,
          void          *fd,
          char          *ptr,
          size_t         len)
{
  Result      rc;
  size_t      bytes;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  /* check that the file was opened with read access */
  if((file->flags & O_ACCMODE) == O_WRONLY)
  {
    r->_errno = EBADF;
    return -1;
  }

  /* read the data */
  rc = fsFileRead(&file->fd, file->offset, ptr, len, FS_READOPTION_NONE, &bytes);
  if(rc == 0xD401)
    return fsdev_read_safe(r, fd, ptr, len);
  if(R_SUCCEEDED(rc))
  {
    /* update current file offset */
    file->offset += bytes;
    return (ssize_t)bytes;
  }

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Read from an open file
 *
 *  @param[in,out] r   newlib reentrancy struct
 *  @param[in,out] fd  Pointer to fsdev_file_t
 *  @param[out]    ptr Pointer to buffer to read into
 *  @param[in]     len Length of data to read
 *
 *  @returns number of bytes written
 *  @returns -1 for error
 */
static ssize_t
fsdev_read_safe(struct _reent *r,
                void          *fd,
                char          *ptr,
                size_t        len)
{
  Result      rc;
  size_t      bytesRead = 0, bytes = 0;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  /* Transfer in chunks with internal buffer.
   * You cannot use FS read/write with certain memory.
   */
  static __thread char tmp_buffer[8192];
  while(len > 0)
  {
    size_t toRead = len;
    if(toRead > sizeof(tmp_buffer))
      toRead = sizeof(tmp_buffer);

    /* read the data */
    rc = fsFileRead(&file->fd, file->offset, tmp_buffer, toRead, FS_READOPTION_NONE, &bytes);

    if(bytes > toRead)
      bytes = toRead;

    /* copy from internal buffer */
    memcpy(ptr, tmp_buffer, bytes);

    if(R_FAILED(rc))
    {
      /* return partial transfer */
      if(bytesRead > 0)
        return bytesRead;

      r->_errno = fsdev_translate_error(rc);
      return -1;
    }

    file->offset += bytes;
    bytesRead    += bytes;
    ptr          += bytes;
    len          -= bytes;
  }

  return bytesRead;
}

/*! Update an open file's current offset
 *
 *  @param[in,out] r      newlib reentrancy struct
 *  @param[in,out] fd     Pointer to fsdev_file_t
 *  @param[in]     pos    Offset to seek to
 *  @param[in]     whence Where to seek from
 *
 *  @returns new offset for success
 *  @returns -1 for error
 */
static off_t
fsdev_seek(struct _reent *r,
          void          *fd,
          off_t         pos,
          int           whence)
{
  Result      rc;
  u64         offset;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  /* find the offset to see from */
  switch(whence)
  {
    /* set absolute position; start offset is 0 */
    case SEEK_SET:
      offset = 0;
      break;

    /* set position relative to the current position */
    case SEEK_CUR:
      offset = file->offset;
      break;

    /* set position relative to the end of the file */
    case SEEK_END:
      rc = fsFileGetSize(&file->fd, &offset);
      if(R_FAILED(rc))
      {
        r->_errno = fsdev_translate_error(rc);
        return -1;
      }
      break;

    /* an invalid option was provided */
    default:
      r->_errno = EINVAL;
      return -1;
  }

  /* TODO: A better check that prevents overflow. */
  if(pos < 0 && offset < -pos)
  {
    /* don't allow seek to before the beginning of the file */
    r->_errno = EINVAL;
    return -1;
  }

  /* update the current offset */
  file->offset = offset + pos;
  return file->offset;
}

/*! Get file stats from an open file
 *
 *  @param[in,out] r  newlib reentrancy struct
 *  @param[in]     fd Pointer to fsdev_file_t
 *  @param[out]    st Pointer to file stats to fill
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_fstat(struct _reent *r,
           void          *fd,
           struct stat   *st)
{
  Result      rc;
  u64         size;
  fsdev_file_t *file = (fsdev_file_t*)fd;

  rc = fsFileGetSize(&file->fd, &size);
  if(R_SUCCEEDED(rc))
  {
    memset(st, 0, sizeof(struct stat));
    st->st_size = (off_t)size;
    st->st_nlink = 1;
    st->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    if(file->timestamps.is_valid)
    {
      st->st_ctime = file->timestamps.created;
      st->st_mtime = file->timestamps.modified;
      st->st_atime = file->timestamps.accessed;
    }

    return 0;
  }

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Get file stats
 *
 *  @param[in,out] r    newlib reentrancy struct
 *  @param[in]     file Path to file
 *  @param[out]    st   Pointer to file stats to fill
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_stat(struct _reent *r,
          const char    *file,
          struct stat   *st)
{
  FsFile  fd;
  FsDir   fdir;
  Result  rc;
  int     ret=0;
  char    fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;
  FsTimeStampRaw timestamps = {0};
  FsEntryType type;

  if(fsdev_getfspath(r, file, &device, fs_path)==-1)
    return -1;

  rc = fsFsGetEntryType(&device->fs, fs_path, &type);
  if(R_SUCCEEDED(rc))
  {
    if(type == ENTRYTYPE_DIR)
    {
      if(R_SUCCEEDED(rc = fsFsOpenDirectory(&device->fs, fs_path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &fdir)))
      {
        memset(st, 0, sizeof(struct stat));
        st->st_nlink = 1;
        st->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        fsDirClose(&fdir);
        return 0;
      }
    }
    else if(type == ENTRYTYPE_FILE)
    {
      if(R_SUCCEEDED(rc = fsFsOpenFile(&device->fs, fs_path, FS_OPEN_READ, &fd)))
      {
        fsdev_file_t tmpfd = { .fd = fd };
        ret = fsdev_fstat(r, &tmpfd, st);
        fsFileClose(&fd);

        if(ret==0)
        {
          rc = fsFsGetFileTimeStampRaw(&device->fs, fs_path, &timestamps);
          if(R_SUCCEEDED(rc) && timestamps.is_valid)
          {
            st->st_ctime = timestamps.created;
            st->st_mtime = timestamps.modified;
            st->st_atime = timestamps.accessed;
          }
        }

        return ret;
      }
    }
    else
    {
      r->_errno = EINVAL;
      return -1;
    }
  }

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Hard link a file
 *
 *  @param[in,out] r        newlib reentrancy struct
 *  @param[in]     existing Path of file to link
 *  @param[in]     newLink  Path of new link
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_link(struct _reent *r,
          const char    *existing,
          const char    *newLink)
{
  r->_errno = ENOSYS;
  return -1;
}

/*! Unlink a file
 *
 *  @param[in,out] r    newlib reentrancy struct
 *  @param[in]     name Path of file to unlink
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_unlink(struct _reent *r,
            const char    *name)
{
  Result  rc;
  char    fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(r, name, &device, fs_path)==-1)
    return -1;

  rc = fsFsDeleteFile(&device->fs, fs_path);
  if(R_SUCCEEDED(rc))
    return 0;

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Change current working directory
 *
 *  @param[in,out] r    newlib reentrancy struct
 *  @param[in]     name Path to new working directory
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_chdir(struct _reent *r,
           const char    *name)
{
  FsDir   fd;
  Result  rc;
  char    fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(r, name, &device, fs_path)==-1)
    return -1;

  rc = fsFsOpenDirectory(&device->fs, fs_path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &fd);
  if(R_SUCCEEDED(rc))
  {
    fsDirClose(&fd);
    strncpy(__cwd, fs_path, PATH_MAX);
    __cwd[PATH_MAX] = '\0';

    size_t __cwd_len = strlen(__cwd);

    if (__cwd[__cwd_len-1] != '/' && __cwd_len < PATH_MAX)
    {
      __cwd[__cwd_len] = '/';
      __cwd[__cwd_len+1] = '\0';
    }

    fsdev_fsdevice_cwd = device->id;
    return 0;
  }

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Rename a file
 *
 *  @param[in,out] r       newlib reentrancy struct
 *  @param[in]     oldName Path to rename from
 *  @param[in]     newName Path to rename to
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_rename(struct _reent *r,
            const char    *oldName,
            const char    *newName)
{
  Result  rc;
  FsEntryType type;
  fsdev_fsdevice *device_old = NULL, *device_new = NULL;
  char fs_path_old[FS_MAX_PATH];
  char fs_path_new[FS_MAX_PATH];

  if(fsdev_getfspath(r, oldName, &device_old, fs_path_old)==-1)
    return -1;

  if(fsdev_getfspath(r, newName, &device_new, fs_path_new)==-1)
    return -1;

  if(device_old->id != device_new->id)
  {
    r->_errno = EXDEV;
    return -1;
  }

  rc = fsFsGetEntryType(&device_old->fs, fs_path_old, &type);
  if(R_SUCCEEDED(rc))
  {
    if(type == ENTRYTYPE_DIR)
    {
      rc = fsFsRenameDirectory(&device_old->fs, fs_path_old, fs_path_new);
      if(R_SUCCEEDED(rc))
      return 0;
    }
    else if(type == ENTRYTYPE_FILE)
    {
      rc = fsFsRenameFile(&device_old->fs, fs_path_old, fs_path_new);
      if(R_SUCCEEDED(rc))
      return 0;
    }
    else
    {
      r->_errno = EINVAL;
      return -1;
    }
  }

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Create a directory
 *
 *  @param[in,out] r    newlib reentrancy struct
 *  @param[in]     path Path of directory to create
 *  @param[in]     mode Permissions of created directory
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_mkdir(struct _reent *r,
           const char    *path,
           int           mode)
{
  Result  rc;
  char    fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(r, path, &device, fs_path)==-1)
    return -1;

  rc = fsFsCreateDirectory(&device->fs, fs_path);
  if(R_SUCCEEDED(rc))
    return 0;

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Open a directory
 *
 *  @param[in,out] r        newlib reentrancy struct
 *  @param[in]     dirState Pointer to open directory state
 *  @param[in]     path     Path of directory to open
 *
 *  @returns dirState for success
 *  @returns NULL for error
 */
static DIR_ITER*
fsdev_diropen(struct _reent *r,
             DIR_ITER      *dirState,
             const char    *path)
{
  FsDir   fd;
  Result  rc;
  char    fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(r, path, &device, fs_path)==-1)
    return NULL;

  /* get pointer to our data */
  fsdev_dir_t *dir = (fsdev_dir_t*)(dirState->dirStruct);

  /* open the directory */
  rc = fsFsOpenDirectory(&device->fs, fs_path, FS_DIROPEN_DIRECTORY | FS_DIROPEN_FILE, &fd);
  if(R_SUCCEEDED(rc))
  {
    dir->magic = FSDEV_DIRITER_MAGIC;
    dir->fd    = fd;
    dir->index = -1;
    dir->size  = 0;
    memset(&dir->entry_data, 0, sizeof(dir->entry_data));
    return dirState;
  }

  r->_errno = fsdev_translate_error(rc);
  return NULL;
}

/*! Reset an open directory to its intial state
 *
 *  @param[in,out] r        newlib reentrancy struct
 *  @param[in]     dirState Pointer to open directory state
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_dirreset(struct _reent *r,
              DIR_ITER      *dirState)
{
  r->_errno = ENOSYS;
  return -1;
}

/*! Fetch the next entry of an open directory
 *
 *  @param[in,out] r        newlib reentrancy struct
 *  @param[in]     dirState Pointer to open directory state
 *  @param[out]    filename Buffer to store entry name
 *  @param[out]    filestat Buffer to store entry attributes
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_dirnext(struct _reent *r,
             DIR_ITER      *dirState,
             char          *filename,
             struct stat   *filestat)
{
  Result              rc;
  size_t              entries;
  ssize_t             units;
  FsDirectoryEntry   *entry;

  /* get pointer to our data */
  fsdev_dir_t *dir = (fsdev_dir_t*)(dirState->dirStruct);

  static const size_t max_entries = sizeof(dir->entry_data) / sizeof(dir->entry_data[0]);

  /* check if it's in the batch already */
  if(++dir->index < dir->size)
  {
    rc = 0;
  }
  else
  {
    /* reset batch info */
    dir->index = -1;
    dir->size  = 0;

    /* fetch the next batch */
    memset(dir->entry_data, 0, sizeof(dir->entry_data));
    rc = fsDirRead(&dir->fd, 0, &entries, max_entries, dir->entry_data);
    if(R_SUCCEEDED(rc))
    {
      if(entries == 0)
      {
        /* there are no more entries; ENOENT signals end-of-directory */
        r->_errno = ENOENT;
        return -1;
      }

      dir->index = 0;
      dir->size  = entries;
    }
  }

  if(R_SUCCEEDED(rc))
  {
    entry = &dir->entry_data[dir->index];

    /* fill in the stat info */
    filestat->st_ino = 0;
    if(entry->type == ENTRYTYPE_DIR)
      filestat->st_mode = S_IFDIR;
    else if(entry->type == ENTRYTYPE_FILE)
    {
      filestat->st_mode = S_IFREG;
      filestat->st_size = entry->fileSize;
    }
    else
    {
      r->_errno = EINVAL;
      return -1;
    }

    /* convert name from fs-path to UTF-8 */
    memset(filename, 0, NAME_MAX);
    units = fsdev_convertfromfspath((uint8_t*)filename, (uint8_t*)entry->name, NAME_MAX);
    if(units < 0)
    {
      r->_errno = EILSEQ;
      return -1;
    }

    if(units >= NAME_MAX)
    {
      r->_errno = ENAMETOOLONG;
      return -1;
    }

    return 0;
  }

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Close an open directory
 *
 *  @param[in,out] r        newlib reentrancy struct
 *  @param[in]     dirState Pointer to open directory state
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_dirclose(struct _reent *r,
              DIR_ITER      *dirState)
{
  Result         rc=0;

  /* get pointer to our data */
  fsdev_dir_t *dir = (fsdev_dir_t*)(dirState->dirStruct);

  /* close the directory */
  fsDirClose(&dir->fd);
  if(R_SUCCEEDED(rc))
    return 0;

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Get filesystem statistics
 *
 *  @param[in,out] r    newlib reentrancy struct
 *  @param[in]     path Path to filesystem to get statistics of
 *  @param[out]    buf  Buffer to fill
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_statvfs(struct _reent  *r,
             const char     *path,
             struct statvfs *buf)
{
  Result rc=0;
  char    fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;
  u64 freespace = 0, total_space = 0;

  if(fsdev_getfspath(r, path, &device, fs_path)==-1)
    return -1;

  rc = fsFsGetFreeSpace(&device->fs, fs_path, &freespace);

  if(R_SUCCEEDED(rc))
    rc = fsFsGetTotalSpace(&device->fs, fs_path, &total_space);

  if(R_SUCCEEDED(rc))
  {
    buf->f_bsize   = 1;
    buf->f_frsize  = 1;
    buf->f_blocks  = total_space;
    buf->f_bfree   = freespace;
    buf->f_bavail  = freespace;
    buf->f_files   = 0;
    buf->f_ffree   = 0;
    buf->f_favail  = 0;
    buf->f_fsid    = 0;
    buf->f_flag    = ST_NOSUID;
    buf->f_namemax = 0;

    return 0;
  }

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Truncate an open file
 *
 *  @param[in,out] r   newlib reentrancy struct
 *  @param[in]     fd  Pointer to fsdev_file_t
 *  @param[in]     len Length to truncate file to
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_ftruncate(struct _reent *r,
               void          *fd,
               off_t         len)
{
  Result      rc;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  /* make sure length is non-negative */
  if(len < 0)
  {
    r->_errno = EINVAL;
    return -1;
  }

  /* set the new file size */
  rc = fsFileSetSize(&file->fd, len);
  if(R_SUCCEEDED(rc))
    return 0;

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Synchronize a file to media
 *
 *  @param[in,out] r  newlib reentrancy struct
 *  @param[in]     fd Pointer to fsdev_file_t
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_fsync(struct _reent *r,
           void          *fd)
{
  Result rc;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  rc = fsFileFlush(&file->fd);
  if(R_SUCCEEDED(rc))
    return 0;

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

/*! Change a file's mode
 *
 *  @param[in,out] r    newlib reentrancy struct
 *  @param[in]     path Path to file to update
 *  @param[in]     mode New mode to set
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_chmod(struct _reent *r,
          const char    *path,
          mode_t        mode)
{
  r->_errno = ENOSYS;
  return -1;
}

/*! Change an open file's mode
 *
 *  @param[in,out] r    newlib reentrancy struct
 *  @param[in]     fd   Pointer to fsdev_file_t
 *  @param[in]     mode New mode to set
 *
 *  @returns 0 for success
 *  @returns -1 for failure
 */
static int
fsdev_fchmod(struct _reent *r,
            void          *fd,
            mode_t        mode)
{
  r->_errno = ENOSYS;
  return -1;
}

/*! Remove a directory
 *
 *  @param[in,out] r    newlib reentrancy struct
 *  @param[in]     name Path of directory to remove
 *
 *  @returns 0 for success
 *  @returns -1 for error
 */
static int
fsdev_rmdir(struct _reent *r,
           const char    *name)
{
  Result  rc;
  char    fs_path[FS_MAX_PATH];
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(r, name, &device, fs_path)==-1)
    return -1;

  rc = fsFsDeleteDirectory(&device->fs, fs_path);
  if(R_SUCCEEDED(rc))
    return 0;

  r->_errno = fsdev_translate_error(rc);
  return -1;
}

Result
fsdev_getmtime(const char *name,
              u64        *mtime)
{
  //Result        rc;
  struct _reent r;

  r._errno = ENOSYS;

  if(r._errno != 0)
    errno = r._errno;

  return -1;

  /*r._errno = 0;

  fs_path = fsdev_getfspath(&r, name);
  if(r._errno != 0)
    errno = r._errno;

  if(fs_path.data == NULL)
    return -1;*/

  /*if(rc == 0)
  {*/
    /* convert from milliseconds to seconds */
    //*mtime /= 1000;
    /* convert from 2000-based timestamp to UNIX timestamp */
    /**mtime += 946684800;
  }

  return rc;*/

}

/*! Error map */
typedef struct
{
  Result fs_error; //!< Error from FS service
  int    error;    //!< POSIX errno
} error_map_t;

/*! Error table */
static const error_map_t error_table[] =
{
  /* keep this list sorted! */
  { 0x202, ENOENT,          },
  { 0x402, EEXIST,          },
  { 0x2EE202, EINVAL,       },
  { 0x2EE602, ENAMETOOLONG, },
};
static const size_t num_errors = sizeof(error_table)/sizeof(error_table[0]);

/*! Comparison function for bsearch on error_table
 *
 *  @param[in] p1 Left side of comparison
 *  @param[in] p2 Right side of comparison
 *
 *  @returns <0 if lhs < rhs
 *  @returns >0 if lhs > rhs
 *  @returns 0  if lhs == rhs
 */
static int
error_cmp(const void *p1, const void *p2)
{
  const error_map_t *lhs = (const error_map_t*)p1;
  const error_map_t *rhs = (const error_map_t*)p2;

  if((u32)lhs->fs_error < (u32)rhs->fs_error)
    return -1;
  else if((u32)lhs->fs_error > (u32)rhs->fs_error)
    return 1;
  return 0;
}

/*! Translate FS service error to errno
 *
 *  @param[in] error FS service error
 *
 *  @returns errno
 */
static int
fsdev_translate_error(Result error)
{
  fsdev_last_result = error;
  error_map_t key = { .fs_error = error };
  const error_map_t *rc = bsearch(&key, error_table, num_errors,
                                  sizeof(error_map_t), error_cmp);

  if(rc != NULL)
    return rc->error;

  return EIO;
}

/*! Getter for last error code translated to errno by fsdev library.
 *
 *  @returns result
 */
Result fsdevGetLastResult(void) {
    return fsdev_last_result;
}

