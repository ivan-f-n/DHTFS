// Hello filesystem class implementation

#include "FSpart.h"

#include <vector>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <msgpack.hpp>


using namespace std;


static int inst = 0;
map<string, int> inodeMap;
dht::Value* valuePtr;
static string fileStr;
dht::DhtRunner* runner;

std::mutex mtx;
std::unique_lock<std::mutex> lk(mtx);
std::condition_variable cv;

static string logfile =""; //Here we will save this session's log file path


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

inline string getCurrentDateTime( string s ){
    time_t now = time(0); //Save current time
    struct tm  tstruct;
    char  buf[80];
    tstruct = *localtime(&now);
    //Either calculate the timestamp now or of just the date
    if(s=="now")
        strftime(buf, sizeof(buf), "%Y-%m-%d-%T %X", &tstruct);
    else if(s=="date")
        strftime(buf, sizeof(buf), "%Y-%m-%d-%T", &tstruct);
    return string(buf);
}

inline void Logger( string logMsg ){
    cout << "Logging" << endl;
    logfile = (logfile=="")?"/home/ivan/log/log_"+getCurrentDateTime("date")+".txt":logfile; //Save this session's log file path
    string filePath = logfile;
    string now = getCurrentDateTime("now"); //Get current time from our function

    //Log the message next to the current timestamp in our file
    ofstream ofs(filePath.c_str(), ios_base::out | ios_base::app );
    ofs << now << '\t' << logMsg << '\n';
    ofs.close();
    cout << "Logged" << endl;
}
//Class that for each file contains:
//-Directory: stbuf struct and the files is the directory
//-Regular file: stbuf struct and the inodes (id) of the blocks of the file
class inodeFiles {
public:
    struct statSerial st;
    vector<string> files;
    inodeFiles(){};
    MSGPACK_DEFINE(st, files);

    inodeFiles& operator=(inodeFiles other) {
        st = other.st;
        files = other.files;
        return *this;
    }

    operator string() {
        string stringOfStruct;
        cout << "stringOfStruct" << endl;

        char buffer[100];
        sprintf(buffer, "%lu", (unsigned long)st.st_ino);
        stringOfStruct += "\nInode number" + string(buffer);

        char buffer2[100];
        sprintf(buffer2, "%u", (unsigned int)st.st_uid);
        stringOfStruct += "\nUser ID owner" + string(buffer2);

        char buffer3[100];
        sprintf(buffer3, "%ld", (long)st.st_size);
        stringOfStruct += "\n Size" + string(buffer3);

        cout << "stringOfStruct2" << endl;

        stringOfStruct += "\n Files: ";
        for(int i = 0; i<files.size(); i++) {
            stringOfStruct += "\n" + files.at(i);
        }
        cout << stringOfStruct << endl;
        return stringOfStruct;
    }
};

map<string, int> getMap()
{
    return inodeMap;
}

int getInodeBack(const char* path)
{
    string p(path);
    if(inodeMap.find(p) != inodeMap.end())
    {
        return inodeMap[p];
    }else {
        return -1;
    }
}

int genInode()
{
    return rand() % INT_MAX;
}

inodeFiles* getInodeStruct(const char* path)
{
    Logger("Get inode struct start");
    inodeFiles* i = new inodeFiles();
    // get data from the dht
    if(getInodeBack(path) == -1)    return NULL;
    runner->get<inodeFiles>(dht::InfoHash::get(to_string(getInodeBack(path))), [&](inodeFiles&& myObject) {
        *i = myObject;
        return false; // return false to stop the search
    });

    time_t now = time(0); //Save current time
    mode_t s = i->st.st_mode;
    while(i->st.st_mode == s && time(0)-5 < now); //Wait until value is received or timeout
    Logger("Get inode struct end");
    return i;
}

