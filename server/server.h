/**
 * @file server.h
 * @brief Module for running a generic TCP #Server
 *
 * This module provides the functions and data structures required to run a generic #Server, which reads
 * options from a configuration file and listens for requests on the specified port.
 * When initializing the server, a function for processing the requests must be provided. This allows the
 * module to be used for different purposes by simply changing the request processor function.
 * @author Diego Ortín Fernández
 * @date 7 March 2020
 */
#ifndef PRACTICA1_SERVER_H
#define PRACTICA1_SERVER_H

#include "constants.h"

/**
 * This enumeration contains the messages a request processor function can return to the #Server. They allow
 * the request processor to interact with the server.
 */
typedef enum _SERVERCMD {
    CONTINUE, ///< This message signals the #Server to continue accepting requests
    STOP ///< This message signals the #Server to stop accepting requests
} SERVERCMD;

/**
 * The Server type, storing all the data required for the operation of the server.
 */
typedef struct _server Server;

/**
 * @brief Reads the provided configuration file, then initializes the structures required for the #Server
 * and creates the main socket where it will listen according to that configuration.
 * @details This function is the one in charge of providing the #Server structure with most of the data it needs
 * to operate. The configuration file provides the basic parameters for server operation, such as port number,
 * number of threads or webroot folder. Passing a request processor function allows for reusing of the #Server
 * with different processor functions.
 * @pre A configuration file with at least the basic parameters must exist in the path defined by @p config_filename.
 * @param[in] config_filename The name of the file to use for reading the server configuration. It must be in
 * the same directory as the server source.
 * @param[in] request_processor The function to be used to process each accepted request.
 * @return An initialized #Server, ready to be started with server_start(), or \a NULL if any error occurs.
 */
Server *server_init(char *config_filename, SERVERCMD (*request_processor)(int socket));

/**
 * @brief Frees all the associated memory of the provided #Server
 * @pre @p srv must point to an initialized #Server
 * @param[in,out] srv The #Server to be freed
 * @return \ref STATUS.ERROR if any error occurs, \ref STATUS.SUCCESS otherwise
 */
STATUS server_free(Server *srv);

/**
 * @brief Makes the #Server start listening and accepting connections with the function stored
 * in #_server.request_processor.
 * @pre @p srv must point to an initialized #Server
 * @param srv The #Server to start.
 * @return \ref STATUS.ERROR if any error occurs, \ref STATUS.SUCCESS otherwise
 */
STATUS server_start(Server *srv);

#endif //PRACTICA1_SERVER_H