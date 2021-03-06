# Peer-to-peer Filesystem by Ivan Fernandez

[![Build Status](https://www.travis-ci.com/ivan-f-n/DHTFS.svg?branch=master)](https://www.travis-ci.com/ivan-f-n/DHTFS)

A P2P Fuse user-space filesystem that uses a DHT for distributing and storing the files.

## Installation

Prior to installing P2PFS, you will need to install its dependencies:

-   [OpenDHT](https://github.com/savoirfairelinux/opendht)
-   [Fuse](https://github.com/libfuse/libfuse), which you can get from its GitHub and install it with their instructions. The version must be > 3.0

After installing all the dependencies of the program, you just run CMake on this root directory and that will compile the program:

```bash
make
```

## Usage

```bash
./p2pfs "IP:Port of known node of the network" "Local port to bind" "Directory to mount"
```

or, if theres no network in place yet:

```bash
./p2pfs -new "Directory to mount"
```

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License

[MIT](https://choosealicense.com/licenses/mit/)
