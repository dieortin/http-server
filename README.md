Performant HTTP server in C
================

*Documentation in progress*

This project aims to build a high-performance HTTP server in C. It's built in an extremely
modular fashion, and makes use of a thread pool to achieve higher performance.

This is just an overview of the project, and a complete and detailed documentation can be
obtained using doxygen. To obtain it, just execute the command `$ doxygen doxygen-conf` from 
the root folder of the project, and a `docs` folder containing the full documentation in
both HTML and Latex will be created (you can even view this html documentation using this
project)

## First steps
### Building the project

Starting in the root folder of the project (practica-1 if you just cloned the repository),
just run the following terminal commands:

```bash
$ cd source/build
$ cmake ..
$ make practica1
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

### Changing log verbosity
The server automatically logs useful data together with the date and time when it was logged.
For example, an HTTP request produces an output like this one:

```Wed May 13 18:54:19 2020 [HTTP] GET /media/animacion.gif 200```

Logs use color to make it easier to see which module or thread the message is coming from.

To increase the level of verbosity provided by logging functions, you can increase the
`DEBUG` macro in the header file located in `source/core/include/constants.h`. Keep in
mind 2 would be a high value, and the logs will get filled with a lot of information for
each single event.

## Project structure
The project is composed by several different modules, each with a clear function. This 
increases maintainability and reusability of the code.

* httputils: Provides functions for dealing and responding to HTTP requests.
* mimetable: Provides functions for storing and retrieving MIME type associations in a performant way
* queue: Provides a simple thread-safe queue implementation
* readconfig: Provides functions to read a configuration file and retrieve configuration
options in a performant way
* server: Provides a general purpose TCP server
* httpserver: Provides the specific implementation of this server using httputils

There is also a `core` directory, containing general files for the project (one being
the main file) and a `test` directory, which contains tests for some of the modules.