#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "server.h"

struct server * server = 0;

int
main (int argc, char** argv)
{
    server = server_alloc ();
    if (!server
        || server_configure (server, argc, argv)
        || server_run (server)
        || server_shutdown (server))
    {
        fprintf (stderr, "%s: %s\n", argv[0], server_error (server));
        server_destroy (server);
        return EXIT_FAILURE;
    }
    server_destroy (server);
    return EXIT_SUCCESS;
}
