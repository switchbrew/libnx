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
#include "runtime/util/utf.h"
#include "services/fs.h"

/// WARNING: This is not ready to be used.

typedef struct romfs_mount
{
    FsFile             fd;
    time_t             mtime;
    u64                offset;
    romfs_header       header;
    romfs_dir          *cwd;
    u32                *dirHashTable, *fileHashTable;
    void               *dirTable, *fileTable;
    struct romfs_mount *next;
} romfs_mount;

extern int __system_argc;
extern char** __system_argv;

static char __component[PATH_MAX+1];
//static uint16_t __utf16path[PATH_MAX+1];

#define romFS_root(m)   ((romfs_dir*)(m)->dirTable)
#define romFS_dir(m,x)  ((romfs_dir*) ((u8*)(m)->dirTable  + (x)))
#define romFS_file(m,x) ((romfs_file*)((u8*)(m)->fileTable + (x)))
#define romFS_none      ((u32)~0)
#define romFS_dir_mode  (S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH)
#define romFS_file_mode (S_IFREG | S_IRUSR | S_IRGRP | S_IROTH)

static ssize_t _romfs_read(romfs_mount *mount, u64 offset, void* buffer, u32 size)
{
    u64 pos = mount->offset + offset;
    size_t read = 0;
    Result rc = fsFileRead(&mount->fd, pos, buffer, size, &read);
    if (R_FAILED(rc)) return -1;
    return read;
}

static bool _romfs_read_chk(romfs_mount *mount, u64 offset, void* buffer, u32 size)
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

static devoptab_t romFS_devoptab =
{
    .name         = "romfs",
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
    .deviceData   = 0,
};

//-----------------------------------------------------------------------------

// File header
/*#define _3DSX_MAGIC 0x58534433 // '3DSX'
typedef struct
{
    u32 magic;
    u16 headerSize, relocHdrSize;
    u32 formatVer;
    u32 flags;

    // Sizes of the code, rodata and data segments +
    // size of the BSS section (uninitialized latter half of the data segment)
    u32 codeSegSize, rodataSegSize, dataSegSize, bssSize;
    // offset and size of smdh
    u32 smdhOffset, smdhSize;
    // offset to filesystem
    u32 fsOffset;
} _3DSX_Header;*/

static Result romfsMountCommon(romfs_mount *mount);
//static void   romfsInitMtime(romfs_mount *mount, FS_ArchiveID archId, FS_Path archPath, FS_Path filePath);

__attribute__((weak)) const char* __romfs_path = NULL;

static romfs_mount *romfs_mount_list = NULL;

static void romfs_insert(romfs_mount *mount)
{
    mount->next      = romfs_mount_list;
    romfs_mount_list = mount;
}

static void romfs_remove(romfs_mount *mount)
{
    for(romfs_mount **it = &romfs_mount_list; *it; it = &(*it)->next)
    {
        if(*it == mount)
        {
            *it = mount->next;
            return;
        }
    }
}

static romfs_mount* romfs_alloc(void)
{
    romfs_mount *mount = (romfs_mount*)calloc(1, sizeof(romfs_mount));

    if(mount)
        romfs_insert(mount);

    return mount;
}

static void romfs_free(romfs_mount *mount)
{
    romfs_remove(mount);
    free(mount->fileTable);
    free(mount->fileHashTable);
    free(mount->dirTable);
    free(mount->dirHashTable);
    free(mount);
}

