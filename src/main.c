#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/http_request.h"

int
main(int argc, char *argv[])
{
        if (argc != 2 && (argc != 3 || strcmp(argv[1], "-d") != 0)) {
                fprintf(stderr, "Usage: %s [-d] hostname\n", argv[0]);
                exit(1);
        }

        const int download = (argc == 3 && strcmp(argv[1], "-d") == 0);
        fetch_html(argv[argc - 1], download);

        exit(0);
}