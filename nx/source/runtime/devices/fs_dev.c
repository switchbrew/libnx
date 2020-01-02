#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <unistd.h>
#include <time.h>

#include "runtime/devices/fs_dev.h"
#include "runtime/util/utf.h"
#include "runtime/env.h"
#include "services/time.h"

#include "path_buf.h"

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
  s64    offset; /*! Current file offset */
  FsTimeStampRaw timestamps;
} fsdev_file_t;

/*! fsdev devoptab */
static const devoptab_t
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
  char *cwd;
  char name[32];
} fsdev_fsdevice;

static bool fsdev_initialised = false;
static s32 fsdev_fsdevice_cwd;
static __thread Result fsdev_last_result = 0;
static fsdev_fsdevice fsdev_fsdevices[32];

/*! @endcond */

_Static_assert((PATH_MAX+1) >= FS_MAX_PATH, "PATH_MAX is too small");

__attribute__((weak)) u32 __nx_fsdev_direntry_cache_size = 32;
__attribute__((weak)) bool __nx_fsdev_support_cwd = true;

static fsdev_fsdevice *fsdevFindDevice(const char *name)
{
  u32 i;
  u32 total = sizeof(fsdev_fsdevices) / sizeof(fsdev_fsdevice);
  fsdev_fsdevice *device = NULL;

  if(!fsdev_initialised)
    return NULL;

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
      size_t devnamelen = strlen(device->name);
      if(strncmp(device->name, name, devnamelen)==0 && (name[devnamelen]=='\0' || name[devnamelen]==':'))
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

  fsdev_fsdevice *dev = NULL;
  if(device && *device != NULL)
    dev = *device;
  else if(path != device_path)
    dev = fsdevFindDevice(device_path);
  else if(fsdev_fsdevice_cwd != -1)
    dev = &fsdev_fsdevices[fsdev_fsdevice_cwd];
  if(dev == NULL)
  {
    r->_errno = ENODEV;
    return NULL;
  }

  if(path[0] == '/')
    strncpy(__nx_dev_path_buf, path, PATH_MAX);
  else
  {
    const char* cwd = dev->cwd ? dev->cwd : "/";
    strncpy(__nx_dev_path_buf, cwd, PATH_MAX);
    __nx_dev_path_buf[PATH_MAX] = '\0';
    strncat(__nx_dev_path_buf, path, PATH_MAX - strlen(cwd));
  }

  if(__nx_dev_path_buf[PATH_MAX] != 0)
  {
    __nx_dev_path_buf[PATH_MAX] = 0;
    r->_errno = ENAMETOOLONG;
    return NULL;
  }

  if(device)
    *device = dev;

  return __nx_dev_path_buf;
}

static int
fsdev_getfspath(struct _reent *r,
               const char     *path,
               fsdev_fsdevice **device,
               char           *outpath)
{
  if(fsdev_fixpath(r, path, device) == NULL)
    return -1;

  if(outpath != __nx_dev_path_buf)
    memcpy(outpath, __nx_dev_path_buf, FS_MAX_PATH-1);
  outpath[FS_MAX_PATH-1] = '\0';

  return 0;
}

static ssize_t fsdev_convertfromfspath(uint8_t *out, uint8_t *in, size_t len)
{
  ssize_t inlen = strnlen((char*)in, len);
  memcpy(out, in, inlen);
  if (inlen < len)
    out[inlen+1] = 0;
  return inlen;
}

static time_t fsdev_converttimetoutc(u64 timestamp)
{
  // Parse timestamp into y/m/d h:m:s
  time_t posixtime = (time_t)timestamp;
  struct tm *t = gmtime(&posixtime);

  // Convert time/date into an actual UTC POSIX timestamp using the system's timezone rules
  TimeCalendarTime caltime;
  caltime.year   = 1900 + t->tm_year;
  caltime.month  = 1 + t->tm_mon;
  caltime.day    = t->tm_mday;
  caltime.hour   = t->tm_hour;
  caltime.minute = t->tm_min;
  caltime.second = t->tm_sec;
  u64 new_timestamp;
  Result rc = timeToPosixTimeWithMyRule(&caltime, &new_timestamp, 1, NULL);
  if (R_SUCCEEDED(rc))
    posixtime = (time_t)new_timestamp;

  return posixtime;
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
      fsdev_fsdevices[i].device.dirStateSize += sizeof(FsDirectoryEntry)*__nx_fsdev_direntry_cache_size;
      fsdev_fsdevices[i].device.deviceData = &fsdev_fsdevices[i];
      fsdev_fsdevices[i].id = i;
    }

    fsdev_fsdevice_cwd = -1;
    fsdev_initialised = true;
  }
}

