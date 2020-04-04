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

// Private functions

/**
 * @brief Prints the passed parameters into the server log
 * @param format Format string to be printed
 * @param ... Parameters to be interpolated into the string
 */
void server_log(const char *format, ...);

void add_connection(Server *srv, int socket);

int get_connection(Server *srv);

void *connectionHandler(void *p);

void server_logv(const char *title, const char *titlecolor, const char *subtitle, const char *subtitlecolor,
                 const char *format, va_list args);

void server_thread_log(int thread_n, const char *format, ...);

void server_http_log(const char *format, ...);

char *get_time_str();

/**
 * @struct _server
 * @brief Stores all the data of the server. Most of it is filled in when the #server_init function
 * is called, and the rest when #server_start is called.
 */
struct _server {
    //struct configuration config; ///< A #configuration structure containing the user defined parameters
    const struct config_param *config; ///< Dictionary which contains the user defined parameters for the server
    struct sockaddr_in address; ///< A #sockaddr_in structure containing the parameters for socket binding
    int addrlen; ///< Contains the length of the #_server.address structure
    int socket_descriptor; ///< Stores the main socket descriptor where the #Server is listening
    queue *queue; ///< Integer queue where the socket identifiers from new connections will be added for processing
    sem_t *sem; ///< Semaphore that blocks the threads unless they have a request to process
    pthread_mutex_t *mutex; ///< Mutex that protects the queue to prevent race conditions
    pthread_t **threads; ///< Array that stores the threads that process the requests
    int nthreads; ///< Number of threads in the array
    SERVERCMD (*request_processor)(int socket, void (*logger)(const char *fmt,
                                                              ...)); ///< The function to be called each time a new connection is accepted.
    ///< Depending on the value returned by it, the server will continue accepting requests or stop doing so. The request
    ///< processor function must accept two parameters: an integer (the socket descriptor) and a logging function that it
    ///< can use to produce logs.
};

struct handler_param {
    Server *srv;
    int thread_id;
};

