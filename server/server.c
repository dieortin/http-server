/**
 * @file server.c
 * @brief Contains the implementation of the functions for #Server operation, as well as some
 * private utility functions
 * @author Diego Ortín Fernández
 * @date 7 March 2020
 */

#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <libnet.h>
#include <pthread.h>
#include <semaphore.h>

#include "server.h"
#include "readconfig.h"
#include "colorcodes.h"
#include "queue.h"
#include "mimetable.h"

// Private functions

/**
 * @brief Prints the passed parameters into the server log
 * @param[out] file where the output must be printed
 * @param[in] format Format string to be printed
 * @param[in] ... Parameters to be interpolated into the string
 */
void server_log(FILE *file, const char *format, ...);

void add_connection(Server *srv, int socket);

int get_connection(Server *srv);

void *connectionHandler(void *p);

void
server_logv(FILE *file, const char *titlecolor, const char *subtitle, const char *subtitlecolor, const char *format,
            va_list args, const char *title);

void server_thread_log(FILE *file, int thread_n, const char *format, ...);

void server_http_log(FILE *file, const char *format, ...);

char *get_time_str();

char *get_full_webroot(const char *webroot);

/**
 * @struct _server
 * @brief Stores all the data of the server. Most of it is filled in when the #server_init function
 * is called, and the rest when #server_start is called.
 */
struct _server {
    const struct config_param *config; ///< Dictionary which contains the user defined parameters for the server
    struct sockaddr_in address; ///< A #sockaddr_in structure containing the parameters for socket binding
    int addrlen; ///< Contains the length of the #_server.address structure
    int socket_descriptor; ///< Stores the main socket descriptor where the #Server is listening
    queue *queue; ///< Integer queue where the socket identifiers from new connections will be added for processing
    sem_t *sem; ///< Semaphore that blocks the threads unless they have a request to process
    pthread_t **threads; ///< Array that stores the threads that process the requests
    int nthreads; ///< Number of threads in the array
    SERVERCMD (*request_processor)(int socket,
                                   const struct _srvutils *utils); ///< The function to be called each time a new connection is accepted.
    ///< Depending on the value returned by it, the server will continue accepting requests or stop doing so. The request
    ///< processor function must accept two parameters: an integer (the socket descriptor) and a logging function that it
    ///< can use to produce logs.
};

/**
 * @struct handler_param
 * @brief Used to pass parameters to the request processing threads
 */
struct handler_param {
    Server *srv; ///< Reference to the server to which the processing thread belongs
    int thread_id; ///< Integer that uniquely identifies the thread among all the threads from the same server
    struct _srvutils *utils; ///< Reference to a structure containing utilities the request processor can use
};

Server *
server_init(char *config_filename, SERVERCMD (*request_processor)(int, const struct _srvutils *)) {
    server_log(stdout, "Initializing server...");

    if (!config_filename) {
        server_log(stderr, "ERROR: no configuration filename provided!");
        return NULL;
    }

    if (!request_processor) {
        server_log(stderr, "ERROR: no request processor function provided!");
        return NULL;
    }

    Server *srv = calloc(1, sizeof(Server));

    srv->request_processor = request_processor;

    srv->config = NULL;

    if (parseConfig(config_filename, &srv->config) != EXIT_SUCCESS) {
        server_log(stderr, "ERROR: couldn't read the configuration file %s", config_filename);
        free(srv);
        return NULL;
    }

    int ret;
    char *mimefile;
    if ((ret = config_getparam_str(&srv->config, PARAMS_MIME_FILE, &mimefile)) != 0) {
        server_log(stderr, "ERROR: could not fetch MIME file name (%s)\n", readconfig_perror(ret));
        free(srv);
        return NULL;
    }

    server_log(stdout, "Parsing the MIME file (%s)...", mimefile);
    if (mime_add_from_file(mimefile) == ERROR) {
        server_log(stderr, "ERROR: could not add MIME types from file (%s)", mimefile);
        free(srv);
        return NULL;
    }

    char *ipaddr;
    config_getparam_str(&srv->config, PARAMS_ADDRESS, &ipaddr);

    // Initialize the address field for binding
    memset(&srv->address, 0, sizeof srv->address);
    srv->address.sin_family = AF_INET;
    inet_pton(AF_INET, ipaddr, &srv->address.sin_addr.s_addr);

    // Retrieve the port number from the configuration file
    int port;
    if ((ret = config_getparam_int(&srv->config, PARAMS_PORT, &port)) != 0) {
        server_log(stderr, "ERROR: could not fetch port value (%s)\n", readconfig_perror(ret));
        free(srv);
        return NULL;
    }

    // Set the port value in the address field
    srv->address.sin_port = htons((uint16_t) port); // NOLINT(hicpp-signed-bitwise)
    srv->addrlen = sizeof(srv->address);

    int max_queue;
    if ((ret = config_getparam_int(&srv->config, PARAMS_QUEUE_SIZE, &max_queue)) != 0) {
        server_log(stderr, "ERROR: could not fetch max queue size value, using %i as value (%s)\n", DEFAULT_MAX_QUEUE,
                   readconfig_perror(ret));
        srv->queue = queue_create(DEFAULT_MAX_QUEUE);
    } else {
        srv->queue = queue_create(max_queue);
    }
    if (!srv->queue) {
        server_log(stderr, "ERROR: could not initialize connection queue.");
        return NULL;
    }

    srv->sem = calloc(1, sizeof(sem_t));
    if (sem_init(srv->sem, 0, 0) != 0) { // Initialize semaphore
        perror("Could not initialize semaphore");
    }

    // Retrieve the number of threads from the configuration file, or use the default if the operation fails
    int num_threads;
    if ((ret = config_getparam_int(&srv->config, PARAMS_NTHREADS, &num_threads)) != 0) {
        server_log(stderr, "Error while fetching the number of threads from the configuration file (%s)",
                   readconfig_perror(ret));
        server_log(stderr, "Using %i as a default value for number of threads", DEFAULT_NTHREADS);
        num_threads = DEFAULT_NTHREADS;
    }

    srv->nthreads = num_threads; // Store the number of threads in the structure
    srv->threads = calloc((size_t) num_threads, sizeof(pthread_t *)); // Allocate space for the thread pointers
    for (int i = 0; i < num_threads; i++) {
        srv->threads[i] = calloc(1, sizeof(pthread_t));
    }

    return srv;
}

