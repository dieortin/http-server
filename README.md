Performant HTTP server in C
================

This project aims to build a high-performance HTTP server in C. It's built in an extremely
modular fashion, and makes use of a thread pool to achieve higher performance.

This is just an overview of the project, and more complete documentation can be
found in the wiki in this repository, as well as instructions on how to generate
the full documentation of this project.

## First steps
### Dependencies
* CMake 3.15 or higher
* libpthread

### Building the project

Starting in the root folder of the project (http-server if you just cloned the repository),
just run the following terminal commands:

```bash
$ cmake -S source -B build
$ cmake --build build/ --config Release
```

This creates a build system adapted for your computer using cmake in the `build directory,
and then uses it to compile the target *practica1*, which is the general executable of the
project.

The produced executables are then located in `source/build/bin`. Therefore, to execute the
program with the predefined options, you just need to execute the following command:

```bash
$ bin/practica1
```

### Configuring the server
The server's configuration is managed through a simple configuration file located in the 
root folder, which is called ```server.cfg```. Options follow the format `OPTION=VALUE`,
and each option must be in its own line. The simplest way to understand this format is
just having a look at the configuration file included in the repository.

The server currently supports the current options:

* `ADDRESS`: string representing the IP address in which the server should listen
* `PORT`: integer representing the port in which the server should listen
* `WEBROOT`: string representing the path of the webroot relative to the root folder of the project (keep in mind it should start with "/")
* `NTHREADS`: integer representing the number of threads that should be created for the server's thread pool.
* `QUEUE_SIZE`: integer representing the maximum number of clients that can be enqueued
* `MIME_FILE`: string representing the name of the file containing the MIME type associations required for serving files

The server parses this configuration file using a custom built module called *readconfig*, which
makes it very easy to add new supported parameters to the server, or different parameter types.

