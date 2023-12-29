/* BSD 3-Clause License
 * Copyright (c) 2023, Ryosuke
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h> // add: tolower function

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

		// 応答を小文字に変換
		for (int i = 0; i < n; i++) {
			response[i] = tolower(response[i]);
		}

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
