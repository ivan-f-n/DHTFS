#ifndef __WRITE_H_
#define __WRITE_H_

#include "../../FSpart.h"
#include "../global.h"
#include "../logger.h"
#include "../globalFunctions.h"

using namespace std;

int FSpart::write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    struct timeval tvalBefore, tvalAfter; // removed comma

    std::mutex mtx;
    std::unique_lock<std::mutex> lk(mtx);
    std::condition_variable cv;

    gettimeofday(&tvalBefore, NULL);
    Logger("Wrtiting to file " + string(path));

    if (!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    string p(path);
    inodeFile *i = getInodeStruct(path, runner);
    if (i == NULL)
    {
        Logger("Writing file " + string(path) + " failed (not found)");
        return -ENOENT;
    }
    dht::Value d{*i};
    runner->cancelPut(dht::InfoHash::get(to_string(inodeMap[p])), d.id);

    if ((offset + size) > i->st.st_size)
    {
        truncate(path, offset + size, fi);
    }
    inodeFile *t = i;
    i = getInodeStruct(path, runner);
    if (i == NULL)
    {
        Logger("Writing file " + string(path) + " failed (not found)");
        return -ENOENT;
    }
    int bytesWritten = 0;
    int sizeSub = i->st.st_size == 0 ? 0 : 1;
    int offSub = offset == 0 ? 0 : 1;
    size_t fileSize = i->st.st_size;
    blksize_t blkSize = i->st.st_blksize;
    while (bytesWritten < size)
    {
        if (bytesWritten == 0)
        {
            unsigned char *block;
            bool s = false;
            int si;
            // get data from the dht
            runner->get(
                dht::InfoHash::get(i->files[(int)((offset + bytesWritten) / blkSize)]), [&](const shared_ptr<dht::Value> &value)
                {
                    si = (*value).data.size();
                    block = new unsigned char[si];
                    copy((*value).data.begin(), (*value).data.end(), block);
                    return false; // return false to stop the search
                },
                [&](bool success)
                {
                    s = success;
                },
                {}, {});
            time_t now = time(0);
            while (!s && time(0) - 1 < now)
                ; //Wait until value is received or timeout

            if (!s)
            {
                return -ENOENT;
            }
            memcpy(block + offset % blkSize, buf, min((int)size, (int)(blkSize - offset % blkSize)));
            int newI = genInode();
            i->files[(int)((offset + bytesWritten) / blkSize)] = to_string(newI);
            s = false;
            runner->put(
                dht::InfoHash::get(to_string(newI)), dht::Value(block, si), [&](bool success)
                {
                    std::lock_guard<std::mutex> l(mtx);
                    s = true;
                    cv.notify_all();
                },
                dht::time_point::max(), true);
            cv.wait(lk, [&]
                    { return s; });
            if (!s)
            {
                Logger("Writing file " + string(path) + "failed (not put)");
                return -ENOENT;
            }

            bytesWritten = min((int)size, (int)(blkSize - offset % blkSize));
        }
        else
        {
            unsigned char *block;
            bool s = false;
            int si;
            // get data from the dht
            runner->get(
                dht::InfoHash::get(i->files[(int)((offset + bytesWritten) / blkSize)]), [&](const shared_ptr<dht::Value> &value)
                {
                    si = (*value).data.size();
                    block = new unsigned char[si];
                    copy((*value).data.begin(), (*value).data.end(), block);
                    return false; // return false to stop the search
                },
                [&](bool success)
                {
                    s = success;
                },
                {}, {});
            time_t now = time(0);
            while (!s && time(0) - 1 < now)
                ; //Wait until value is received or timeout

            if (!s)
            {
                return -ENOENT;
            }
            memcpy(block, buf + bytesWritten, min((int)size - bytesWritten, (int)blkSize));
            int newI = genInode();
            i->files[(int)((offset + bytesWritten) / blkSize)] = to_string(newI);
            s = false;
            runner->put(
                dht::InfoHash::get(to_string(newI)), dht::Value(block, si), [&](bool success)
                {
                    std::lock_guard<std::mutex> l(mtx);
                    s = true;
                    cv.notify_all();
                },
                dht::time_point::max(), true);
            cv.wait(lk, [&]
                    { return s; });
            if (!s)
            {
                Logger("Writing file " + string(path) + "failed (not put)");
                return -ENOENT;
            }

            bytesWritten += min((int)size - bytesWritten, (int)blkSize);
        }
    }

    int newII = genInode();
    bool s = false;
    changeEntry(path, newII, runner);
    runner->put(
        dht::InfoHash::get(to_string(newII)), *i, [&](bool success)
        {
            std::lock_guard<std::mutex> l(mtx);
            s = true;
            cv.notify_all();
        },
        dht::time_point::max(), true);
    cv.wait(lk, [&]
            { return s; });
    if (!s)
    {
        Logger("Writing file " + string(path) + "failed (not put)");
        return -ENOENT;
    }
    gettimeofday(&tvalAfter, NULL);
    printf("Write took %d ms\n", diff_ms(tvalBefore, tvalAfter));
    return bytesWritten;
}
#endif