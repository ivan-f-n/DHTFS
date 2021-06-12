#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <vector>
#include <string>

#include <sys/syscall.h>
#include <signal.h>
#include <stdlib.h>
#include <regex>
#include <sys/time.h>
#include <opendht.h>
#include <iostream>
#include <fstream>
#include <mutex>
#include <condition_variable>
#include <msgpack.hpp>

using namespace std;

//Modified stat structure that supports serialization with MSGPack
struct statSerial {
    dev_t     st_dev;     /* ID of device containing file */
    ino_t     st_ino;     /* inode number */
    mode_t    st_mode;    /* protection */
    nlink_t   st_nlink;   /* number of hard links */
    uid_t     st_uid;     /* user ID of owner */
    gid_t     st_gid;     /* group ID of owner */
    dev_t     st_rdev;    /* device ID (if special file) */
    off_t     st_size;    /* total size, in bytes */
    blksize_t st_blksize; /* blocksize for file system I/O */
    blkcnt_t  st_blocks;  /* number of 512B blocks allocated */

    //Implements serialization
    MSGPACK_DEFINE(st_dev, st_ino, st_mode, st_nlink, st_uid, st_gid, st_rdev, st_size, st_blksize, st_blocks);
};

//Class that for each file contains:
//-Directory: stbuf struct and the files is the directory
//-Regular file: stbuf struct and the inodes (id) of the blocks of the file
class inodeFile {
public:
    struct statSerial st;
    vector<string> files;
    inodeFile(){};
    MSGPACK_DEFINE(st, files);

    inodeFile& operator=(inodeFile other) {
        st = other.st;
        files = other.files;
        return *this;
    }

    operator string() {
        string stringOfStruct;

        char buffer[100];
        sprintf(buffer, "%lu", (unsigned long)st.st_ino);
        stringOfStruct += "\nInode number" + string(buffer);

        char buffer2[100];
        sprintf(buffer2, "%u", (unsigned int)st.st_uid);
        stringOfStruct += "\nUser ID owner" + string(buffer2);

        char buffer3[100];
        sprintf(buffer3, "%ld", (long)st.st_size);
        stringOfStruct += "\n Size" + string(buffer3);


        stringOfStruct += "\n Files: ";
        for(int i = 0; i<files.size(); i++) {
            stringOfStruct += "\n" + files.at(i);
        }
        return stringOfStruct;
    }
};
#endif
