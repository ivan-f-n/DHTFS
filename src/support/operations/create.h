#ifndef __CREATE_H_
#define __CREATE_H_

#include "../../FSpart.h"
#include "../global.h"
#include "../logger.h"
#include "../globalFunctions.h"

using namespace std;

int FSpart::create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    Logger("Creating file " + string(path) + "with mode: " + to_string(mode));

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
        i->st.st_mode = mode;
        i->st.st_size = 1;
        i->st.st_nlink = 1;
        i->st.st_uid = getuid();
        i->st.st_gid = getgid();
        i->st.st_blksize = pow(2, 12);
        int firstInode = genInode();
        i->files.push_back(to_string(firstInode));
        unsigned char *buf = new unsigned char[1];
        buf[0] = '\0';
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
        runner->put(
            dht::InfoHash::get(to_string(firstInode)), dht::Value(buf, 1), [&](bool success) {}, dht::time_point::max(), true);
        cv.wait(lk, [&]
                { return s; });
        if (!s)
            return -ENOENT;
    }
    return 0;
}

#endif