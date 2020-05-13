#include <stdio.h>

#include "httpserver.h"
#include "constants.h"
#include "server.h"

int main(int argc, char *argv[]) {
    char *project_path;
    if (argc == 2) {
        project_path = argv[1]; // Take the provided path
    } else {
        fprintf(stdout, "Using predefined path for configuration directory: %s\n", CONFIG_PATH);
        project_path = CONFIG_PATH; // Use the predefined path
    }


    Server *server = server_init(project_path, (SERVERCMD (*)(int, const struct _srvutils *)) processHTTPRequest);
    if (server) {
        server_start(server);
        server_free(server);
    }
}



