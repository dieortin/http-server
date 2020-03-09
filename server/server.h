/**
 * @file server.h
 */
#ifndef PRACTICA1_SERVER_H
#define PRACTICA1_SERVER_H

#include "constants.h"

/**
 * The Server type, storing all the data required for the operation of the server.
 */
typedef struct _server Server;

/**
 * @brief Reads the provided configuration file, then initializes the structures required for the #Server
 * and creates the main socket where it will listen according to that configuration.
 * @param[in] config_filename The name of the file to use for reading the server configuration. It must be in
 * the same directory as the server source.
 * @param[in] request_processor The function to be used to process each accepted request.
 * @return An initialized #Server, ready to be started with server_start(), or \a NULL if any error occurs.
 */
Server *server_init (char *config_filename, void (*request_processor)(int socket));

/**
 * @brief Frees all the associated memory of the provided #Server
 * @param[in,out] srv The #Server to be freed
 * @return \ref STATUS.ERROR if any error occurs, \ref STATUS.SUCCESS otherwise
 */
STATUS server_free (Server *srv);

/**
 * @brief Makes the #Server start listening and accepting connections with the function stored
 * in #_server.request_processor.
 * @param srv The #Server to start.
 * @return \ref STATUS.ERROR if any error occurs, \ref STATUS.SUCCESS otherwise
 */
STATUS server_start (Server *srv);

#endif //PRACTICA1_SERVER_H
