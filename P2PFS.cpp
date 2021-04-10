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


/*
 * Command line options
 *
 * We can't set default values for the char* fields here because
 * fuse_opt_parse would attempt to free() them when the user specifies
 * different values on the command line.
 */
int foo(int argc, char *argv[], dht::DhtRunner* node, pid_t* tid)
{
    FSpart fs; //Create the FUSE part object
    *tid = syscall(SYS_gettid); //Save the tid
    int status = fs.run(argc, argv, node); //Call our function that will in turn call fuse_main()
    return status;
}
void usage()
{
    cerr << "usage: p2pfs -new \"directory to mount\" \n or if you would like to connect to an existing network: p2pfs \"IP:port\" \"directory to mount\" \n";
    throw std::invalid_argument("Invalid syntax.");
    exit(EXIT_FAILURE);
}
int main(int argc, char *argv[])
{
    //Variables declarations
    pid_t* tid; //Thread id of FUSE thread
    tid = (pid_t*) malloc(sizeof(pid_t));
    bool newNetwork = false;


    string firstArg(argv[1]); //We pass the first argument to string
    //Regex to check the IP is of a valid format
    regex IPRegex("^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?):[0-9]+$");
    //Check if the first argument is valid
    if(firstArg.compare("-new") == 0) //If we want to create new network
    {
        cout << "Creating new network\n";
        newNetwork = true;
    }else if(!std::regex_match (firstArg,IPRegex)) //Otherwise check IP:port format correct
    {
        usage();
    }
    //Now we want to create the new array of arguments that will be passed to fuse_main()
    char ** FUSEArgs = new char* [argc-1];
    int j = 0;
    for(int i = 0; i<argc; i++)
    {
        if(i == 1) continue;
        FUSEArgs[j] = argv[i];
        j++;
    }

    //Create and run the DHT node
    dht::DhtRunner* node = new dht::DhtRunner;
    node->run(4222, dht::crypto::generateIdentity(), true);

    //If the user provided an IP and port, bootstrap to that network
    string delimiter = ":";
    if(!newNetwork)
    {
        //Extract IP and port from argument
        string IP = firstArg.substr(0, firstArg.find(delimiter));
        string port = firstArg.substr(firstArg.find(delimiter) + 1);
        //Bootstrap
        node->bootstrap(IP, port);
    }
    //Run the Fuse thread
    thread thread_obj(foo, argc-1, FUSEArgs, node, tid);

    sleep(5); //Run for 5 seconds (This is for automated testing purposes only)

    cout << "Exiting program";
    node->join(); //Join the DHT node
    kill(*tid, SIGTERM); //Kill the FUSE thread
    free(tid); //Free the allocation for the thread ID
    thread_obj.join(); //Join the FUSE thread
    return 0;

    // Join the network through any running node,
    // here using a known bootstrap node.
    //node.bootstrap("bootstrap.ring.cx", "4222");

    // put some data on the dht
    /*std::vector<uint8_t> some_data(5, 10);


    // put some data on the dht, signed with our generated private key
    node.putSigned("unique_key_42", some_data);

    // get data from the dht
    node.get("1", [](const std::vector<std::shared_ptr<dht::Value>>& values) {
        // Callback called when values are found
        for (const auto& value : values)
            std::cout << "Found value: " << unsigned((*value).data.at(1))  << std::endl;
        return true; // return false to stop the search
    });
    sleep(5);
    // wait for dht threads to end

    return 0;*/
}
