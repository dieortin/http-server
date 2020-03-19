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

#include "server.h"
#include "readconfig.h"
#include "colorcodes.h"

// Private functions

/**
 * @brief Prints the passed parameters into the server log
 * @param format Format string to be printed
 * @param ... Parameters to be interpolated into the string
 */
void server_log(const char *format, ...);


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
    int options; ///< Contains the options applied to the socket
    int socket_descriptor; ///< Stores the main socket descriptor where the #Server is listening
    SERVERCMD (*request_processor)(int socket, void (*logger)(const char *fmt,
                                                              ...)); ///< The function to be called each time a new connection is accepted.
    ///< Depending on the value returned by it, the server will continue
    ///< accepting requests or stop doing so.
};

Server *
server_init(char *config_filename, SERVERCMD (*request_processor)(int socket, void (*logger)(const char *fmt, ...))) {
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

    int port, ret;
    if ((ret = config_getparam_int(&srv->config, PARAMS_PORT, &port)) != 0) {
        server_log("ERROR: could not fetch port value (%s)\n", readconfig_perror(ret));
        free(srv);
        return NULL;
    }
    srv->address.sin_port = htons(port); // NOLINT(hicpp-signed-bitwise)

    srv->addrlen = sizeof(srv->address);

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

    int port;
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

    if ((setsockopt(srv->socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &srv->options, sizeof srv->options)) < 0) {
        server_log("Socket creation failed");
        perror("Options failed: SOL_REUSEADDR");
        return ERROR;
    }

    if ((setsockopt(srv->socket_descriptor, SOL_SOCKET, SO_REUSEPORT, &srv->options, sizeof srv->options)) < 0) {
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

    int queue_size;
    config_getparam_int_n(&srv->config, USERPARAMS_META[PARAMS_QUEUE_SIZE].name, &queue_size);
    if (listen(srv->socket_descriptor, queue_size) < 0) {
        server_log("Listen failed");
        perror("Listen failed");
        return ERROR;
    }

    server_log("Server listening on port %i", port);

    while (1) {
        int new_socket;
        if ((new_socket = accept(srv->socket_descriptor, (struct sockaddr *) &srv->address,
                                 (socklen_t *) &srv->addrlen)) == 0) {
            server_log("Accept failed");
            perror(("Accept failed"));
            return ERROR;
        } else {
            if ((*(srv->request_processor))(new_socket, &server_http_log) == STOP) {
                break;
            }
        }
    }
    return SUCCESS;
}

void server_log(const char *format, ...) {
    char *timestr = get_time_str();

    printf("%s%s %s[Server]%s ", YEL, timestr, GRN, reset); // TODO: Allow printing to other files (syslog?)
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void server_http_log(const char *format, ...) {
    char *timestr = get_time_str();

    printf("%s%s %s[HTTP]%s ", YEL, timestr, BLU, reset); // TODO: Allow printing to other files (syslog?)
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
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