static int _fsdevMountDevice(const char *name, FsFileSystem fs, fsdev_fsdevice **out_device)
{
  fsdev_fsdevice *device = NULL;

  if(fsdevFindDevice(name)) //Device is already mounted with the same name.
    goto _fail;

  _fsdevInit(); //Ensure fsdev is initialized

  device = fsdevFindDevice(NULL);
  if(device==NULL)
    goto _fail;

  device->fs = fs;
  memset(device->name, 0, sizeof(device->name));
  strncpy(device->name, name, sizeof(device->name)-1);

  int dev = AddDevice(&device->device);
  if(dev==-1)
    goto _fail;

  device->setup = 1;
  device->cwd = __nx_fsdev_support_cwd ? malloc(FS_MAX_PATH) : NULL;
  if(device->cwd!=NULL)
  {
    device->cwd[0] = '/';
    device->cwd[1] = '\0';
  }

  if(fsdev_fsdevice_cwd==-1)
    fsdev_fsdevice_cwd = device->id;

  const devoptab_t *default_dev = GetDeviceOpTab("");
  if(default_dev==NULL || strcmp(default_dev->name, "stdnull")==0)
    setDefaultDevice(dev);

  if(out_device)
    *out_device = device;

  return dev;

_fail:
  fsFsClose(&fs);
  return -1;
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
  free(device->cwd);
  fsFsClose(&device->fs);

  if(device->id == fsdev_fsdevice_cwd)
    fsdev_fsdevice_cwd = -1;

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

Result fsdevSetConcatenationFileAttribute(const char *path) {
  char           *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(_REENT, path, &device, fs_path)==-1)
    return MAKERESULT(Module_Libnx, LibnxError_NotFound);

  return fsFsSetConcatenationFileAttribute(&device->fs, fs_path);
}

Result fsdevIsValidSignedSystemPartitionOnSdCard(const char *name, bool *out) {
  fsdev_fsdevice *device;

  device = fsdevFindDevice(name);
  if(device==NULL)
    return MAKERESULT(Module_Libnx, LibnxError_NotFound);

  return fsFsIsValidSignedSystemPartitionOnSdCard(&device->fs, out);
}

Result fsdevCreateFile(const char* path, size_t size, u32 flags) {
  char           *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(_REENT, path, &device, fs_path)==-1)
    return MAKERESULT(Module_Libnx, LibnxError_NotFound);

  return fsFsCreateFile(&device->fs, fs_path, size, flags);
}

Result fsdevDeleteDirectoryRecursively(const char *path) {
  char           *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = NULL;

  if(fsdev_getfspath(_REENT, path, &device, fs_path)==-1)
    return MAKERESULT(Module_Libnx, LibnxError_NotFound);

  return fsFsDeleteDirectoryRecursively(&device->fs, fs_path);
}

/*! Initialize SDMC device */
Result fsdevMountSdmc(void)
{
  FsFileSystem fs;
  Result rc = fsOpenSdCardFileSystem(&fs);
  if(R_SUCCEEDED(rc))
  {
    int ret = fsdevMountDevice("sdmc", fs);
    if(ret==-1)
      rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
  }

  return rc;
}

Result fsdevMountSaveData(const char *name, u64 application_id, AccountUid uid)
{
  FsFileSystem fs;
  Result rc = fsOpen_SaveData(&fs, application_id, uid);
  if(R_SUCCEEDED(rc))
  {
    int ret = fsdevMountDevice(name, fs);
    if(ret==-1)
      rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
  }
  return rc;
}

