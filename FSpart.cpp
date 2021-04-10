// Hello filesystem class implementation

#include "FSpart.h"
#include "P2PFS.h"

#include <iostream>
#include <fstream>
#include <string>


using namespace std;

static const string root_path = "/";
static const string hello_str = "Hello World!\n";
static const string hello_path = "/hello";
static string logfile =""; //Here we will save this session's log file path


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

    logfile = (logfile=="")?"/home/ivan/log/log_"+getCurrentDateTime("date")+".txt":logfile; //Save this session's log file path
    string filePath = logfile;
    string now = getCurrentDateTime("now"); //Get current time from our function

    //Log the message next to the current timestamp in our file
    ofstream ofs(filePath.c_str(), ios_base::out | ios_base::app );
    ofs << now << '\t' << logMsg << '\n';
    ofs.close();
}

int FSpart::getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (path == root_path) {
        Logger("reading root directory attributes");
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (path == hello_path) {
        Logger("reading file attributes");
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = hello_str.length();
	} else
		res = -ENOENT;

	return res;
}

int FSpart::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
    Logger("reading root directory");
	if (path != root_path)
		return -ENOENT;

	filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, hello_path.c_str() + 1, NULL, 0, FUSE_FILL_DIR_PLUS);

	return 0;
}


int FSpart::open(const char *path, struct fuse_file_info *fi)
{
    Logger("opening file");

    dht::DhtRunner* node = Fuse::this_();
    node->put("1", dht::Value((const uint8_t*)hello_str.data(), hello_str.size()));
    // get data from the dht
    node->get("1", [&](const std::vector<std::shared_ptr<dht::Value>>& values) {
        // Callback called when values are found
        for (const auto& value : values)
            Logger("Found value: " + unsigned((*value).data.at(1)));
        return true; // return false to stop the search
    });
	if (path != hello_path)
		return -ENOENT;
    Logger("file path is hello_path");
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}


int FSpart::read(const char *path, char *buf, size_t size, off_t offset,
		              struct fuse_file_info *)
{

	if (path != hello_path)
		return -ENOENT;
    Logger("reading file");
    std::string the_data_from_value = "";
    std::string empty_string = "";
    std::string hello_str = "Hello World!\n";
    dht::DhtRunner* node = Fuse::this_();

    // get data from the dht
    node->get("1", [&](const vector<std::shared_ptr<dht::Value>>& values) {
        // Callback called when values are found
        for (const auto& value : values) {
            Logger("value received");
            the_data_from_value = {(*value).data.begin(), (*value).data.end()};
        }
        return true; // return false to stop the search
    });
    while(the_data_from_value==empty_string);
    size_t len;
    len = the_data_from_value.length();
    if ((size_t)offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, the_data_from_value.c_str() + offset, size);
    } else {
        size = 0;
    }


	return size;
}
