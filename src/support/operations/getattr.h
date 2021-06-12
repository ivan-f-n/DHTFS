
#ifndef __GETATTR_H_
#define __GETATTR_H_

#include "../../FSpart.h"
#include "../global.h"
#include "../logger.h"
#include "../globalFunctions.h"

using namespace std;

int FSpart::getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{
    Logger("Getting attributes for " + string(path));
    if (!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    int res = 0;
    inodeFile *i = getInodeStruct(path, runner);
    if (i == NULL)
    {
        Logger(string(path) + " not found");
        return -ENOENT;
    }
    else
    {
        Logger(string(path) + " found");
        Logger("Path  " + string(path) + ": " + string(*i));
        memset(stbuf, 0, sizeof(struct stat));
        time_t now = time(0); //Save current time
        stbuf->st_size = i->st.st_size;
        stbuf->st_dev = i->st.st_dev;         /* ID of device containing file */
        stbuf->st_ino = i->st.st_ino;         /* inode number */
        stbuf->st_mode = i->st.st_mode;       /* protection */
        stbuf->st_nlink = i->st.st_nlink;     /* number of hard links */
        stbuf->st_uid = i->st.st_uid;         /* user ID of owner */
        stbuf->st_gid = i->st.st_gid;         /* group ID of owner */
        stbuf->st_rdev = i->st.st_rdev;       /* device ID (if special file) */
        stbuf->st_blksize = i->st.st_blksize; /* blocksize for file system I/O */
        stbuf->st_blocks = i->st.st_blocks;   /* number of 512B blocks allocated */
        stbuf->st_atime = now;                /* time of last access */
        stbuf->st_ctime = now;                /* time of last status change */
    }
    return res;
}
#endif
