/**
 * @file httpserver.h
 * @brief Functions that implement this specific HTTP server
 * @details HTTP server using the methods and data structures provided by the httputils.h module. Its public interface
 * consists of a single function, conforming to the prototype required by the #Server module, so that it can be used
 * with it.
 * @see httputils.h
 * @see server.h
 * @author Diego Ortín and Mario López
 * @date February 2020
 */

#ifndef PRACTICA1_HTTPSERVER_H
#define PRACTICA1_HTTPSERVER_H

#include "server.h"
#include "httputils.h"

/**
 * @brief Processes the request in the provided socket
 * @param[in,out] socket The socket where the connection of the request has been established.
 * @param[in] utils Structure containing utilities that the HTTP server can use during its operation.
 * @return SERVERCMD to control the behavior of the underlying TCP server.
 */
SERVERCMD processHTTPRequest(int socket, struct _srvutils *utils);

#endif //PRACTICA1_HTTPSERVER_H
