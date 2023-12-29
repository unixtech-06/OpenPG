#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/http_request.h"

#define PORT 80
#define BUFFER_SIZE 1024

void
fetch_html(const char *hostname, int download)
{
        struct sockaddr_in serv_addr;
        char buffer[BUFFER_SIZE];
        int header_found = 0;
        FILE *file = stdout;

        if (download) {
                file = fopen("index.html", "wb");
                if (file == NULL)
                        err(1, "ERROR opening file");
        }

        const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
                err(1, "ERROR opening socket");

        const struct hostent* server = gethostbyname(hostname);
        if (server == NULL)
                errx(1, "ERROR, no such host");

        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        serv_addr.sin_port = htons(PORT);

        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                err(1, "ERROR connecting");

        sprintf(buffer, "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", hostname);
        int n = write(sockfd, buffer, strlen(buffer));
        if (n < 0)
                err(1, "ERROR writing to socket");

        char response[BUFFER_SIZE];
        while ((n = read(sockfd, response, BUFFER_SIZE - 1)) > 0) {
                response[n] = '\0';
                if (!header_found) {
                        const char *header_end = strstr(response, "\r\n\r\n");
                        if (header_end) {
                                header_found = 1;
                                fwrite(header_end + 4, 1, strlen(header_end) - 4, file);
                        }
                } else {
                        fwrite(response, 1, n, file);
                }
                if (strstr(response, "</html>") != NULL)
                        break;
        }

        if (n < 0)
                err(1, "ERROR reading from socket");

        if (download) {
                fclose(file);
        }

        close(sockfd);
}