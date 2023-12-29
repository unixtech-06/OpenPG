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

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/http_request_https.h"

#define HTTPS_PORT 443
#define BUFFER_SIZE 1024

// HTTPS接続が可能かどうかを確認する関数
	int
can_connect_https(const char *hostname)
{
	struct sockaddr_in serv_addr;
	int result = 0; // 接続が不可能と仮定

	SSL_library_init();
	SSL_load_error_strings();

	SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
	if (!ctx) {
		return 0;
	}

	const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		SSL_CTX_free(ctx);
		return 0;
	}

	const struct hostent* server = gethostbyname(hostname);
	if (server == NULL) {
		close(sockfd);
		SSL_CTX_free(ctx);
		return 0;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	serv_addr.sin_port = htons(HTTPS_PORT);

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		close(sockfd);
		SSL_CTX_free(ctx);
		return 0;
	}

	SSL* ssl = SSL_new(ctx);
	SSL_set_fd(ssl, sockfd);
	if (SSL_connect(ssl) == 1) {
		result = 1; // SSL接続に成功
	}

	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sockfd);
	SSL_CTX_free(ctx);

	return result;
}

// HTTPSを使用してHTMLコンテンツを取得する関数
void 
print_ssl_errors(void) 
{
	unsigned long err;
	while ((err = ERR_get_error()) != 0) {
		char *str = ERR_error_string(err, 0);
		if (str) {
			fprintf(stderr, "SSL error: %s\n", str);
		} else {
			fprintf(stderr, "SSL error: %lu (no string representation)\n", err);
		}
	}
}

int 
verify_certificate(void) 
{
	char response[2];
	printf("SSL/TLS handshake failed. Ignore certificate verification? (y/N): ");
	if (scanf("%1s", response) == 1) {
		return (response[0] == 'y' || response[0] == 'Y');
	}
	return 0;
}

void 
parse_url(const char *url, char *hostname, char *path) 
{
	char *url_copy = strdup(url);
	char *scheme_end = strstr(url_copy, "://");

	if (scheme_end) {
		*scheme_end = '\0';  // スキームの終わりをマーク
		scheme_end += 3;     // "://" をスキップ
	} else {
		scheme_end = url_copy;  // スキームがない場合
	}

	char *path_start = strstr(scheme_end, "/");
	if (path_start) {
		*path_start = '\0';  // ホスト名の終わりをマーク
		strcpy(hostname, scheme_end);
		strcpy(path, path_start + 1);
	} else {
		strcpy(hostname, scheme_end);
		strcpy(path, "");  // パスがない場合
	}
	free(url_copy);
}

void 
fetch_html_https(const char *url, int download) 
{
	char hostname[256] = {0};
	char path[256] = {0};
	parse_url(url, hostname, path);

	struct sockaddr_in serv_addr;
	char buffer[BUFFER_SIZE];
	FILE *file = stdout;

	if (download) {
		file = fopen("index.html", "wb");
		if (file == NULL) {
			perror("ERROR opening file");
			return;
		}
	}

	SSL_library_init();
	SSL_load_error_strings();

	SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx) {
		print_ssl_errors();
		return;
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERROR opening socket");
		SSL_CTX_free(ctx);
		return;
	}

	struct hostent *server = gethostbyname(hostname);
	if (server == NULL) {
		perror("ERROR, no such host");
		close(sockfd);
		SSL_CTX_free(ctx);
		return;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	serv_addr.sin_port = htons(HTTPS_PORT);

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		close(sockfd);
		SSL_CTX_free(ctx);
		return;
	}

	SSL *ssl = SSL_new(ctx);
	SSL_set_fd(ssl, sockfd);

	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

	if (SSL_connect(ssl) != 1) {
		print_ssl_errors();
		if (verify_certificate()) {
			SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);
			SSL_free(ssl);
			ssl = SSL_new(ctx);
			SSL_set_fd(ssl, sockfd);
			if (SSL_connect(ssl) != 1) {
				print_ssl_errors();
				SSL_free(ssl);
				close(sockfd);
				SSL_CTX_free(ctx);
				return;
			}
		} else {
			SSL_free(ssl);
			close(sockfd);
			SSL_CTX_free(ctx);
			return;
		}
	}

	sprintf(buffer, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, hostname);
	int n = SSL_write(ssl, buffer, strlen(buffer));
	if (n < 0) {
		print_ssl_errors();
		SSL_free(ssl);
		close(sockfd);
		SSL_CTX_free(ctx);
		return;
	}

	while ((n = SSL_read(ssl, buffer, BUFFER_SIZE - 1)) > 0) {
		buffer[n] = '\0';
		fwrite(buffer, 1, n, file);
	}

	if (n < 0) {
		print_ssl_errors();
	}

	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sockfd);
	SSL_CTX_free(ctx);

	if (download) {
		fclose(file);
	}
}
