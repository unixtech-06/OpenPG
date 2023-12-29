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
void fetch_html_https(const char *hostname, int download) {
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

    // OpenSSLライブラリの初期化
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    // SSLコンテキストの作成
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method()); // TLS_client_methodを使用
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return;
    }

    // SSL/TLSの設定
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1); // 古いプロトコルを無効化

    // SSL証明書の検証を無視する設定（セキュリティリスクに注意）
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);

    // ソケットの作成と接続
    const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        SSL_CTX_free(ctx);
        return;
    }

    const struct hostent *server = gethostbyname(hostname);
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

    // SSLオブジェクトの作成とソケットに結び付け
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) != 1) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sockfd);
        SSL_CTX_free(ctx);
        return;
    }

    // HTTPSリクエストの送信
    sprintf(buffer, "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", hostname);
    int n = SSL_write(ssl, buffer, strlen(buffer));
    if (n < 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sockfd);
        SSL_CTX_free(ctx);
        return;
    }

    // HTTPSレスポンスの受信と処理
    while ((n = SSL_read(ssl, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[n] = '\0';
        fwrite(buffer, 1, n, file);
    }

    if (n < 0) {
        ERR_print_errors_fp(stderr);
    }

    // SSL接続の終了
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sockfd);
    SSL_CTX_free(ctx);

    if (download) {
        fclose(file);
    }
}