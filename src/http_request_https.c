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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/http_request_https.h"

#define HTTPS_PORT	443
#define BUFFER_SIZE	1024

/*
 * SSLエラーを表示する関数
 */
static void
print_ssl_errors(void)
{
	unsigned long	err;
	char		*str;

	while ((err = ERR_get_error()) != 0) {
		if ((str = ERR_error_string(err, 0)) != NULL) {
			fprintf(stderr, "SSL error: %s\n", str);
		} else {
			fprintf(stderr, "SSL error: %lu (no string representation)\n", err);
		}
	}
}

/*
 * ユーザーに確認を求める関数
 */
int
user_confirm(const char *message)
{
	char	response[2];

	printf("%s (y/N): ", message);
	if (scanf("%1s", response) == 1)
		return (response[0] == 'y' || response[0] == 'Y');
	return 0;
}

/*
 * URLからホスト名とパスを解析する関数
 */
static void
parse_url(const char *url, char *hostname, char *path)
{
	char	*url_copy, *scheme_end, *path_start;

	url_copy = strdup(url);
	scheme_end = strstr(url_copy, "://");
	path_start = NULL;

	if (scheme_end) {
		*scheme_end = '\0';	/* スキームの終わりをマーク */
		scheme_end += 3;	/* "://" をスキップ */
	} else {
		scheme_end = url_copy;	/* スキームがない場合 */
	}

	path_start = strstr(scheme_end, "/");
	if (path_start) {
		*path_start = '\0';	/* ホスト名の終わりをマーク */
		strlcpy(hostname, scheme_end, sizeof(hostname));
		strlcpy(path, path_start + 1, sizeof(path));
	} else {
		strlcpy(hostname, scheme_end, sizeof(hostname));
		strlcpy(path, "", sizeof(path));	/* パスがない場合 */
	}
	free(url_copy);
}

/*
 * HTTPS接続を確立する関数
 */
static SSL *
establish_https_connection(const char *hostname, int *sockfd, SSL_CTX **ctx)
{
	struct sockaddr_in	serv_addr;
	struct hostent		*server;
	SSL			*ssl;

	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd < 0) {
		perror("ERROR opening socket");
		return NULL;
	}

	if ((server = gethostbyname(hostname)) == NULL) {
		perror("ERROR, no such host");
		close(*sockfd);
		return NULL;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	serv_addr.sin_port = htons(HTTPS_PORT);

	if (connect(*sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		close(*sockfd);
		return NULL;
	}

	*ctx = SSL_CTX_new(TLS_client_method());
	if (!*ctx) {
		print_ssl_errors();
		close(*sockfd);
		return NULL;
	}

	ssl = SSL_new(*ctx);
	SSL_set_fd(ssl, *sockfd);

	if (SSL_connect(ssl) != 1) {
		print_ssl_errors();
		SSL_free(ssl);
		close(*sockfd);
		SSL_CTX_free(*ctx);
		return NULL;
	}

	return ssl;
}

/*
 * HTTPS接続が可能かどうかを確認する関数
 */
int
can_connect_https(const char *hostname)
{
	int	sockfd;
	SSL_CTX	*ctx;
	SSL	*ssl;

	ssl = establish_https_connection(hostname, &sockfd, &ctx);
	if (!ssl) {
		if (sockfd >= 0)
			close(sockfd);
		if (ctx)
			SSL_CTX_free(ctx);
		return 0;
	}

	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sockfd);
	SSL_CTX_free(ctx);
	return 1;
}

static void
extract_filename_from_url(const char *url, char *filename)
{
	const char	*last_slash;

	last_slash = strrchr(url, '/');
	if (last_slash && *(last_slash + 1)) {
		strlcpy(filename, last_slash + 1, sizeof(filename));
	} else {
		strlcpy(filename, "downloaded_file", sizeof(filename));
	}
}

/*
 * HTTPSを介してHTMLを取得する関数
 */
