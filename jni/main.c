#include <stdlib.h>
#include "evproxy.h"
#include "protocol.h"

static void usage(int argc, char **argv);

int main(int argc, char **argv)
{
	int port = DEFAULT_PORT;
	if (3 == argc) {
		port = atoi(argv[2]);
	}
	struct evp_cntxt *cntxt = create_evproxy(argv[1], port);
	if (NULL == cntxt) {
		printf("Failed create evproxy\n");
		free_evproxy(cntxt);
		return -1;
	}

	if (start_evproxy(cntxt) < 0) {
		printf("Failed start evproxy\n");
		free_evproxy(cntxt);
		return -2;
	}

	loop_evproxy(cntxt);
	free_evproxy(cntxt);
}

static void usage(int argc, char **argv)
{
	if (2 != argc && 3 != argc) {
		printf("Usage:\n");
		printf("  %s ip [port]\n", argv[0]);
		exit (0);
	}
}