int changeEntry(const char* path, int inode)
{
    string p(path);
    size_t found;
    found=p.find_last_of("/");
    string dir = p.substr(0,found);
    if(!dir.compare(""))
    {
        dir = "/";
    }
    std::map<string, int>::iterator it = inodeMap.find(dir);
    if(it != inodeMap.end())
    {
        inodeFiles* i = getInodeStruct(dir.c_str());
        dht::Value d {*i};
        //runner->cancelPut(dht::InfoHash::get(to_string(inodeMap[dir])), d.id);
        if(find(i->files.begin(), i->files.end(), p.substr(found+1)) == i->files.end())
        {
            i->files.emplace_back(p.substr(found+1));
        }
        int in = genInode();

        inodeMap[dir] = in;
        bool s = false;
        runner->put(dht::InfoHash::get(to_string(in)), *i, [&](bool success) {
            std::lock_guard<std::mutex> l(mtx);
            s = true;
            cv.notify_all();
        }, dht::time_point::max(), true);
        cv.wait(lk, [&]{ return s; });
        if(!s) return -1;
    }else {
        return -1;
    }

    std::map<string, int>::iterator it2 = inodeMap.find(p);
    if(it2 != inodeMap.end())
    {
        inodeMap[p] = inode;
    }else {
        inodeMap.insert(make_pair(p, inode));
    }

    runner->cancelPut(dht::InfoHash::get(fileStr), valuePtr->id);

    msgpack::sbuffer sbuf;
    msgpack::pack(sbuf, inodeMap);
    dht::Value d ((uint8_t*)sbuf.data(), sbuf.size());
    valuePtr = &d;
    bool s = false;
    runner->put(dht::InfoHash::get(fileStr), inodeMap, [&](bool success) {
        std::lock_guard<std::mutex> l(mtx);
        s = true;
        cv.notify_all();
    }, dht::time_point::max(), true);
    cv.wait(lk, [&]{ return s; });
    if(!s) return -1;
    return 0;
}

