/**
 * @file server.c
 * Contains the implementation of the functions for #Server operation, as well as some
 * private utility functions
 */

#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "server.h"
#include "readconfig.h"
#include "colorcodes.h"

// Private functions

/**
 * @brief Prints the passed parameters into the server log
 * @param format Format string to be printed
 * @param ... Parameters to be interpolated into the string
 */
void server_log (const char *format, ...);

/**
 * This structure stores all the data of the server. Most of it is filled in when the #server_init function
 * is called, and the rest when #server_start is called.
 */
struct _server {
	struct configuration config; ///< A #configuration structure containing the user defined parameters
	struct sockaddr_in address; ///< A #sockaddr_in structure containing the parameters for socket binding
	int addrlen; ///< Contains the length of the #_server.address structure
	int options; ///< Contains the options applied to the socket
	int socket_descriptor; ///< Stores the main socket descriptor where the #Server is listening
	void (*request_processor)(int socket); ///< The function to be called each time a new connection is accepted
};

Server *server_init (char *config_filename, void (*request_processor)(int socket)) {
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

	if (readConfig(config_filename, &srv->config) != EXIT_SUCCESS) {
		server_log("ERROR: couldn't read the configuration file %s", config_filename);
		free(srv);
		return NULL;
	}

	// Initialize the address field for binding
	memset(&srv->address, 0, sizeof srv->address);
	srv->address.sin_family = AF_INET;
	srv->address.sin_addr.s_addr = htonl(INADDR_ANY);
	srv->address.sin_port = htons(srv->config.port);

	srv->addrlen = sizeof(srv->address);

	return srv;
}

STATUS server_free (Server *srv) {
	if (!srv) return ERROR;

	free(srv);
	return SUCCESS;
}

STATUS server_start (Server *srv) {
	if (!srv) return ERROR;

	server_log("Starting server...");
	server_log("Configuration options are:\n\tport %u\n\twebroot [%s]", srv->config.port, srv->config.webroot);

	if ((srv->socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		server_log("Socket creation failed");
		perror("Socket creation failed");
		return ERROR;
	}

	if ((setsockopt(srv->socket_descriptor, SOL_SOCKET, SO_REUSEADDR, &srv->options, sizeof srv->options)) == -1) {
		server_log("Socket creation failed");
		perror("Options failed: SOL_REUSEADDR");
		return ERROR;
	}

	if ((setsockopt(srv->socket_descriptor, SOL_SOCKET, SO_REUSEPORT, &srv->options, sizeof srv->options)) == -1) {
		server_log("Socket creation failed");
		perror("Options failed: SOL_REUSEPORT");
		return ERROR;
	}

	server_log("Starting server on port %i...", srv->config.port);

	if (bind(srv->socket_descriptor, (struct sockaddr *) &srv->address, sizeof(srv->address)) < 0) {
		server_log("Bind failed");
		perror("Bind failed");
		return ERROR;
	}

	if (listen(srv->socket_descriptor, srv->config.queue_size) < 0) {
		server_log("Listen failed");
		perror("Listen failed");
		return ERROR;
	}

	server_log("Server listening on port %i", srv->config.port);

	while(1) {
		int new_socket;
		if ((new_socket = accept(srv->socket_descriptor, (struct sockaddr *) &srv->address, (socklen_t * ) &srv->addrlen)) == 0) {
			server_log("Accept failed");
			perror(("Accept failed"));
			return ERROR;
		} else {
			(*(srv->request_processor))(new_socket);
		}
	}
	return SUCCESS;
}

void server_log (const char *format, ...) {
	time_t rawtime;
	struct tm * timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	char *timestr = asctime(timeinfo);
	timestr[strlen(timestr) - 1] = 0; // Remove the newline at the end

	printf("%s%s %s[Server]%s ", YEL, timestr, GRN, reset); // TODO: Allow printing to other files (syslog?)
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}
