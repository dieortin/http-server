#include "httputils.h"
#include "constants.h"
#include "server.h"

int main() {
	Server *server = server_init(CONFIG_PATH, processHTTPRequest);

	server_start(server);

	server_free(server);
}



