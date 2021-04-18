//
// Created by ifnav on 02/02/2020.
//

#include <vector>
#include <string>

#include <sys/syscall.h>
#include <signal.h>
#include <stdlib.h>
#include <regex>

map<string, int> filesInodes;
dht::Value* filesValue = NULL;
static const string files = "fileStr";
dht::DhtRunner* node;

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
class inodeFile {
public:
    struct statSerial st;
    vector<string> files;
    inodeFile(){};
    MSGPACK_DEFINE(st, files);
};

map<string, int> getFiles()
{
    return filesInodes;
}

int getINODE(const char* path)
{
    string p(path);
    if(filesInodes.find(p) != filesInodes.end())
    {
        return filesInodes[p];
    }else {
        return -1;
    }
}

int getNewInode()
{
    map<string, int>::iterator it = filesInodes.begin();
    while(it != filesInodes.end())
    {
        it++;
        if(it == filesInodes.end())
        {
            return --it->second + 1;
        }
    }
    return 0;
}

inodeFile* getInodeFile(const char* path)
{
    inodeFile* i = new inodeFile();
    // get data from the dht
    if(getINODE(path)==-1)  return NULL;
    node->get<inodeFile>(dht::InfoHash::get(to_string(getINODE(path))), [&](inodeFile&& myObject) {
        *i = myObject;
        return false; // return false to stop the search
    });
    time_t now = time(0); //Save current time
    while(i==NULL && time(0)-5 < now); //Wait until value is received or timeout

    return i;
}
dht::Value f(dht::Value a) {
    return a;
}

int editINODE(const char* path, int inode)
{
    string p(path);
    size_t found;
    found=p.find_last_of("/");
    std::map<string, int>::iterator it = filesInodes.find(p.substr(0,found));
    if(it != filesInodes.end())
    {
        inodeFile* i = getInodeFile(path);
        dht::Value d {*i};
        node->cancelPut(dht::InfoHash::get(to_string(filesInodes[p.substr(0,found)])), d.id);
        if(std::find(i->files.begin(), i->files.end(), p) == i->files.end())
        {
            i->files.push_back(p.substr(found+1));
        }
        int in = getNewInode();

        filesInodes[p] = in;
        node->put(dht::InfoHash::get(to_string(in)), *i, [](bool success) {
        }, dht::time_point::max(), true);
    }else {
        if(p.compare("/")) {
            return -1;
        }
    }


    if(filesInodes.find(p) != filesInodes.end())
    {
        filesInodes[p] = inode;
    }else {
        filesInodes.insert(make_pair(p, inode));
    }

    if(p.compare("/")) {
        node->cancelPut(dht::InfoHash::get(files), filesValue->id);
    }
    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, filesInodes);
    dht::Value d ((uint8_t*)sbuf.data(), sbuf.size());
    filesValue = &d;
    node->put(dht::InfoHash::get(files), filesInodes, [](bool success) {
    }, dht::time_point::max(), true);
    return 0;
}

#ifndef PROJECT_P2PFS_H
#define PROJECT_P2PFS_H

#endif //PROJECT_P2PFS_H
