#ifndef __TRUNCATE_H_
#define __TRUNCATE_H_

#include "../../FSpart.h"
#include "../global.h"
#include "../logger.h"
#include "../globalFunctions.h"

using namespace std;

int FSpart::truncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    Logger("Truncating file " + string(path) + " to " + to_string(offset));
    if (!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    string p(path);

    std::mutex mtx;
    std::unique_lock<std::mutex> lk(mtx);
    std::condition_variable cv;

    inodeFile *i = getInodeStruct(path, runner);

    int inode = genInode();
    if (i == NULL)
    {
        Logger("Truncating file " + string(path) + "failed (not found)");
        return -ENOENT;
    }
    int diff = offset - i->st.st_size;
    int sizeSub = i->st.st_size == 0 ? 0 : 1;
    int offSub = offset == 0 ? 0 : 1;
    size_t fileSize = i->st.st_size;
    blksize_t blkSize = i->st.st_blksize;
    if (offset > fileSize)
    {

        for (int j = 0; j <= (((int)((offset - offSub) / blkSize)) - (int)((fileSize - sizeSub) / blkSize)); j++)
        {

            if (j == 0)
            {
                unsigned char *block;
                bool s = false;
                int siz = 0;
                // get data from the dht
                runner->get(
                    dht::InfoHash::get(i->files[(int)((fileSize - sizeSub) / blkSize)]), [&](const shared_ptr<dht::Value> &value)
                    {
                        block = new unsigned char[(*value).data.size()];
                        siz = (*value).data.size();

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
                unsigned char *b;
                int sb = 0;
                if (((int)((offset - offSub) / blkSize)) - ((int)((fileSize - sizeSub) / blkSize)) == 0)
                {
                    sb = ((offset - offSub) % blkSize) + 1;
                    b = new unsigned char[sb];
                    memcpy(b, block, siz);
                    if (siz != sb)
                        memset(b + siz, 0, (sb - siz));
                }
                else
                {
                    sb = blkSize;
                    b = new unsigned char[sb];
                    memcpy(b, block, siz);
                    memset(b + siz, 0, blkSize - siz);
                }
                int newI = genInode();
                bool successful = false;
                i->files[(int)((fileSize - sizeSub) / (blkSize))] = to_string(newI);
                runner->put(
                    dht::InfoHash::get(to_string(newI)), dht::Value(b, sb), [&](bool success)
                    {
                        std::lock_guard<std::mutex> l(mtx);
                        successful = true;
                        cv.notify_all();
                    },
                    dht::time_point::max(), true);
                cv.wait(lk, [&]
                        { return successful; });
            }
            else
            {
                int firstInode = genInode();
                i->files.emplace_back(to_string(firstInode));
                unsigned char *b;
                int sb = 0;
                if ((int)((offset - offSub) / blkSize) - (int)((fileSize - sizeSub) / blkSize) == j)
                {
                    sb = ((offset - offSub) % (blkSize)) + 1;
                }
                else
                {
                    sb = blkSize;
                }
                b = new unsigned char[sb];
                memset(b, 0, sb);
                bool successful = false;
                runner->put(
                    dht::InfoHash::get(to_string(firstInode)), dht::Value(b, sb), [&](bool success)
                    {
                        std::lock_guard<std::mutex> l(mtx);
                        successful = true;
                        cv.notify_all();
                    },
                    dht::time_point::max(), true);
                cv.wait(lk, [&]
                        { return successful; });
            }
        }
        i->st.st_size = offset;
    }
    else
    {
        for (int j = 0; j >= ((int)(offset / (blkSize + 1)) - (int)(fileSize / (blkSize + 1))); j--)
        {

            if (j == ((int)(offset / (blkSize + 1)) - (int)(fileSize / (blkSize + 1))))
            {
                unsigned char *block;
                bool s = false;
                // get data from the dht
                runner->get(
                    dht::InfoHash::get(i->files[(int)(fileSize / (blkSize + 1))]), [&](const shared_ptr<dht::Value> &value)
                    {
                        block = new unsigned char[(*value).data.size()];
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
                    return 0;
                }
                unsigned char *b;
                int sb = 0;
                sb = offset % (blkSize + 1);
                b = new unsigned char[sb];
                memcpy(b, block, offset % (blkSize + 1));

                int newI = genInode();
                bool successful = false;
                i->files[(int)(offset / (blkSize + 1))] = to_string(newI);
                runner->put(
                    dht::InfoHash::get(to_string(newI)), dht::Value(b, sb), [&](bool success)
                    {
                        std::lock_guard<std::mutex> l(mtx);
                        successful = true;
                        cv.notify_all();
                    },
                    dht::time_point::max(), true);
                cv.wait(lk, [&]
                        { return successful; });
            }
            else
            {
                i->files.pop_back();
            }
        }
        i->st.st_size = offset;
    }

    changeEntry(path, inode, runner);
    bool successful = false;
    runner->put(
        dht::InfoHash::get(to_string(inode)), *i, [&](bool success)
        {
            std::lock_guard<std::mutex> l(mtx);
            successful = true;
            cv.notify_all();
        },
        dht::time_point::max(), true);
    cv.wait(lk, [&]
            { return successful; });

    if (!successful)
    {
        Logger("Truncating file " + string(path) + "failed (not put)");
        return -ENOENT;
    }
    return offset;
}

#endif