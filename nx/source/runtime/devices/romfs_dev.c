#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/iosupport.h>
#include <sys/param.h>
#include <unistd.h>

#include "runtime/devices/romfs_dev.h"
#include "runtime/devices/fs_dev.h"
#include "runtime/util/utf.h"
#include "runtime/env.h"
#include "nro.h"

#include "path_buf.h"

typedef enum {
    RomfsSource_FsFile,
    RomfsSource_FsStorage,
} RomfsSource;

typedef struct romfs_mount
{
    devoptab_t         device;
    bool               setup;
    RomfsSource        fd_type;
    s32                id;
    FsFile             fd;
    FsStorage          fd_storage;
    time_t             mtime;
    u64                offset;
    romfs_header       header;
    romfs_dir          *cwd;
    u32                *dirHashTable, *fileHashTable;
    void               *dirTable, *fileTable;
    char               name[32];
} romfs_mount;

extern int __system_argc;
extern char** __system_argv;

#define romFS_root(m)   ((romfs_dir*)(m)->dirTable)
#define romFS_none      ((u32)~0)
#define romFS_dir_mode  (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH)
#define romFS_file_mode (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH)

static romfs_dir *romFS_dir(romfs_mount *mount, u32 off)
{
    if (off + sizeof(romfs_dir) > mount->header.dirTableSize) return NULL;
    romfs_dir* curDir = ((romfs_dir*) ((u8*)mount->dirTable + off));
    if (off + sizeof(romfs_dir) + curDir->nameLen > mount->header.dirTableSize) return NULL;
    return curDir;
}

static romfs_file *romFS_file(romfs_mount *mount, u32 off)
{
    if (off + sizeof(romfs_file) > mount->header.fileTableSize) return NULL;
    romfs_file* curFile = ((romfs_file*) ((u8*)mount->fileTable + off));
    if (off + sizeof(romfs_file) + curFile->nameLen > mount->header.fileTableSize) return NULL;
    return curFile;
}

static ssize_t _romfs_read(romfs_mount *mount, u64 offset, void* buffer, u64 size)
{
    s64 pos = mount->offset + offset;
    u64 read = 0;
    Result rc = 0;
    if(mount->fd_type == RomfsSource_FsFile)
    {
        rc = fsFileRead(&mount->fd, pos, buffer, size, FsReadOption_None, &read);
    }
    else if(mount->fd_type == RomfsSource_FsStorage)
    {
        rc = fsStorageRead(&mount->fd_storage, pos, buffer, size);
        read = size;
    }
    if (R_FAILED(rc)) return -1;
    return read;
}

static bool _romfs_read_chk(romfs_mount *mount, u64 offset, void* buffer, u64 size)
{
    return _romfs_read(mount, offset, buffer, size) == size;
}

//-----------------------------------------------------------------------------

static int       romfs_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
static int       romfs_close(struct _reent *r, void *fd);
static ssize_t   romfs_read(struct _reent *r, void *fd, char *ptr, size_t len);
static off_t     romfs_seek(struct _reent *r, void *fd, off_t pos, int dir);
static int       romfs_fstat(struct _reent *r, void *fd, struct stat *st);
static int       romfs_stat(struct _reent *r, const char *path, struct stat *st);
static int       romfs_chdir(struct _reent *r, const char *path);
static DIR_ITER* romfs_diropen(struct _reent *r, DIR_ITER *dirState, const char *path);
static int       romfs_dirreset(struct _reent *r, DIR_ITER *dirState);
static int       romfs_dirnext(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat);
static int       romfs_dirclose(struct _reent *r, DIR_ITER *dirState);

typedef struct
{
    romfs_mount *mount;
    romfs_file  *file;
    u64         offset, pos;
} romfs_fileobj;

typedef struct
{
    romfs_mount *mount;
    romfs_dir* dir;
    u32        state;
    u32        childDir;
    u32        childFile;
} romfs_diriter;