void
fetch_html_https(const char *url, int download)
{
	char	hostname[256] = {0};
	char	path[256] = {0};
	int	sockfd, n;
	SSL_CTX	*ctx;
	SSL	*ssl;
	char	buffer[BUFFER_SIZE];
	FILE	*file;

	parse_url(url, hostname, path);

	ssl = establish_https_connection(hostname, &sockfd, &ctx);
	if (!ssl) {
		return;
	}

	file = stdout;
	if (download) {
		file = fopen("index.html", "wb");
		if (file == NULL) {
			perror("ERROR opening file");
			SSL_free(ssl);
			close(sockfd);
			SSL_CTX_free(ctx);
			return;
		}
	}

	snprintf(buffer, sizeof(buffer),
	    "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
	    path, hostname);
	n = SSL_write(ssl, buffer, strlen(buffer));
	if (n < 0) {
		print_ssl_errors();
		SSL_free(ssl);
		close(sockfd);
		SSL_CTX_free(ctx);
		if (download) {
			fclose(file);
		}
		return;
	}

	int header_ended = 0;
	while ((n = SSL_read(ssl, buffer, BUFFER_SIZE - 1)) > 0) {
		buffer[n] = '\0';
		if (!header_ended) {
			const char *header_end = strstr(buffer, "\r\n\r\n");
			if (header_end) {
				header_ended = 1;
				fwrite(header_end + 4, 1, n - (header_end - buffer) - 4, file);
			}
		} else {
			fwrite(buffer, 1, n, file);
		}
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

/*
 * HTTPSを介してCSSを取得する関数
 */
void
fetch_css_https(const char *url, int download)
{
	char	hostname[256] = {0};
	char	path[256] = {0};
	int	sockfd, n;
	SSL_CTX	*ctx;
	SSL	*ssl;
	char	buffer[BUFFER_SIZE];
	FILE	*file;

	parse_url(url, hostname, path);

	ssl = establish_https_connection(hostname, &sockfd, &ctx);
	if (!ssl) {
		return;
	}

	file = stdout;
	if (download) {
		char css_filename[256] = {0};
		extract_filename_from_url(path, css_filename);
		file = fopen(css_filename, "wb");
		if (file == NULL) {
			perror("ERROR opening file");
			SSL_free(ssl);
			close(sockfd);
			SSL_CTX_free(ctx);
			return;
		}
	}

	snprintf(buffer, sizeof(buffer),
	    "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
	    path, hostname);
	n = SSL_write(ssl, buffer, strlen(buffer));
	if (n < 0) {
		print_ssl_errors();
		SSL_free(ssl);
		close(sockfd);
		SSL_CTX_free(ctx);
		if (download) {
			fclose(file);
		}
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

/*
 * HTTPSを介してファイルをダウンロードする関数
 */
void
download_file_https(const char *url)
{
	char	hostname[256] = {0};
	char	path[256] = {0};
	char	filename[256] = {0};
	int	sockfd, n;
	SSL_CTX	*ctx;
	SSL	*ssl;
	char	buffer[BUFFER_SIZE];
	FILE	*file;

	parse_url(url, hostname, path);
	extract_filename_from_url(path, filename);

	ssl = establish_https_connection(hostname, &sockfd, &ctx);
	if (!ssl) {
		return;
	}

	file = fopen(filename, "wb");
	if (!file) {
		perror("ERROR opening file");
		SSL_free(ssl);
		close(sockfd);
		SSL_CTX_free(ctx);
		return;
	}

	snprintf(buffer, sizeof(buffer),
	    "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
	    path, hostname);
	n = SSL_write(ssl, buffer, strlen(buffer));
	if (n < 0) {
		print_ssl_errors();
		SSL_free(ssl);
		close(sockfd);
		SSL_CTX_free(ctx);
		fclose(file);
		return;
	}

	while ((n = SSL_read(ssl, buffer, BUFFER_SIZE - 1)) > 0) {
		fwrite(buffer, 1, n, file);
	}

	if (n < 0) {
		print_ssl_errors();
	}

	SSL_shutdown(ssl);
	SSL_free(ssl);
	close(sockfd);
	SSL_CTX_free(ctx);
	fclose(file);
}
