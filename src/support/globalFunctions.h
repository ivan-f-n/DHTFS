

#ifndef __GLFUNC_H_
#define __GLFUNC_H_

#include "global.h"
#include "logger.h"

using namespace std;
static string fileStr;
dht::Value *valuePtr;
static int inst = 0;
dht::DhtRunner *runner;
map<string, int> inodeMap;

map<string, int> getMap()
{
    return inodeMap;
}

int diff_ms(timeval t1, timeval t2)
{
    return (((t1.tv_sec - t2.tv_sec) * 1000000) +
            (t1.tv_usec - t2.tv_usec)) /
           1000;
}

int getInodeBack(const char *path)
{
    string p(path);
    if (inodeMap.find(p) != inodeMap.end())
    {
        return inodeMap[p];
    }
    else
    {
        return -1;
    }
}

int genInode()
{
    return rand() % INT_MAX;
}

inodeFile *getInodeStruct(const char *path, dht::DhtRunner *dhtNode)
{
    Logger("Get inode struct start");
    // get data from the dht
    if (getInodeBack(path) == -1)
        return NULL;
    inodeFile *i = new inodeFile();
    bool s = false;
    dhtNode->get<inodeFile>(dht::InfoHash::get(to_string(getInodeBack(path))), [&](inodeFile &&myObject)
                            {
                                *i = myObject;
                                s = true;
                                return false; // return false to stop the search
                            });

    time_t now = time(0); //Save current time
    while (!s && time(0) - 1 < now); //Wait until value is received or timeout
    Logger("Get inode struct end");
    if(!s)
        return NULL;
    return i;
}

int changeEntry(const char *path, int inode, dht::DhtRunner *dhtNode)
{
    string p(path);
    size_t found;

    std::mutex mtx;
    std::unique_lock<std::mutex> lk(mtx);
    std::condition_variable cv;

    found = p.find_last_of("/");
    string dir = p.substr(0, found);
    if (!dir.compare(""))
    {
        dir = "/";
    }
    std::map<string, int>::iterator it = inodeMap.find(dir);
    if (it != inodeMap.end())
    {
        inodeFile *i = getInodeStruct(dir.c_str(), dhtNode);
        dht::Value d{*i};
        //dhtNode->cancelPut(dht::InfoHash::get(to_string(inodeMap[dir])), d.id);
        if (find(i->files.begin(), i->files.end(), p.substr(found + 1)) == i->files.end())
        {
            i->files.emplace_back(p.substr(found + 1));
        }
        int in = genInode();

        inodeMap[dir] = in;
        bool s = false;
        dhtNode->put(
            dht::InfoHash::get(to_string(in)), *i, [&](bool success)
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
    else
    {
        return -1;
    }

    std::map<string, int>::iterator it2 = inodeMap.find(p);
    if (it2 != inodeMap.end())
    {
        inodeMap[p] = inode;
    }
    else
    {
        inodeMap.insert(make_pair(p, inode));
    }

    dhtNode->cancelPut(dht::InfoHash::get(fileStr), valuePtr->id);

    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, inodeMap);
    dht::Value d((uint8_t *)sbuf.data(), sbuf.size());
    valuePtr = &d;
    bool s = false;
    dhtNode->put(
        dht::InfoHash::get(fileStr), inodeMap, [&](bool success)
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
    return 0;
}
#endif