#include "httputils.h"
#include "constants.h"
#include "server.h"

int main() {
    Server *server = server_init(CONFIG_PATH, (SERVERCMD (*)(int, const struct _srvutils *)) processHTTPRequest);
    if (server) {
        server_start(server);
        server_free(server);
    }
}