static const devoptab_t romFS_devoptab =
{
    .structSize   = sizeof(romfs_fileobj),
    .open_r       = romfs_open,
    .close_r      = romfs_close,
    .read_r       = romfs_read,
    .seek_r       = romfs_seek,
    .fstat_r      = romfs_fstat,
    .stat_r       = romfs_stat,
    .chdir_r      = romfs_chdir,
    .dirStateSize = sizeof(romfs_diriter),
    .diropen_r    = romfs_diropen,
    .dirreset_r   = romfs_dirreset,
    .dirnext_r    = romfs_dirnext,
    .dirclose_r   = romfs_dirclose,
};

static bool romfs_initialised = false;
static romfs_mount romfs_mounts[32];

//-----------------------------------------------------------------------------

static Result romfsMountCommon(const char *name, romfs_mount *mount);
static void romfsInitMtime(romfs_mount *mount);

static void _romfsResetMount(romfs_mount *mount, s32 id) {
    memset(mount, 0, sizeof(*mount));
    memcpy(&mount->device, &romFS_devoptab, sizeof(romFS_devoptab));
    mount->device.name = mount->name;
    mount->device.deviceData = mount;
    mount->id = id;
}

static void _romfsInit(void)
{
    u32 i;
    u32 total = sizeof(romfs_mounts) / sizeof(romfs_mount);

    if(!romfs_initialised)
    {
        for(i = 0; i < total; i++)
        {
            _romfsResetMount(&romfs_mounts[i], i);
        }

        romfs_initialised = true;
    }
}

static romfs_mount *romfsFindMount(const char *name)
{
    u32 i;
    u32 total = sizeof(romfs_mounts) / sizeof(romfs_mount);
    romfs_mount *mount = NULL;

    _romfsInit();

    for(i=0; i<total; i++)
    {
        mount = &romfs_mounts[i];

        if(name==NULL) //Find an unused mount entry.
        {
            if(!mount->setup)
                return mount;
        }
        else if(mount->setup) //Find the mount with the input name.
        {
            if(strncmp(mount->name, name, sizeof(mount->name))==0)
                return mount;
        }
    }

    return NULL;
}

__attribute__((weak)) const char* __romfs_path = NULL;

static romfs_mount* romfs_alloc(void)
{
    return romfsFindMount(NULL);
}

static void romfs_free(romfs_mount *mount)
{
    free(mount->fileTable);
    free(mount->fileHashTable);
    free(mount->dirTable);
    free(mount->dirHashTable);
    _romfsResetMount(mount, mount->id);
}

static void romfs_mountclose(romfs_mount *mount)
{
    if(mount->fd_type == RomfsSource_FsFile)fsFileClose(&mount->fd);
    if(mount->fd_type == RomfsSource_FsStorage)fsStorageClose(&mount->fd_storage);
    romfs_free(mount);
}