Result romfsMount(struct romfs_mount **p)
{
    romfs_mount *mount = romfs_alloc();
    if(mount == NULL)
        return 99;

    if (/*envIsHomebrew()*/1)//TODO: How to handle?
    {
        // RomFS appended to a 3DSX file
        const char* filename = __romfs_path;
        if (__system_argc > 0 && __system_argv[0])
            filename = __system_argv[0];
        if (!filename)
        {
            romfs_free(mount);
            return 1;
        }

        if (strncmp(filename, "sdmc:/", 6) == 0)
            filename += 5;
        /*else if (strncmp(filename, "3dslink:/", 9) == 0)
        {
            strncpy(__component, "/3ds",     PATH_MAX);
            strncat(__component, filename+8, PATH_MAX);
            __component[PATH_MAX] = 0;
            filename = __component;
        }*/
        else
        {
            romfs_free(mount);
            return 2;
        }

        //TODO
        /*ssize_t units = utf8_to_utf16(__utf16path, (const uint8_t*)filename, PATH_MAX);
        if (units < 0)
        {
            romfs_free(mount);
            return 3;
        }
        if (units >= PATH_MAX)
        {
            romfs_free(mount);
            return 4;
        }
        __utf16path[units] = 0;

        FS_Path archPath = { PATH_EMPTY, 1, (u8*)"" };
        FS_Path filePath = { PATH_UTF16, (units+1)*2, (u8*)__utf16path };

        Result rc = FSUSER_OpenFileDirectly(&mount->fd, ARCHIVE_SDMC, archPath, filePath, FS_OPEN_READ, 0);
        if (R_FAILED(rc))
        {
            romfs_free(mount);
            return rc;
        }

        //romfsInitMtime(mount, ARCHIVE_SDMC, archPath, filePath);

        _3DSX_Header hdr;
        if (!_romfs_read_chk(mount, 0, &hdr, sizeof(hdr))) goto _fail0;
        if (hdr.magic != _3DSX_MAGIC) goto _fail0;
        if (hdr.headerSize < sizeof(hdr)) goto _fail0;
        mount->offset = hdr.fsOffset;
        if (!mount->offset) goto _fail0;*/
    }
    else//TODO
    {
        // Regular RomFS
        /*u8 zeros[0xC];
        memset(zeros, 0, sizeof(zeros));

        FS_Path archPath = { PATH_EMPTY, 1, (u8*)"" };
        FS_Path filePath = { PATH_BINARY, sizeof(zeros), zeros };

        Result rc = FSUSER_OpenFileDirectly(&mount->fd, ARCHIVE_ROMFS, archPath, filePath, FS_OPEN_READ, 0);
        if (R_FAILED(rc))
        {
            romfs_free(mount);
            return rc;
        }

        //romfsInitMtime(mount, ARCHIVE_ROMFS, archPath, filePath);*/
    }

    Result ret = romfsMountCommon(mount);
    if(R_SUCCEEDED(ret) && p)
        *p = mount;

    return ret;

//_fail0:
    fsFileClose(&mount->fd);
    romfs_free(mount);
    return 10;
}

Result romfsMountFromFile(FsFile file, u64 offset, struct romfs_mount **p)
{
    romfs_mount *mount = romfs_alloc();
    if(mount == NULL)
        return 99;

    mount->fd     = file;
    mount->offset = offset;

    Result ret = romfsMountCommon(mount);
    if(R_SUCCEEDED(ret) && p)
        *p = mount;

    return ret;
}

Result romfsMountCommon(romfs_mount *mount)
{
    if (_romfs_read(mount, 0, &mount->header, sizeof(mount->header)) != sizeof(mount->header))
        goto fail;

    mount->dirHashTable = (u32*)malloc(mount->header.dirHashTableSize);
    if (!mount->dirHashTable)
        goto fail;
    if (!_romfs_read_chk(mount, mount->header.dirHashTableOff, mount->dirHashTable, mount->header.dirHashTableSize))
        goto fail;

    mount->dirTable = malloc(mount->header.dirTableSize);
    if (!mount->dirTable)
        goto fail;
    if (!_romfs_read_chk(mount, mount->header.dirTableOff, mount->dirTable, mount->header.dirTableSize))
        goto fail;

    mount->fileHashTable = (u32*)malloc(mount->header.fileHashTableSize);
    if (!mount->fileHashTable)
        goto fail;
    if (!_romfs_read_chk(mount, mount->header.fileHashTableOff, mount->fileHashTable, mount->header.fileHashTableSize))
        goto fail;

    mount->fileTable = malloc(mount->header.fileTableSize);
    if (!mount->fileTable)
        goto fail;
    if (!_romfs_read_chk(mount, mount->header.fileTableOff, mount->fileTable, mount->header.fileTableSize))
        goto fail;

    mount->cwd = romFS_root(mount);

    // add device if this is the first one
    if(mount->next == NULL && AddDevice(&romFS_devoptab) < 0)
        goto fail;

    return 0;

fail:
    fsFileClose(&mount->fd);
    romfs_free(mount);
    return 10;
}

/*static void romfsInitMtime(romfs_mount *mount, FS_ArchiveID archId, FS_Path archPath, FS_Path filePath)
{
    u64 mtime;
    FS_Archive arch;
    Result rc;

    mount->mtime = time(NULL);
    rc = FSUSER_OpenArchive(&arch, archId, archPath);
    if (R_FAILED(rc))
        return;

    rc = FSUSER_ControlArchive(arch, ARCHIVE_ACTION_GET_TIMESTAMP,
                               (void*)filePath.data, filePath.size,
                               &mtime, sizeof(mtime));
    FSUSER_CloseArchive(arch);
    if (R_FAILED(rc))
        return;*/

    /* convert from milliseconds to seconds */
    //mtime /= 1000;
    /* convert from 2000-based timestamp to UNIX timestamp */
    /*mtime += 946684800;
    mount->mtime = mtime;
}*/