Result fsdevMountSystemSaveData(const char *name, FsSaveDataSpaceId save_data_space_id, u64 system_save_data_id, AccountUid uid)
{
  FsFileSystem fs;
  Result rc = fsOpen_SystemSaveData(&fs, save_data_space_id, system_save_data_id, uid);
  if(R_SUCCEEDED(rc))
  {
    int ret = fsdevMountDevice(name, fs);
    if(ret==-1)
      rc = MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);
  }
  return rc;
}

void __libnx_init_cwd(void)
{
  if(envIsNso() || __system_argc==0 || __system_argv[0] == NULL)
    return;

  char *last_slash = NULL;
  char *p = __system_argv[0];
  uint32_t code;
  do
  {
    ssize_t units = decode_utf8(&code, (const uint8_t*)p);
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
    chdir(__system_argv[0]);
    last_slash[0] = '/';
  }
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
  char         *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = r->deviceData;

  if(fsdev_getfspath(r, path, &device, fs_path)==-1)
    return -1;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fileStruct;

  /* check access mode */
  switch(flags & O_ACCMODE)
  {
    /* read-only: do not allow O_APPEND */
    case O_RDONLY:
      fsdev_flags |= FsOpenMode_Read;
      if(flags & O_APPEND)
      {
        r->_errno = EINVAL;
        return -1;
      }
      break;

    /* write-only */
    case O_WRONLY:
      fsdev_flags |= FsOpenMode_Write | FsOpenMode_Append;
      break;

    /* read and write */
    case O_RDWR:
      fsdev_flags |= (FsOpenMode_Read | FsOpenMode_Write | FsOpenMode_Append);
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

  rc = fsFileWrite(&file->fd, file->offset, ptr, len, FsWriteOption_None);
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
  char tmp_buffer[0x1000];
  while(len > 0)
  {
    size_t toWrite = len;
    if(toWrite > sizeof(tmp_buffer))
      toWrite = sizeof(tmp_buffer);

    /* copy to internal buffer */
    memcpy(tmp_buffer, ptr, toWrite);

    /* write the data */
    rc = fsFileWrite(&file->fd, file->offset, tmp_buffer, toWrite, FsWriteOption_None);

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
  u64         bytes;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  /* check that the file was opened with read access */
  if((file->flags & O_ACCMODE) == O_WRONLY)
  {
    r->_errno = EBADF;
    return -1;
  }

  /* read the data */
  rc = fsFileRead(&file->fd, file->offset, ptr, len, FsReadOption_None, &bytes);
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
  u64         bytesRead = 0, bytes = 0;

  /* get pointer to our data */
  fsdev_file_t *file = (fsdev_file_t*)fd;

  /* Transfer in chunks with internal buffer.
   * You cannot use FS read/write with certain memory.
   */
  char tmp_buffer[0x1000];
  while(len > 0)
  {
    u64 toRead = len;
    if(toRead > sizeof(tmp_buffer))
      toRead = sizeof(tmp_buffer);

    /* read the data */
    rc = fsFileRead(&file->fd, file->offset, tmp_buffer, toRead, FsReadOption_None, &bytes);

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
  s64         offset;

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
  s64         size;
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
      st->st_ctime = fsdev_converttimetoutc(file->timestamps.created);
      st->st_mtime = fsdev_converttimetoutc(file->timestamps.modified);
      st->st_atime = fsdev_converttimetoutc(file->timestamps.accessed);
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
  char   *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = r->deviceData;
  FsTimeStampRaw timestamps = {0};
  FsDirEntryType type;

  if(fsdev_getfspath(r, file, &device, fs_path)==-1)
    return -1;

  rc = fsFsGetEntryType(&device->fs, fs_path, &type);
  if(R_SUCCEEDED(rc))
  {
    if(type == FsDirEntryType_Dir)
    {
      if(R_SUCCEEDED(rc = fsFsOpenDirectory(&device->fs, fs_path, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &fdir)))
      {
        memset(st, 0, sizeof(struct stat));
        st->st_nlink = 1;
        st->st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        fsDirClose(&fdir);
        return 0;
      }
    }
    else if(type == FsDirEntryType_File)
    {
      if(R_SUCCEEDED(rc = fsFsOpenFile(&device->fs, fs_path, FsOpenMode_Read, &fd)))
      {
        fsdev_file_t tmpfd = { .fd = fd };
        ret = fsdev_fstat(r, &tmpfd, st);
        fsFileClose(&fd);

        if(ret==0)
        {
          rc = fsFsGetFileTimeStampRaw(&device->fs, fs_path, &timestamps);
          if(R_SUCCEEDED(rc) && timestamps.is_valid)
          {
            st->st_ctime = fsdev_converttimetoutc(timestamps.created);
            st->st_mtime = fsdev_converttimetoutc(timestamps.modified);
            st->st_atime = fsdev_converttimetoutc(timestamps.accessed);
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
  char   *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = r->deviceData;

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
  char   *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = r->deviceData;

  if(device->cwd==NULL)
  {
    r->_errno = ENOSYS;
    return -1;
  }

  if(fsdev_getfspath(r, name, &device, fs_path)==-1)
    return -1;

  rc = fsFsOpenDirectory(&device->fs, fs_path, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &fd);
  if(R_SUCCEEDED(rc))
  {
    fsDirClose(&fd);
    memcpy(device->cwd, fs_path, FS_MAX_PATH);

    size_t cwdlen = strlen(fs_path);
    if (device->cwd[cwdlen-1] != '/' && cwdlen < FS_MAX_PATH-1)
    {
      device->cwd[cwdlen] = '/';
      device->cwd[cwdlen+1] = '\0';
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
  FsDirEntryType type;
  fsdev_fsdevice *device = r->deviceData;
  char fs_path_old[FS_MAX_PATH];
  char*fs_path_new = __nx_dev_path_buf;

  if(fsdev_getfspath(r, oldName, &device, fs_path_old)==-1)
    return -1;

  if(fsdev_getfspath(r, newName, &device, fs_path_new)==-1)
    return -1;

  rc = fsFsGetEntryType(&device->fs, fs_path_old, &type);
  if(R_SUCCEEDED(rc))
  {
    if(type == FsDirEntryType_Dir)
    {
      rc = fsFsRenameDirectory(&device->fs, fs_path_old, fs_path_new);
      if(R_SUCCEEDED(rc))
      return 0;
    }
    else if(type == FsDirEntryType_File)
    {
      rc = fsFsRenameFile(&device->fs, fs_path_old, fs_path_new);
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
  char   *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = r->deviceData;

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
  char   *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = r->deviceData;

  if(fsdev_getfspath(r, path, &device, fs_path)==-1)
    return NULL;

  /* get pointer to our data */
  fsdev_dir_t *dir = (fsdev_dir_t*)(dirState->dirStruct);

  /* open the directory */
  rc = fsFsOpenDirectory(&device->fs, fs_path, FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, &fd);
  if(R_SUCCEEDED(rc))
  {
    dir->magic = FSDEV_DIRITER_MAGIC;
    dir->fd    = fd;
    dir->index = -1;
    dir->size  = 0;
    memset(fsdevDirGetEntries(dir), 0, sizeof(FsDirectoryEntry)*__nx_fsdev_direntry_cache_size);
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
  s64                 entries;
  ssize_t             units;
  FsDirectoryEntry   *entry;

  /* get pointer to our data */
  fsdev_dir_t *dir = (fsdev_dir_t*)(dirState->dirStruct);

  const size_t max_entries = __nx_fsdev_direntry_cache_size;
  FsDirectoryEntry *entry_data = fsdevDirGetEntries(dir);

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
    memset(entry_data, 0, sizeof(FsDirectoryEntry)*max_entries);
    rc = fsDirRead(&dir->fd, &entries, max_entries, entry_data);
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
    entry = &entry_data[dir->index];

    /* fill in the stat info */
    filestat->st_ino = 0;
    if(entry->type == FsDirEntryType_Dir)
      filestat->st_mode = S_IFDIR;
    else if(entry->type == FsDirEntryType_File)
    {
      filestat->st_mode = S_IFREG;
      filestat->st_size = entry->file_size;
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
  char  *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = r->deviceData;
  s64 freespace = 0, total_space = 0;

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
  char   *fs_path = __nx_dev_path_buf;
  fsdev_fsdevice *device = r->deviceData;

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

