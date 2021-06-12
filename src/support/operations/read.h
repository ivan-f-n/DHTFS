#ifndef __READ_H_
#define __READ_H_

#include "../../FSpart.h"
#include "../global.h"
#include "../logger.h"
#include "../globalFunctions.h"

using namespace std;

int FSpart::read(const char *path, char *buf, size_t size, off_t offset,
                 struct fuse_file_info *)
{
    struct timeval tvalBefore, tvalAfter; // removed comma

    gettimeofday(&tvalBefore, NULL);
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
        return -ENOENT;
    }
    if (offset > i->st.st_size)
        return 0;
    if (offset + size > i->st.st_size)
        size = i->st.st_size - offset;
    Logger("Reading " + to_string(size) + " bytes from file " + string(path));
    int bytesRead = 0;
    while (bytesRead < size)
    {
        unsigned char *block;
        Logger("Reading the block number " + to_string((int)((offset + bytesRead) / i->st.st_blksize)));
        // get data from the dht
        runner->get(
            dht::InfoHash::get(i->files[(int)((offset + bytesRead) / i->st.st_blksize)]), [&](const shared_ptr<dht::Value> &value)
            {
                block = new unsigned char[(*value).data.size()];

                copy((*value).data.begin(), (*value).data.end(), block);
                return false; // return false to stop the search
            },
            [&](bool success) {
            },
            {}, {});
        time_t now = time(0);
        while (block == NULL && time(0) - 1 < now)
            ; //Wait until value is received or timeout

        if (block == NULL)
        {
            return -ENOENT;
        }
        int bytesToRead;
        if (bytesRead == 0)
        {
            if ((size + offset) < i->st.st_blksize)
            {
                bytesToRead = min((int)(offset % i->st.st_blksize + size), (int)(i->st.st_blksize)) - offset % i->st.st_blksize;
            }
            else
            {
                bytesToRead = i->st.st_blksize - offset % i->st.st_blksize;
            }
            memcpy(buf, block + offset % i->st.st_blksize, bytesToRead);
        }
        else
        {
            if ((size - bytesRead) < i->st.st_blksize)
            {
                bytesToRead = size - bytesRead;
            }
            else
            {
                bytesToRead = i->st.st_blksize;
            }
            memcpy(buf + bytesRead, block, bytesToRead);
        }
        bytesRead += bytesToRead;
        Logger("Bytes read so far:" + to_string(bytesRead));
    }
    gettimeofday(&tvalAfter, NULL);
    printf("Read took %d ms\n", diff_ms(tvalBefore, tvalAfter));
    return size;
}

#endif