STATUS server_free(Server *srv) {
    if (!srv) return ERROR;

    free(srv);
    return SUCCESS;
}

/**
 * @brief Sets the socket options for the server socket
 * @pre The socket must have been created previously
 * @param[in] socket The socket to set options for
 * @return \ref STATUS.ERROR if any error occurs, \ref STATUS.SUCCESS otherwise
 */
STATUS server_setsockopts(int socket) {
    int flag = 1;

    if ((setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag)) < 0) {
        server_log(stderr, "Socket creation failed");
        perror("Options failed: SOL_REUSEADDR");
        return ERROR;
    }

    if ((setsockopt(socket, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof flag)) < 0) {
        server_log(stderr, "Socket creation failed");
        perror("Options failed: SOL_REUSEPORT");
        return ERROR;
    }

    return SUCCESS;
}

STATUS server_start(Server *srv) {
    if (!srv) return ERROR;

    server_log(stdout, "Starting server...");

    int port, queue_size;
    char *webroot, *ip;
    config_getparam_int(&srv->config, PARAMS_PORT, &port);
    config_getparam_str(&srv->config, PARAMS_ADDRESS, &ip);
    config_getparam_str(&srv->config, PARAMS_WEBROOT, &webroot);
    server_log(stdout, "Configuration options are:\n\taddress %s\n\tport %u\n\twebroot [%s]", ip, port, webroot);

    if ((srv->socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        server_log(stderr, "Socket creation failed");
        perror("Socket creation failed");
        return ERROR;
    }

    server_setsockopts(srv->socket_descriptor);

    server_log(stdout, "Starting server on port %i...", port);

    if (bind(srv->socket_descriptor, (struct sockaddr *) &srv->address, sizeof(srv->address)) < 0) {
        server_log(stderr, "Bind failed");
        perror("Bind failed");
        return ERROR;
    }

    config_getparam_int(&srv->config, PARAMS_QUEUE_SIZE, &queue_size);
    if (listen(srv->socket_descriptor, queue_size) < 0) {
        server_log(stderr, "Listen failed");
        perror("Listen failed");
        return ERROR;
    }

    server_log(stdout, "Server listening on port %i", port);

    char *full_webroot = get_full_webroot(webroot);

#if DEBUG >= 1
    server_log(stdout, "The full path for the webroot is '%s'", full_webroot);
#endif

    // Create a structure holding the server utilities for the request processor
    struct _srvutils utils;
    utils.log = server_http_log;
    utils.webroot = full_webroot;

    server_log(stdout, "Starting %i threads...", srv->nthreads);
    for (int i = 0; i < srv->nthreads; i++) {
        struct handler_param *param = malloc(sizeof(struct handler_param));
        param->srv = srv;
        param->thread_id = i;
        param->utils = &utils;
        pthread_create(srv->threads[i], NULL, connectionHandler, param);
    }

    server_log(stdout, "Server running on http://%s:%i", ip, port);

    while (1) {
        int new_socket;
        if ((new_socket = accept(srv->socket_descriptor, (struct sockaddr *) &srv->address,
                                 (socklen_t *) &srv->addrlen)) == 0) {
            server_log(stderr, "Accept failed");
            perror(("Accept failed"));
            return ERROR;
        } else {
            add_connection(srv, new_socket); // Add the socket of the new connection to the queue
            sem_post(srv->sem); // Increment the semaphore so that one thread is freed up to process the request
        }
    }
    //return SUCCESS;
}

char *get_full_webroot(const char *webroot) {
    if (!webroot) return NULL;

    // Get the current working directory
    // When buf == NULL, getcwd allocates the buffer
    // When size == 0, getcwd allocates as much space as needed
    char *cwd = getcwd(NULL, 0);

    // Calculate the size needed for the entire webroot plus the null terminator
    size_t webroot_full_size = strlen(cwd) + strlen(webroot) + 1;

    // Allocate enough memory for the full webroot
    char *full_webroot = malloc(sizeof(char) * webroot_full_size);

    // Concatenate the current working directory and the webroot to form the full webroot
    strcat(full_webroot, cwd);
    strcat(full_webroot, webroot);
    free(cwd);

    return full_webroot;
}

/**
 * @brief Obtains a new connection from the queue
 * @details This function is a wrapper around the #queue_pop function from the queue.h module. This operation is
 * blocking if there are no available connections in the queue.
 * @param[in] srv The server from whose queue the connections must be obtained
 * @return The integer that identifies the socket in which the connection has been established
 */
int get_connection(Server *srv) {
    return queue_pop(srv->queue);
}

/**
 * @brief Adds a new connection to the queue
 * @details This function is a wrapper around the #queue_add function. This operation is blocking when there are no
 * empty slots in the queue.
 * @param[in] srv The server to whose queue the connection must be added
 * @param[in] socket The integer that identifies the socket in which the connection has been established
 */
void add_connection(Server *srv, int socket) {
    queue_add(srv->queue, socket);
}

void *connectionHandler(void *p) {
    struct handler_param *param = (struct handler_param *) p;
    Server *srv = param->srv;
    int thread_id = param->thread_id;

    server_thread_log(stdout, thread_id, "Thread started operation");

    while (1) {
        int socket = get_connection(srv);
        if (socket == -1) {
            server_thread_log(stdout, thread_id, "Could not obtain a connection");
            continue; // If for some reason there was an error getting a connection, skip the iteration
        }
#if DEBUG >= 2
        server_thread_log(stdout, thread_id, "Thread processing request on socket [%i]", socket);
#endif

        if (srv->request_processor(socket, param->utils) == STOP) {
            return NULL;
        }
    }
}

void server_thread_log(FILE *file, int thread_n, const char *format, ...) {
    char threadnum[24];
    sprintf(threadnum, "%i", thread_n);

    va_list args;
    va_start(args, format);
    server_logv(file, GRN, threadnum, MAG, format, args, "Server");
    va_end(args);
}

void server_log(FILE *file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    server_logv(file, GRN, NULL, 0, format, args, "Server");
    va_end(args);
}

void
server_logv(FILE *file, const char *titlecolor, const char *subtitle, const char *subtitlecolor, const char *format,
            va_list args, const char *title) {
    char *timestr = get_time_str();
    flockfile(file); // Lock the file so that the entire output is printed atomically
    if (subtitle && subtitlecolor) {
        fprintf(file, "%s%s %s[%s]%s::[%s]%s ", YEL, timestr, titlecolor, title, subtitlecolor, subtitle,
                reset); // TODO: Allow printing to other files (syslog?)
    } else {
        fprintf(file, "%s%s %s[%s]%s ", YEL, timestr, titlecolor, title,
                reset); // TODO: Allow printing to other files (syslog?)
    }
    vfprintf(file, format, args);
    printf("\n");
    funlockfile(file); // Unlock the file
    free(timestr);
}

void server_http_log(FILE *file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    server_logv(file, BLU, NULL, NULL, format, args, "HTTP");
    va_end(args);
}

char *get_time_str() {
    time_t rawtime;
    struct tm timeinfo;

    time(&rawtime);
    localtime_r(&rawtime, &timeinfo);

    char *timestr = malloc(sizeof(char) * 100); // Allocate size for the time string
    asctime_r(&timeinfo, timestr);

    timestr[strlen(timestr) - 1] = 0; // Remove the newline at the end

    return timestr;
}