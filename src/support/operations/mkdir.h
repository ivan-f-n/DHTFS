#ifndef __MKDIR_H_
#define __MKDIR_H_

#include "../../FSpart.h"
#include "../global.h"
#include "../logger.h"
#include "../globalFunctions.h"

using namespace std;

int FSpart::mkdir(const char *path, mode_t mode)
{
    Logger("Making directory " + string(path));

    std::mutex mtx;
    std::unique_lock<std::mutex> lk(mtx);
    std::condition_variable cv;

    if (!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    inodeFile *i = getInodeStruct(path, runner);
    if (i != NULL)
    {
        return -ENOENT;
    }
    else
    {
        inodeFile *i = new inodeFile();
        i->st.st_mode = S_IFDIR | 0755;
        i->st.st_uid = getuid();
        i->st.st_gid = getgid();
        i->st.st_nlink = 2;
        i->files.emplace_back(string("."));
        i->files.emplace_back(string(".."));
        int inode = genInode();
        changeEntry(path, inode, runner);
        bool s = false;
        runner->put(
            dht::InfoHash::get(to_string(inode)), *i, [&](bool success)
            {
                std::lock_guard<std::mutex> l(mtx);
                s = true;
                cv.notify_all();
            },
            dht::time_point::max(), true);

        cv.wait(lk, [&]
                { return s; });
        if (!s)
            return -1;
    }
    return 0;
}

#endif