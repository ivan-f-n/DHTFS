#ifndef __READDIR_H_
#define __READDIR_H_

#include "../../FSpart.h"
#include "../global.h"
#include "../logger.h"
#include "../globalFunctions.h"

using namespace std;

int FSpart::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                    off_t, struct fuse_file_info *fi, enum fuse_readdir_flags)
{
    Logger("Reading directory " + string(path));
    if (!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    inodeFile *i = getInodeStruct(path, runner);
    Logger("Path  " + string(path) + ": " + string(*i));

    if (i == NULL)
    {
        Logger(string(path) + " directory not found");
        return -ENOENT;
    }
    else
    {
        if (S_ISREG(i->st.st_mode))
        {
            return -ENOENT;
        }
        else
        {
            int off, nextoff = 0, lenentry;
            for (string f : i->files)
            {
                struct stat st;
                if (f.compare(".") == 0 || f.compare("..") == 0)
                {
                    if (filler(buf, f.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS))
                        break;
                }
                else
                {
                    getattr(f.c_str(), &st, fi);
                    if (filler(buf, f.c_str(), &st, 0, FUSE_FILL_DIR_PLUS))
                        break;
                }
            }
        }
    }

    return 0;
}

#endif