Server *
server_init(char *config_filename, SERVERCMD (*request_processor)(int, void (*)(const char *, ...))) {
    server_log("Initializing server...");

    if (!config_filename) {
        server_log("ERROR: no configuration filename provided!");
        return NULL;
    }

    if (!request_processor) {
        server_log("ERROR: no request processor function provided!");
        return NULL;
    }

    Server *srv = calloc(1, sizeof(Server));

    srv->request_processor = request_processor;

    srv->config = NULL;

    if (parseConfig(config_filename, &srv->config) != EXIT_SUCCESS) {
        server_log("ERROR: couldn't read the configuration file %s", config_filename);
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
    int port, ret;
    if ((ret = config_getparam_int(&srv->config, PARAMS_PORT, &port)) != 0) {
        server_log("ERROR: could not fetch port value (%s)\n", readconfig_perror(ret));
        free(srv);
        return NULL;
    }

    // Set the port value in the address field
    srv->address.sin_port = htons((uint16_t) port); // NOLINT(hicpp-signed-bitwise)
    srv->addrlen = sizeof(srv->address);

    int max_queue;
    if ((ret = config_getparam_int(&srv->config, PARAMS_QUEUE_SIZE, &max_queue)) != 0) {
        server_log("ERROR: could not fetch max queue size value, using %i as value (%s)\n", DEFAULT_MAX_QUEUE,
                   readconfig_perror(ret));
        srv->queue = queue_create(DEFAULT_MAX_QUEUE);
    } else {
        srv->queue = queue_create(max_queue);
    }
    if (!srv->queue) {
        server_log("ERROR: could not initialize connection queue.");
        return NULL;
    }

    srv->sem = calloc(1, sizeof(sem_t));
    if (sem_init(srv->sem, 0, 0) != 0) { // Initialize semaphore
        perror("Could not initialize semaphore");
    }

    srv->mutex = calloc(1, sizeof(pthread_mutex_t));
    pthread_mutex_init(srv->mutex, NULL);


    // Retrieve the number of threads from the configuration file, or use the default if the operation fails
    int num_threads;
    if ((ret = config_getparam_int(&srv->config, PARAMS_NTHREADS, &num_threads)) != 0) {
        server_log("Error while fetching the number of threads from the configuration file (%s)",
                   readconfig_perror(ret));
        server_log("Using %i as a default value for number of threads", DEFAULT_NTHREADS);
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

STATUS server_start(Server *srv) {
    if (!srv) return ERROR;

    server_log("Starting server...");

    int port, queue_size;
    char *webroot, *ip;
    config_getparam_int(&srv->config, PARAMS_PORT, &port);
    config_getparam_str(&srv->config, PARAMS_ADDRESS, &ip);
    config_getparam_str(&srv->config, PARAMS_WEBROOT, &webroot);
    server_log("Configuration options are:\n\taddress %s\n\tport %u\n\twebroot [%s]", ip, port, webroot);

    if ((srv->socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        server_log("Socket creation failed");
        perror("Socket creation failed");
        return ERROR;
    }

    int flag = 1;

    if ((setsockopt(srv->socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag)) < 0) {
        server_log("Socket creation failed");
        perror("Options failed: SOL_REUSEADDR");
        return ERROR;
    }

    if ((setsockopt(srv->socket_descriptor, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof flag)) < 0) {
        server_log("Socket creation failed");
        perror("Options failed: SOL_REUSEPORT");
        return ERROR;
    }

    server_log("Starting server on port %i...", port);

    if (bind(srv->socket_descriptor, (struct sockaddr *) &srv->address, sizeof(srv->address)) < 0) {
        server_log("Bind failed");
        perror("Bind failed");
        return ERROR;
    }

    config_getparam_int(&srv->config, PARAMS_QUEUE_SIZE, &queue_size);
    if (listen(srv->socket_descriptor, queue_size) < 0) {
        server_log("Listen failed");
        perror("Listen failed");
        return ERROR;
    }

    server_log("Server listening on port %i", port);

    server_log("Starting %i threads...", srv->nthreads);
    for (int i = 0; i < srv->nthreads; i++) {
        struct handler_param *param = malloc(sizeof(struct handler_param));
        param->srv = srv;
        param->thread_id = i;
        pthread_create(srv->threads[i], NULL, connectionHandler, param);
    }

    while (1) {
        int new_socket;
        if ((new_socket = accept(srv->socket_descriptor, (struct sockaddr *) &srv->address,
                                 (socklen_t *) &srv->addrlen)) == 0) {
            server_log("Accept failed");
            perror(("Accept failed"));
            return ERROR;
        } else {
            add_connection(srv, new_socket); // Add the socket of the new connection to the queue
            sem_post(srv->sem); // Increment the semaphore so that one thread is freed up to process the request
        }
    }
    //return SUCCESS;
}

int get_connection(Server *srv) {
    int socket;

    pthread_mutex_lock(srv->mutex);
    socket = queue_pop(srv->queue);
    pthread_mutex_unlock(srv->mutex);

    return socket;
}

void add_connection(Server *srv, int socket) {
    pthread_mutex_lock(srv->mutex);
    queue_add(srv->queue, socket);
    pthread_mutex_unlock(srv->mutex);
}

void *connectionHandler(void *p) {
    struct handler_param *param = (struct handler_param *) p;
    Server *srv = param->srv;
    int thread_id = param->thread_id;

    server_thread_log(thread_id, "Thread started operation");

    while (1) {
        sem_wait(srv->sem);

        int socket = get_connection(srv);
        server_thread_log(thread_id, "Thread processing request on socket [%i]", socket);

        if (srv->request_processor(socket, server_http_log) == STOP) {
            return NULL;
        }
    }
}

void server_thread_log(int thread_n, const char *format, ...) {
    char threadnum[24];
    sprintf(threadnum, "%i", thread_n);

    va_list args;
    va_start(args, format);
    server_logv("Server", GRN, threadnum, MAG, format, args);
    va_end(args);
}

void server_log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    server_logv("Server", GRN, NULL, 0, format, args);
    va_end(args);
}

void server_logv(const char *title, const char *titlecolor, const char *subtitle, const char *subtitlecolor,
                 const char *format, va_list args) {
    char *timestr = get_time_str();
    if (subtitle && subtitlecolor) {
        printf("%s%s %s[%s]%s::[%s]%s ", YEL, timestr, titlecolor, title, subtitlecolor, subtitle,
               reset); // TODO: Allow printing to other files (syslog?)
    } else {
        printf("%s%s %s[%s]%s ", YEL, timestr, titlecolor, title,
               reset); // TODO: Allow printing to other files (syslog?)
    }
    vprintf(format, args);
    printf("\n");
}

void server_http_log(const char *format, ...) {
    va_list args;
    va_start(args, format);
    server_logv("HTTP", BLU, NULL, NULL, format, args);
    va_end(args);
}

char *get_time_str() {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    char *timestr = asctime(timeinfo);
    timestr[strlen(timestr) - 1] = 0; // Remove the newline at the end

    return timestr;
}