Result romfsMountSelf(const char *name)
{
    // Check whether we are a NSO; if so then just mount the RomFS from the current process
    if (envIsNso())
        return romfsMountFromCurrentProcess(name);

    // Otherwise, we are a homebrew NRO and we need to use our embedded RomFS
    // Retrieve the filename of our NRO
    const char* filename = __romfs_path;
    if (__system_argc > 0 && __system_argv[0])
        filename = __system_argv[0];
    if (!filename)
        return MAKERESULT(Module_Libnx, LibnxError_NotFound);

    // Retrieve IFileSystem object + fixed path for our NRO
    FsFileSystem *tmpfs = NULL;
    char* path_buf = __nx_dev_path_buf;
    if(fsdevTranslatePath(filename, &tmpfs, path_buf)==-1)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    // Open the NRO file
    FsFile nro_file;
    Result rc = fsFsOpenFile(tmpfs, path_buf, FsOpenMode_Read, &nro_file);
    if (R_FAILED(rc))
        return rc;

    // Read and parse the header
    NroHeader hdr;
    u64 readbytes = 0;
    rc = fsFileRead(&nro_file, sizeof(NroStart), &hdr, sizeof(hdr), FsReadOption_None, &readbytes);
    if (R_FAILED(rc) || readbytes != sizeof(hdr)) goto _fail_io;
    if (hdr.magic != NROHEADER_MAGIC) goto _fail_io;

    // Read and parse the asset header
    NroAssetHeader asset_header;
    rc = fsFileRead(&nro_file, hdr.size, &asset_header, sizeof(asset_header), FsReadOption_None, &readbytes);
    if (R_FAILED(rc) || readbytes != sizeof(asset_header)) goto _fail_io;
    if (asset_header.magic != NROASSETHEADER_MAGIC
        || asset_header.version > NROASSETHEADER_VERSION
        || asset_header.romfs.offset == 0
        || asset_header.romfs.size == 0)
        goto _fail_io;

    // Calculate the start offset of the embedded RomFS and mount it
    u64 romfs_offset = hdr.size + asset_header.romfs.offset;
    return romfsMountFromFile(nro_file, romfs_offset, name);

_fail_io:
    fsFileClose(&nro_file);
    return MAKERESULT(Module_Libnx, LibnxError_IoError);
}