int FSpart::getattr(const char *path, struct stat *stbuf, struct fuse_file_info * fi)
{
    Logger("Getting attributes for "+ string(path));
    if(!inst)
    {
        cout << "Create instance" << endl;

        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    int res = 0;
    inodeFiles* i = getInodeStruct(path);
    if(i==NULL)
    {
        Logger(string(path) + " not found");
        return -ENOENT;
    }else{
        Logger(string(path) + " found");
        Logger("Path  "+ string(path) + ": " + string(*i));
        memset(stbuf, 0, sizeof(struct stat));
        time_t now = time(0); //Save current time
        stbuf->st_size = i->st.st_size;
        stbuf->st_dev = i->st.st_dev;     /* ID of device containing file */
        stbuf->st_ino = i->st.st_ino;     /* inode number */
        stbuf->st_mode = i->st.st_mode;    /* protection */
        stbuf->st_nlink = i->st.st_nlink;   /* number of hard links */
        stbuf->st_uid = i->st.st_uid;     /* user ID of owner */
        stbuf->st_gid = i->st.st_gid;     /* group ID of owner */
        stbuf->st_rdev = i->st.st_rdev;    /* device ID (if special file) */
        stbuf->st_blksize = i->st.st_blksize; /* blocksize for file system I/O */
        stbuf->st_blocks = i->st.st_blocks;  /* number of 512B blocks allocated */
        stbuf->st_atime = now;   /* time of last access */
        stbuf->st_ctime = now;   /* time of last status change */
    }
	return res;
}


int diff_ms(timeval t1, timeval t2)
{
    return (((t1.tv_sec - t2.tv_sec) * 1000000) + 
            (t1.tv_usec - t2.tv_usec))/1000;
}
int FSpart::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info * fi, enum fuse_readdir_flags)
{
    Logger("Reading directory " + string(path));
    if(!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    inodeFiles* i = getInodeStruct(path);
    if(i==NULL)
    {
        Logger(string(path) + " not found");
        return -ENOENT;
    }else {
        if (S_ISREG(i->st.st_mode)) {
            return -ENOENT;
        } else {
            for (string f : i->files) {
                    filler(buf, f.c_str(), NULL, 0, FUSE_FILL_DIR_PLUS);
            }
        }
    }

	return 0;
}

int FSpart::mkdir(const char * path, mode_t mode)
{
    Logger("Making directory " + string(path));
    if(!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    inodeFiles* i = getInodeStruct(path);
    if(i!=NULL)
    {
        return -ENOENT;
    }else{
        inodeFiles* i = new inodeFiles();
        i->st.st_mode = S_IFDIR | 0755;
        i->st.st_uid = getuid();
        i->st.st_gid = getgid();
        i->st.st_nlink = 2;
        i->files.emplace_back(string("."));
        i->files.emplace_back(string(".."));
        int inode = genInode();
        changeEntry(path, inode);
        bool s = false;
        runner->put(dht::InfoHash::get(to_string(inode)), *i, [&](bool success) {
            std::lock_guard<std::mutex> l(mtx);
            s = true;
            cv.notify_all();
        }, dht::time_point::max(), true);

        cv.wait(lk, [&]{ return s; });
        if(!s) return -1;
    }
    return 0;
}



int FSpart::open(const char *path, struct fuse_file_info *fi)
{
    Logger("Opening file " + string(path));
    if(!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    inodeFiles* i = getInodeStruct(path);
    if(i==NULL)
    {
        return -ENOENT;
    }
		

	return 0;
}

int FSpart::create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    Logger("Creating file " + string(path) + "with mode: " + to_string(mode));
    if(!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    inodeFiles* i = getInodeStruct(path);
    if(i!=NULL)
    {
        return -ENOENT;
    }else{
        inodeFiles* i = new inodeFiles();
        i->st.st_mode = mode;
        i->st.st_size = 1;
        i->st.st_nlink = 1;
        i->st.st_uid = getuid();
        i->st.st_gid = getgid();
        i->st.st_blksize = pow(2, 12);
        int firstInode = genInode();
        i->files.push_back(to_string(firstInode));
        unsigned char* buf = new unsigned char[1];
        buf[0] = '\0';
        int inode = genInode();
        changeEntry(path, inode);
        bool s = false;
        runner->put(dht::InfoHash::get(to_string(inode)), *i, [&](bool success) {
            std::lock_guard<std::mutex> l(mtx);
            s = true;
            cv.notify_all();
        }, dht::time_point::max(), true);
        runner->put(dht::InfoHash::get(to_string(firstInode)), dht::Value(buf, 1), [&](bool success) {
        }, dht::time_point::max(), true);
        cv.wait(lk, [&]{ return s; });
        if(!s) return -ENOENT;
    }
    return 0;
}

int FSpart::read(const char *path, char *buf, size_t size, off_t offset,
		              struct fuse_file_info *)
{
    struct timeval tvalBefore, tvalAfter;  // removed comma

    gettimeofday (&tvalBefore, NULL);
    Logger("Reading "+to_string(size)+" bytes from file " + string(path));
    if(!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    string p (path);
    
    inodeFiles* i = getInodeStruct(path);
    if(i == NULL)
    {
        return -ENOENT;
    }
    if(offset > i->st.st_size)  return 0;
    if(offset+size > i->st.st_size) size = i->st.st_size - offset;
    int bytesRead = 0;
    while(bytesRead < size)
    {
        unsigned char* block;
        // get data from the dht
        runner->get(dht::InfoHash::get(i->files[(int) (offset+bytesRead / i->st.st_blksize)]), [&](const shared_ptr<dht::Value>& value) {
            block = new unsigned char[(*value).data.size()];
            
            copy((*value).data.begin(), (*value).data.end(), block);
            return false; // return false to stop the search
        }, [&](bool success) {
        }, {}, {});
        time_t now = time(0);
        while(block==NULL && time(0)-5 < now); //Wait until value is received or timeout

        if(block==NULL)
        {
            return -ENOENT;
        }
        if(bytesRead==0)
        {
            if((size+offset-bytesRead) < i->st.st_blksize)
            {
                memcpy(buf+bytesRead, block + offset%i->st.st_blksize, min((int)(offset%i->st.st_blksize + size), (int)(i->st.st_blksize)) - offset%i->st.st_blksize);
                bytesRead += (min((int)(offset%i->st.st_blksize + size), (int)(i->st.st_blksize)) - offset%i->st.st_blksize);
            } else {
                memcpy(buf+bytesRead, block + offset%i->st.st_blksize, i->st.st_blksize - offset%i->st.st_blksize);
                bytesRead += (i->st.st_blksize - offset%i->st.st_blksize);
            }
        }else{
            if((size-bytesRead) < i->st.st_blksize)
            {
                memcpy(buf+bytesRead, block, (size-bytesRead));
                bytesRead += (size-bytesRead);
            } else {
                memcpy(buf+bytesRead, block, i->st.st_blksize);
                bytesRead += (i->st.st_blksize);
            }
        }
        
    }
    gettimeofday (&tvalAfter, NULL);
    printf("Read took %d ms\n", diff_ms(tvalBefore, tvalAfter));
	return size;
}

int FSpart::truncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    Logger("Truncating file " + string(path) + " to " + to_string(offset));
    if(!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    string p(path);
    
    inodeFiles* i = getInodeStruct(path);
    
    int inode = genInode();
    if(i == NULL)
    {
        Logger("Truncating file " + string(path) + "failed (not found)");
        return -ENOENT;
    }
    int diff = offset - i->st.st_size;
    int sizeSub = i->st.st_size==0 ? 0 : 1;
    int offSub = offset==0 ? 0 : 1;
    size_t fileSize = i->st.st_size;
    blksize_t blkSize = i->st.st_blksize;
    if(offset>fileSize)
    {
        
        for(int j = 0; j <= (((int)((offset-offSub)/blkSize))-(int)((fileSize-sizeSub)/blkSize)); j++)
        {
            
            if(j==0)
            {
                unsigned char *block;
                bool s = false;
                int siz = 0;
                // get data from the dht
                runner->get(dht::InfoHash::get(i->files[(int)((fileSize-sizeSub)/blkSize)]), [&](const shared_ptr<dht::Value>& value) {
                    block = new unsigned char[(*value).data.size()];
                    siz = (*value).data.size();
                    
                    copy((*value).data.begin(), (*value).data.end(), block);
                    return false; // return false to stop the search
                },[&](bool success) { s = success;
                }, {}, {});
                time_t now = time(0);
                while (!s && time(0) - 5 < now); //Wait until value is received or timeout

                if (!s) {
                    return -ENOENT;
                }
                unsigned char* b;
                int sb = 0;
                if(((int)((offset-offSub)/blkSize))-((int)((fileSize-sizeSub)/blkSize)) == 0)
                {
                    sb = ((offset-offSub) % blkSize)+1;
                    b = new unsigned char[sb];
                    memcpy(b, block, siz);
                    if(siz != sb)
                        memset(b+siz, 0, (sb-siz));
                }else{
                    sb = blkSize;
                    b = new unsigned char[sb];
                    memcpy(b, block, siz);
                    memset(b+siz, 0, blkSize - siz);
                    
                }
                int newI = genInode();
                bool successful = false;
                i->files[(int) ((fileSize-sizeSub)/(blkSize))] = to_string(newI);
                runner->put(dht::InfoHash::get(to_string(newI)), dht::Value(b, sb), [&](bool success) {
                    std::lock_guard<std::mutex> l(mtx);
                    successful = true;
                    cv.notify_all();
                }, dht::time_point::max(), true);
                cv.wait(lk, [&]{ return successful; });
                
            }else{
                int firstInode = genInode();
                i->files.emplace_back(to_string(firstInode));
                unsigned char* b;
                int sb = 0;
                if((int)((offset-offSub)/blkSize)-(int)((fileSize-sizeSub)/blkSize) == j)
                {
                    sb = ((offset-offSub) % (blkSize))+1;
                    
                }else{
                    sb = blkSize;
                }
                b = new unsigned char[sb];
                memset(b, 0, sb);
                bool successful = false;
                runner->put(dht::InfoHash::get(to_string(firstInode)), dht::Value(b, sb), [&](bool success) {
                    std::lock_guard<std::mutex> l(mtx);
                    successful = true;
                    cv.notify_all();
                }, dht::time_point::max(), true);
                cv.wait(lk, [&]{ return successful; });
            }
            
        }
        i->st.st_size = offset;
    }else{
        for(int j = 0; j >= ((int)(offset/(blkSize+1))-(int)(fileSize/(blkSize+1))); j--)
        {
            
            if(j==((int)(offset/(blkSize+1))-(int)(fileSize/(blkSize+1))))
            {
                unsigned char *block;
                bool s = false;
                // get data from the dht
                runner->get(dht::InfoHash::get(i->files[(int)(fileSize/(blkSize+1))]), [&](const shared_ptr<dht::Value>& value) {
                    block = new unsigned char[(*value).data.size()];
                    copy((*value).data.begin(), (*value).data.end(), block);
                    return false; // return false to stop the search
                },[&](bool success) { s = success;
                }, {}, {});
                time_t now = time(0);
                while (!s && time(0) - 5 < now); //Wait until value is received or timeout

                if (!s) {
                    return 0;
                }
                unsigned char* b;
                int sb = 0;
                sb = offset%(blkSize+1);
                b = new unsigned char[sb];
                memcpy(b, block, offset%(blkSize+1));
                
                int newI = genInode();
                bool successful = false;
                i->files[(int) (offset / (blkSize+1))] = to_string(newI);
                runner->put(dht::InfoHash::get(to_string(newI)), dht::Value(b, sb), [&](bool success) {
                    std::lock_guard<std::mutex> l(mtx);
                    successful = true;
                    cv.notify_all();
                }, dht::time_point::max(), true);
                cv.wait(lk, [&]{ return successful; });
                
            }else{
                i->files.pop_back();
            }
            
        }
        i->st.st_size = offset;
    }
    
    changeEntry(path, inode);
    bool successful = false;
    runner->put(dht::InfoHash::get(to_string(inode)), *i, [&](bool success) { 
        std::lock_guard<std::mutex> l(mtx);
        successful = true;
        cv.notify_all();
    }, dht::time_point::max(), true);
    cv.wait(lk, [&]{ return successful; });
    
    if(!successful)
    {
        Logger("Truncating file " + string(path) + "failed (not put)");
        return -ENOENT;
    }
    return offset;
    
}


int FSpart::write(const char * path, const char * buf, size_t size, off_t offset,struct fuse_file_info * fi)
{
    struct timeval tvalBefore, tvalAfter;  // removed comma

    gettimeofday (&tvalBefore, NULL);
    Logger("Wrtiting to file " + string(path));
    
    if(!inst)
    {
        inodeMap = Fuse::this_()->fi;
        valuePtr = Fuse::this_()->fiv;
        fileStr = "fileStr";
        runner = Fuse::this_()->n;
        inst = 1;
    }
    string p(path);
    inodeFiles* i = getInodeStruct(path);
    if(i == NULL)
    {
        Logger("Writing file " + string(path) + " failed (not found)");
        return -ENOENT;
    }
    dht::Value d {*i};
    runner->cancelPut(dht::InfoHash::get(to_string(inodeMap[p])), d.id);

    if((offset+size)>i->st.st_size)
    {
        truncate(path, offset+size, fi);
    }
    inodeFiles* t = i;
    i = getInodeStruct(path);
    if(i == NULL)
    {
        Logger("Writing file " + string(path) + " failed (not found)");
        return -ENOENT;
    }
    int bytesWritten = 0;
    int sizeSub = i->st.st_size==0 ? 0 : 1;
    int offSub = offset==0 ? 0 : 1;
    size_t fileSize = i->st.st_size;
    blksize_t blkSize = i->st.st_blksize;
    while(bytesWritten < size)
    {
        if(bytesWritten == 0)
        {
            unsigned char *block;
            bool s = false;
            int si;
            // get data from the dht
            runner->get(dht::InfoHash::get(i->files[(int)((offset + bytesWritten)/blkSize)]), [&](const shared_ptr<dht::Value>& value) {
                si = (*value).data.size();
                block = new unsigned char[si];
                copy((*value).data.begin(), (*value).data.end(), block);
                return false; // return false to stop the search
            },[&](bool success) { s = success;
            }, {}, {});
            time_t now = time(0);
            while (!s && time(0) - 5 < now); //Wait until value is received or timeout

            if (!s) {
                return -ENOENT;
            }
            memcpy(block + offset%blkSize, buf, min((int)size,(int) (blkSize-offset%blkSize)));
            int newI = genInode();
            i->files[(int) ((offset + bytesWritten) / blkSize)] = to_string(newI);
            s = false;
            runner->put(dht::InfoHash::get(to_string(newI)), dht::Value(block, si), [&](bool success) {
                std::lock_guard<std::mutex> l(mtx);
                s = true;
                cv.notify_all();
            }, dht::time_point::max(), true);
            cv.wait(lk, [&]{ return s; });
            if(!s)
            {
                Logger("Writing file " + string(path) + "failed (not put)");
                return -ENOENT;
            }
            
            bytesWritten += min((int)size,(int) (blkSize-offset%blkSize));
        }
        else {
            unsigned char *block;
            bool s = false;
            int si;
            // get data from the dht
            runner->get(dht::InfoHash::get(i->files[(int)((offset + bytesWritten)/blkSize)]), [&](const shared_ptr<dht::Value>& value) {
                si = (*value).data.size();
                block = new unsigned char[si];
                copy((*value).data.begin(), (*value).data.end(), block);
                return false; // return false to stop the search
            },[&](bool success) { s = success;
            }, {}, {});
            time_t now = time(0);
            while (!s && time(0) - 5 < now); //Wait until value is received or timeout

            if (!s) {
                return -ENOENT;
            }
            memcpy(block, buf, min((int)size-bytesWritten,(int) blkSize));
            int newI = genInode();
            i->files[(int) ((offset + bytesWritten) / blkSize)] = to_string(newI);
            s = false;
            runner->put(dht::InfoHash::get(to_string(newI)), dht::Value(block, si), [&](bool success) {
                std::lock_guard<std::mutex> l(mtx);
                s = true;
                cv.notify_all();
            }, dht::time_point::max(), true);
            cv.wait(lk, [&]{ return s; });
            if(!s)
            {
                Logger("Writing file " + string(path) + "failed (not put)");
                return -ENOENT;
            }           
            
            bytesWritten += min((int)size-bytesWritten, (int)blkSize); 
        }
    }
    
    int newII = genInode();
    bool s = false;
    changeEntry(path, newII);
    runner->put(dht::InfoHash::get(to_string(newII)), *i, [&](bool success) {
        std::lock_guard<std::mutex> l(mtx);
        s = true;
        cv.notify_all();
            }, dht::time_point::max(), true);
    cv.wait(lk, [&]{ return s; });
    if(!s)
    {
        Logger("Writing file " + string(path) + "failed (not put)");
        return -ENOENT;
    }
    gettimeofday (&tvalAfter, NULL);
    printf("Write took %d ms\n", diff_ms(tvalBefore, tvalAfter));
    return bytesWritten;
}