Result romfsBind(struct romfs_mount *mount)
{
    for(romfs_mount **it = &romfs_mount_list; *it; it = &(*it)->next)
    {
        if(*it == mount)
        {
            *it = mount->next;
            romfs_insert(mount);
            return 0;
        }
    }

    return 99;
}

Result romfsUnmount(struct romfs_mount *mount)
{
    if(mount)
    {
        // unmount specific
        fsFileClose(&mount->fd);
        romfs_free(mount);
    }
    else
    {
        // unmount everything
        while(romfs_mount_list)
        {
            fsFileClose(&romfs_mount_list->fd);
            romfs_free(romfs_mount_list);
        }
    }

    // if no more mounts, remove device
    if(romfs_mount_list == NULL)
        RemoveDevice("romfs:");

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

static romfs_dir* searchForDir(romfs_mount *mount, romfs_dir* parent, const uint8_t* name, u32 namelen)
{
    u64 parentOff = (uintptr_t)parent - (uintptr_t)mount->dirTable;
    u32 hash = calcHash(parentOff, name, namelen, mount->header.dirHashTableSize/4);
    romfs_dir* curDir = NULL;
    u32 curOff;
    for (curOff = mount->dirHashTable[hash]; curOff != romFS_none; curOff = curDir->nextHash)
    {
        curDir = romFS_dir(mount, curOff);
        if (curDir->parent != parentOff) continue;
        if (curDir->nameLen != namelen) continue;
        if (memcmp(curDir->name, name, namelen) != 0) continue;
        return curDir;
    }
    return NULL;
}

static romfs_file* searchForFile(romfs_mount *mount, romfs_dir* parent, const uint8_t* name, u32 namelen)
{
    u64 parentOff = (uintptr_t)parent - (uintptr_t)mount->dirTable;
    u32 hash = calcHash(parentOff, name, namelen, mount->header.fileHashTableSize/4);
    romfs_file* curFile = NULL;
    u32 curOff;
    for (curOff = mount->fileHashTable[hash]; curOff != romFS_none; curOff = curFile->nextHash)
    {
        curFile = romFS_file(mount, curOff);
        if (curFile->parent != parentOff) continue;
        if (curFile->nameLen != namelen) continue;
        if (memcmp(curFile->name, name, namelen) != 0) continue;
        return curFile;
    }
    return NULL;
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
        char* component = __component;

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
                continue;
            }
        }

        *ppDir = searchForDir(mount, *ppDir, (uint8_t*)component, strlen(component)+1);
        if (!*ppDir)
            return EEXIST;
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
        ++count;
        offset = tmp->sibling;
    }

    offset = dir->childFile;
    while(offset != romFS_none)
    {
        romfs_file *tmp = romFS_file(mount, offset);
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

    fileobj->mount = romfs_mount_list;

    if ((flags & O_ACCMODE) != O_RDONLY)
    {
        r->_errno = EROFS;
        return -1;
    }

    romfs_dir* curDir = NULL;
    r->_errno = navigateToDir(fileobj->mount, &curDir, &path, false);
    if (r->_errno != 0)
        return -1;

    romfs_file* file = searchForFile(fileobj->mount, curDir, (uint8_t*)path, strlen(path)+1);
    if (!file)
    {
        if(flags & O_CREAT)
            r->_errno = EROFS;
        else
            r->_errno = ENOENT;
        return -1;
    }
    else if((flags & O_CREAT) && (flags & O_EXCL))
    {
        r->_errno = EEXIST;
        return -1;
    }

    fileobj->file   = file;
    fileobj->offset = (u64)fileobj->mount->header.fileDataOff + file->dataOff;
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
    romfs_mount* mount = romfs_mount_list;
    romfs_dir* curDir = NULL;
    r->_errno = navigateToDir(mount, &curDir, &path, false);
    if(r->_errno != 0)
        return -1;

    romfs_dir* dir = searchForDir(mount, curDir, (uint8_t*)path, strlen(path)+1);
    if(dir)
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

    romfs_file* file = searchForFile(mount, curDir, (uint8_t*)path, strlen(path)+1);
    if(file)
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
    romfs_mount* mount = romfs_mount_list;
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
    iter->mount = romfs_mount_list;

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