Result romfsMountFromFile(FsFile file, u64 offset, const char *name)
{
    romfs_mount *mount = romfs_alloc();
    if(mount == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    mount->fd_type = RomfsSource_FsFile;
    mount->fd     = file;
    mount->offset = offset;

    return romfsMountCommon(name, mount);
}

Result romfsMountFromStorage(FsStorage storage, u64 offset, const char *name)
{
    romfs_mount *mount = romfs_alloc();
    if(mount == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    mount->fd_type = RomfsSource_FsStorage;
    mount->fd_storage = storage;
    mount->offset = offset;

    return romfsMountCommon(name, mount);
}

Result romfsMountFromCurrentProcess(const char *name) {
    FsStorage storage;

    Result rc = fsOpenDataStorageByCurrentProcess(&storage);
    if (R_FAILED(rc))
        return rc;

    return romfsMountFromStorage(storage, 0, name);
}

Result romfsMountFromFsdev(const char *path, u64 offset, const char *name)
{
    FsFileSystem *tmpfs = NULL;
    if(fsdevTranslatePath(path, &tmpfs, __nx_dev_path_buf)==-1)
        return MAKERESULT(Module_Libnx, LibnxError_BadInput);

    romfs_mount *mount = romfs_alloc();
    if(mount == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

    mount->fd_type = RomfsSource_FsFile;
    mount->offset = offset;

    Result rc = fsFsOpenFile(tmpfs, __nx_dev_path_buf, FsOpenMode_Read, &mount->fd);
    if (R_FAILED(rc))
    {
        romfs_free(mount);
        return rc;
    }

    return romfsMountCommon(name, mount);
}

Result romfsMountFromDataArchive(u64 dataId, NcmStorageId storageId, const char *name) {
    FsStorage storage;

    Result rc = fsOpenDataStorageByDataId(&storage, dataId, storageId);
    if (R_FAILED(rc))
        return rc;

    return romfsMountFromStorage(storage, 0, name);
}

Result romfsMountCommon(const char *name, romfs_mount *mount)
{
    memset(mount->name, 0, sizeof(mount->name));
    strncpy(mount->name, name, sizeof(mount->name)-1);

    romfsInitMtime(mount);

    if (_romfs_read(mount, 0, &mount->header, sizeof(mount->header)) != sizeof(mount->header))
        goto fail_io;

    mount->dirHashTable = (u32*)malloc(mount->header.dirHashTableSize);
    if (!mount->dirHashTable)
        goto fail_oom;
    if (!_romfs_read_chk(mount, mount->header.dirHashTableOff, mount->dirHashTable, mount->header.dirHashTableSize))
        goto fail_io;

    mount->dirTable = malloc(mount->header.dirTableSize);
    if (!mount->dirTable)
        goto fail_oom;
    if (!_romfs_read_chk(mount, mount->header.dirTableOff, mount->dirTable, mount->header.dirTableSize))
        goto fail_io;

    mount->fileHashTable = (u32*)malloc(mount->header.fileHashTableSize);
    if (!mount->fileHashTable)
        goto fail_oom;
    if (!_romfs_read_chk(mount, mount->header.fileHashTableOff, mount->fileHashTable, mount->header.fileHashTableSize))
        goto fail_io;

    mount->fileTable = malloc(mount->header.fileTableSize);
    if (!mount->fileTable)
        goto fail_oom;
    if (!_romfs_read_chk(mount, mount->header.fileTableOff, mount->fileTable, mount->header.fileTableSize))
        goto fail_io;

    mount->cwd = romFS_root(mount);

    if(AddDevice(&mount->device) < 0)
        goto fail_oom;

    mount->setup = true;
    return 0;

fail_oom:
    romfs_mountclose(mount);
    return MAKERESULT(Module_Libnx, LibnxError_OutOfMemory);

fail_io:
    romfs_mountclose(mount);
    return MAKERESULT(Module_Libnx, LibnxError_IoError);
}

static void romfsInitMtime(romfs_mount *mount)
{
    mount->mtime = time(NULL);
}

Result romfsUnmount(const char *name)
{
    romfs_mount *mount;
    char tmpname[34];

    mount = romfsFindMount(name);
    if (mount == NULL)
        return MAKERESULT(Module_Libnx, LibnxError_NotFound);

    // Remove device
    memset(tmpname, 0, sizeof(tmpname));
    strncpy(tmpname, mount->name, sizeof(tmpname)-2);
    strncat(tmpname, ":", sizeof(tmpname)-strlen(tmpname)-1);

    RemoveDevice(tmpname);

    romfs_mountclose(mount);

    return 0;
}

//-----------------------------------------------------------------------------

static u32 calcHash(u32 parent, const uint8_t* name, u32 namelen, u32 total)
{
    u32 hash = parent ^ 123456789;
    u32 i;
    for (i = 0; i < namelen; i ++)
    {
        hash = (hash >> 5) | (hash << 27);
        hash ^= name[i];
    }
    return hash % total;
}

static int searchForDir(romfs_mount *mount, romfs_dir* parent, const uint8_t* name, u32 namelen, romfs_dir** out)
{
    u64 parentOff = (uintptr_t)parent - (uintptr_t)mount->dirTable;
    u32 hash = calcHash(parentOff, name, namelen, mount->header.dirHashTableSize/4);
    romfs_dir* curDir = NULL;
    u32 curOff;
    *out = NULL;
    for (curOff = mount->dirHashTable[hash]; curOff != romFS_none; curOff = curDir->nextHash)
    {
        curDir = romFS_dir(mount, curOff);
        if (curDir == NULL) return EFAULT;
        if (curDir->parent != parentOff) continue;
        if (curDir->nameLen != namelen) continue;
        if (memcmp(curDir->name, name, namelen) != 0) continue;
        *out = curDir;
        return 0;
    }
    return ENOENT;
}

static int searchForFile(romfs_mount *mount, romfs_dir* parent, const uint8_t* name, u32 namelen, romfs_file** out)
{
    u64 parentOff = (uintptr_t)parent - (uintptr_t)mount->dirTable;
    u32 hash = calcHash(parentOff, name, namelen, mount->header.fileHashTableSize/4);
    romfs_file* curFile = NULL;
    u32 curOff;
    *out = NULL;
    for (curOff = mount->fileHashTable[hash]; curOff != romFS_none; curOff = curFile->nextHash)
    {
        curFile = romFS_file(mount, curOff);
        if (curFile == NULL) return EFAULT;
        if (curFile->parent != parentOff) continue;
        if (curFile->nameLen != namelen) continue;
        if (memcmp(curFile->name, name, namelen) != 0) continue;
        *out = curFile;
        return 0;
    }
    return ENOENT;
}

static int navigateToDir(romfs_mount *mount, romfs_dir** ppDir, const char** pPath, bool isDir)
{
    char* colonPos = strchr(*pPath, ':');
    if (colonPos) *pPath = colonPos+1;
    if (!**pPath)
        return EILSEQ;

    *ppDir = mount->cwd;
    if (**pPath == '/')
    {
        *ppDir = romFS_root(mount);
        (*pPath)++;
    }

    while (**pPath)
    {
        char* slashPos = strchr(*pPath, '/');
        char* component = __nx_dev_path_buf;

        if (slashPos)
        {
            u32 len = slashPos - *pPath;
            if (!len)
                return EILSEQ;
            if (len > PATH_MAX)
                return ENAMETOOLONG;

            memcpy(component, *pPath, len);
            component[len] = 0;
            *pPath = slashPos+1;
        } else if (isDir)
        {
            component = (char*)*pPath;
            *pPath += strlen(component);
        } else
            return 0;

        if (component[0]=='.')
        {
            if (!component[1]) continue;
            if (component[1]=='.' && !component[2])
            {
                *ppDir = romFS_dir(mount, (*ppDir)->parent);
                if (!*ppDir)
                    return EFAULT;
                continue;
            }
        }

        int ret = searchForDir(mount, *ppDir, (uint8_t*)component, strlen(component), ppDir);
        if (ret !=0)
            return ret;
    }

    if (!isDir && !**pPath)
        return EILSEQ;

    return 0;
}

static ino_t dir_inode(romfs_mount *mount, romfs_dir *dir)
{
    return (uint32_t*)dir - (uint32_t*)mount->dirTable;
}

static off_t dir_size(romfs_dir *dir)
{
    return sizeof(romfs_dir) + (dir->nameLen+3)/4;
}

static nlink_t dir_nlink(romfs_mount *mount, romfs_dir *dir)
{
    nlink_t count = 2; // one for self, one for parent
    u32     offset = dir->childDir;

    while(offset != romFS_none)
    {
        romfs_dir *tmp = romFS_dir(mount, offset);
        if (!tmp) break;
        ++count;
        offset = tmp->sibling;
    }

    offset = dir->childFile;
    while(offset != romFS_none)
    {
        romfs_file *tmp = romFS_file(mount, offset);
        if (!tmp) break;
        ++count;
        offset = tmp->sibling;
    }

    return count;
}

static ino_t file_inode(romfs_mount *mount, romfs_file *file)
{
    return ((uint32_t*)file - (uint32_t*)mount->fileTable) + mount->header.dirTableSize/4;
}

//-----------------------------------------------------------------------------

int romfs_open(struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
    romfs_fileobj* fileobj = (romfs_fileobj*)fileStruct;

    fileobj->mount = (romfs_mount*)r->deviceData;

    if ((flags & O_ACCMODE) != O_RDONLY)
    {
        r->_errno = EROFS;
        return -1;
    }

    romfs_dir* curDir = NULL;
    r->_errno = navigateToDir(fileobj->mount, &curDir, &path, false);
    if (r->_errno != 0)
        return -1;

    romfs_file* file = NULL;
    int ret = searchForFile(fileobj->mount, curDir, (uint8_t*)path, strlen(path), &file);
    if (ret != 0)
    {
        if(ret == ENOENT && (flags & O_CREAT))
            r->_errno = EROFS;
        else
            r->_errno = ret;
        return -1;
    }
    else if((flags & O_CREAT) && (flags & O_EXCL))
    {
        r->_errno = EEXIST;
        return -1;
    }

    fileobj->file   = file;
    fileobj->offset = fileobj->mount->header.fileDataOff + file->dataOff;
    fileobj->pos    = 0;

    return 0;
}

int romfs_close(struct _reent *r, void *fd)
{
    return 0;
}

ssize_t romfs_read(struct _reent *r, void *fd, char *ptr, size_t len)
{
    romfs_fileobj* file = (romfs_fileobj*)fd;
    u64 endPos = file->pos + len;

    /* check if past end-of-file */
    if(file->pos >= file->file->dataSize)
        return 0;

    /* truncate the read to end-of-file */
    if(endPos > file->file->dataSize)
        endPos = file->file->dataSize;
    len = endPos - file->pos;

    ssize_t adv = _romfs_read(file->mount, file->offset + file->pos, ptr, len);
    if(adv >= 0)
    {
        file->pos += adv;
        return adv;
    }

    r->_errno = EIO;
    return -1;
}

off_t romfs_seek(struct _reent *r, void *fd, off_t pos, int dir)
{
    romfs_fileobj* file = (romfs_fileobj*)fd;
    off_t          start;
    switch (dir)
    {
        case SEEK_SET:
            start = 0;
            break;

        case SEEK_CUR:
            start = file->pos;
            break;

        case SEEK_END:
            start = file->file->dataSize;
            break;

        default:
            r->_errno = EINVAL;
            return -1;
    }

    /* don't allow negative position */
    if(pos < 0)
    {
         if(start + pos < 0)
        {
            r->_errno = EINVAL;
            return -1;
        }
    }
    /* check for overflow */
    else if(INT64_MAX - pos < start)
    {
        r->_errno = EOVERFLOW;
        return -1;
    }

    file->pos = start + pos;
    return file->pos;
}

int romfs_fstat(struct _reent *r, void *fd, struct stat *st)
{
    romfs_fileobj* file = (romfs_fileobj*)fd;
    memset(st, 0, sizeof(struct stat));
    st->st_ino   = file_inode(file->mount, file->file);
    st->st_mode  = romFS_file_mode;
    st->st_nlink = 1;
    st->st_size  = (off_t)file->file->dataSize;
    st->st_blksize = 512;
    st->st_blocks  = (st->st_blksize + 511) / 512;
    st->st_atime = st->st_mtime = st->st_ctime = file->mount->mtime;

    return 0;
}

int romfs_stat(struct _reent *r, const char *path, struct stat *st)
{
    romfs_mount* mount = (romfs_mount*)r->deviceData;
    romfs_dir* curDir = NULL;
    r->_errno = navigateToDir(mount, &curDir, &path, false);
    if(r->_errno != 0)
        return -1;

    romfs_dir* dir = NULL;
    int ret=0;
    ret = searchForDir(mount, curDir, (uint8_t*)path, strlen(path), &dir);
    if (ret != 0 && ret != ENOENT)
    {
        r->_errno = ret;
        return -1;
    }
    if(ret == 0)
    {
        memset(st, 0, sizeof(*st));
        st->st_ino     = dir_inode(mount, dir);
        st->st_mode    = romFS_dir_mode;
        st->st_nlink   = dir_nlink(mount, dir);
        st->st_size    = dir_size(dir);
        st->st_blksize = 512;
        st->st_blocks  = (st->st_blksize + 511) / 512;
        st->st_atime = st->st_mtime = st->st_ctime = mount->mtime;

        return 0;
    }

    romfs_file* file = NULL;
    ret = searchForFile(mount, curDir, (uint8_t*)path, strlen(path), &file);
    if (ret != 0 && ret != ENOENT)
    {
        r->_errno = ret;
        return -1;
    }
    if(ret == 0)
    {
        memset(st, 0, sizeof(*st));
        st->st_ino   = file_inode(mount, file);
        st->st_mode  = romFS_file_mode;
        st->st_nlink = 1;
        st->st_size  = file->dataSize;
        st->st_blksize = 512;
        st->st_blocks  = (st->st_blksize + 511) / 512;
        st->st_atime = st->st_mtime = st->st_ctime = mount->mtime;

        return 0;
    }

    r->_errno = ENOENT;
    return 1;
}

int romfs_chdir(struct _reent *r, const char *path)
{
    romfs_mount* mount = (romfs_mount*)r->deviceData;
    romfs_dir* curDir = NULL;
    r->_errno = navigateToDir(mount, &curDir, &path, true);
    if (r->_errno != 0)
        return -1;

    mount->cwd = curDir;
    return 0;
}

DIR_ITER* romfs_diropen(struct _reent *r, DIR_ITER *dirState, const char *path)
{
    romfs_diriter* iter = (romfs_diriter*)(dirState->dirStruct);
    romfs_dir* curDir = NULL;
    iter->mount = (romfs_mount*)r->deviceData;

    r->_errno = navigateToDir(iter->mount, &curDir, &path, true);
    if(r->_errno != 0)
        return NULL;

    iter->dir       = curDir;
    iter->state     = 0;
    iter->childDir  = curDir->childDir;
    iter->childFile = curDir->childFile;

    return dirState;
}

int romfs_dirreset(struct _reent *r, DIR_ITER *dirState)
{
    romfs_diriter* iter = (romfs_diriter*)(dirState->dirStruct);

    iter->state     = 0;
    iter->childDir  = iter->dir->childDir;
    iter->childFile = iter->dir->childFile;

    return 0;
}

int romfs_dirnext(struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
    romfs_diriter* iter = (romfs_diriter*)(dirState->dirStruct);

    if(iter->state == 0)
    {
        /* '.' entry */
        memset(filestat, 0, sizeof(*filestat));
        filestat->st_ino  = dir_inode(iter->mount, iter->dir);
        filestat->st_mode = romFS_dir_mode;

        strcpy(filename, ".");
        iter->state = 1;
        return 0;
    }
    else if(iter->state == 1)
    {
        /* '..' entry */
        romfs_dir* dir = romFS_dir(iter->mount, iter->dir->parent);
        if(!dir)
        {
            r->_errno = EFAULT;
            return -1;
        }

        memset(filestat, 0, sizeof(*filestat));
        filestat->st_ino = dir_inode(iter->mount, dir);
        filestat->st_mode = romFS_dir_mode;

        strcpy(filename, "..");
        iter->state = 2;
        return 0;
    }

    if(iter->childDir != romFS_none)
    {
        romfs_dir* dir = romFS_dir(iter->mount, iter->childDir);
        if(!dir)
        {
            r->_errno = EFAULT;
            return -1;
        }

        iter->childDir = dir->sibling;

        memset(filestat, 0, sizeof(*filestat));
        filestat->st_ino = dir_inode(iter->mount, dir);
        filestat->st_mode = romFS_dir_mode;

        memset(filename, 0, NAME_MAX);

        if(dir->nameLen >= NAME_MAX)
        {
            r->_errno = ENAMETOOLONG;
            return -1;
        }

        strncpy(filename, (char*)dir->name, dir->nameLen);

        return 0;
    }
    else if(iter->childFile != romFS_none)
    {
        romfs_file* file = romFS_file(iter->mount, iter->childFile);
        if(!file)
        {
            r->_errno = EFAULT;
            return -1;
        }

        iter->childFile = file->sibling;

        memset(filestat, 0, sizeof(*filestat));
        filestat->st_ino = file_inode(iter->mount, file);
        filestat->st_mode = romFS_file_mode;

        memset(filename, 0, NAME_MAX);

        if(file->nameLen >= NAME_MAX)
        {
            r->_errno = ENAMETOOLONG;
            return -1;
        }

        strncpy(filename, (char*)file->name, file->nameLen);

        return 0;
    }

    r->_errno = ENOENT;
    return -1;
}

int romfs_dirclose(struct _reent *r, DIR_ITER *dirState)
{
    return 0;
}

