//
// Created by ifnav on 30/01/2020.
//

#include "FSpart.h"
#include "P2PFS.h"

using namespace std;
/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.
*/

/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall hello.c `pkg-config fuse3 --cflags --libs` -o hello
 *
 * ## Source code ##
 * \include hello.c
 */

static const string hello_str = "Hello World!\n";

static string root_path = "/";

/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
int foo(int argc, char *argv[], dht::DhtRunner *node, map<string, int> *fi, dht::Value *fiv, pid_t *tid)
{
    FSpart fs; //Create the FUSE part object
    cout << "FUSE thread" << endl;
    *tid = syscall(SYS_gettid);                     //Save the tid
    int status = fs.run(argc, argv, node, fi, fiv); //Call our function that will in turn call fuse_main()
    exit(status);
    return status;
}
void usage()
{
    cerr << "usage: p2pfs -new \"Directory to mount\" \n or if you would like to connect to an existing network: p2pfs \"IP:port\" \"Local port to bind to\"  \"Directory to mount\" \n";
    throw std::invalid_argument("Invalid syntax.");
    exit(EXIT_FAILURE);
}

void addRoot()
{

    inodeFile *i = new inodeFile();
    i->st.st_mode = S_IFDIR | 0755;
    i->st.st_uid = getuid();
    i->st.st_gid = getgid();
    i->st.st_nlink = 2;
    i->files.emplace_back(string("."));
    i->files.emplace_back(string(".."));
    int inode = getNewInode();
    editINODE(root_path.c_str(), inode);
    node->put(
        dht::InfoHash::get(to_string(inode)), *i, [](bool success) {
        },
        dht::time_point::max(), true);
}
int main(int argc, char *argv[])
{
    //Variables declarations
    pid_t *tid; //Thread id of FUSE thread
    tid = (pid_t *)malloc(sizeof(pid_t));
    bool newNetwork = false;

    string firstArg(argv[1]); //We pass the first argument to string
    //Regex to check the IP is of a valid format
    regex IPRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?):[0-9]+$");
    //Check if the first argument is valid
    int indexOfDirectory = 2;
    if (firstArg.compare("-new") == 0) //If we want to create new network
    {
        cout << "Creating new network\n";
        newNetwork = true;
        //Create and run the DHT node
        node = new dht::DhtRunner;
        try
        {
            node->run(4222, dht::crypto::generateIdentity(), true);
        }
        catch (exception e)
        {
            exit(-1);
        }
    }
    else if (!std::regex_match(firstArg, IPRegex)) //Otherwise check IP:port format correct
    {
        usage();
    }
    else
    {
        //Create and run the DHT node
        int secArg = atoi(argv[2]);
        node = new dht::DhtRunner;
        try
        {
            node->run(secArg, dht::crypto::generateIdentity(), true);
        }
        catch (exception e)
        {
            exit(-1);
        }
        indexOfDirectory = 3;
        cout << "Connected to network" << endl;
    }

    //Now we want to create the new array of arguments that will be passed to fuse_main()
    char **FUSEArgs = new char *[6];
    FUSEArgs[0] = argv[0];
    FUSEArgs[1] = "-f";
    FUSEArgs[2] = "-d";
    FUSEArgs[3] = "-o";
    FUSEArgs[4] = "allow_other";
    FUSEArgs[5] = argv[indexOfDirectory];


    //If the user provided an IP and port, bootstrap to that network
    string delimiter = ":";
    if (!newNetwork)
    {
        //Extract IP and port from argument
        string IP = firstArg.substr(0, firstArg.find(delimiter));
        string port = firstArg.substr(firstArg.find(delimiter) + 1);
        //Bootstrap
        node->bootstrap(IP, port);
    }
    else
    {
        addRoot();
    }
    
    //Run the Fuse thread
    cout << "Creating thread\n";

    thread thread_obj(foo, 6, FUSEArgs, node, &filesInodes, filesValue, tid);

    while (1);
    sleep(5); //Run for 5 seconds (This is for automated testing purposes only)

    cout << "Exiting program";
    node->join();        //Join the DHT node
    kill(*tid, SIGTERM); //Kill the FUSE thread
    free(tid);           //Free the allocation for the thread ID
    thread_obj.join();   //Join the FUSE thread
    return 0;

}
