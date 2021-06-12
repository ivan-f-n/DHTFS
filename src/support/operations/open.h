#ifndef __OPEN_H_
#define __OPEN_H_

#include "../../FSpart.h"
#include "../global.h"
#include "../logger.h"
#include "../globalFunctions.h"

using namespace std;

int FSpart::open(const char *path, struct fuse_file_info *fi)
{
    Logger("Opening file " + string(path));
    if (!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    inodeFile *i = getInodeStruct(path, runner);
    if (i == NULL)
    {
        return -ENOENT;
    }

    return 0;
}

#endif