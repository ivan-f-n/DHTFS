#include "support/global.h"

map<string, int> filesInodes;
dht::Value* filesValue = NULL;
static const string files = "fileStr";
dht::DhtRunner* node;

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
