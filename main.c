#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "httputils.h"

#define PORT 8081

int main() {
	printf("INFO: Server started\n");

	int socket_descriptor, new_socket, valread;
	int opt = 1;

	struct sockaddr_in address;
	int addrlen = sizeof(address);

	// Create socket
	if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	if ((setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof opt)) == 0) {
		perror("Options failed");
		exit(EXIT_FAILURE);
	}

	memset(&address, 0, sizeof address);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(PORT);


	if (bind(socket_descriptor, (struct sockaddr *) &address, sizeof(address)) < 0) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(socket_descriptor, 3) < 0) {
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	printf("INFO: Server listening on port %i\n", PORT);

	while(1) {
		if ((new_socket = accept(socket_descriptor, (struct sockaddr *) &address, (socklen_t * ) & addrlen)) == 0) {
			perror(("Accept failed"));
			exit(EXIT_FAILURE);
		} else {
			processHTTPRequest(new_socket);
		}
	}



}

