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
int foo(int argc, char *argv[], dht::DhtRunner* node)
{
    // Do something
    HelloFS fs;
    int status = fs.run(argc, argv, node);
    return status;
}
int main(int argc, char *argv[])
{
    cout << "Start\n";
    dht::DhtRunner* node = new dht::DhtRunner;
    std::thread thread_obj(foo, argc,argv,node);
    cout << "thread\n";
    node->run(4222, dht::crypto::generateIdentity(), true);

    cout << "Hey\n";
    int g = 0;
    while(g!=1)
    {
        g=0;
        cin >> g;
    }
    thread_obj.join();
    cout << "Hey\n";
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
