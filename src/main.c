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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/http_request.h"
#include "../include/http_request_https.h"

int
main(int argc, char* argv[])
{
        int download_html = 0;
        int download_css = 0;
        int download_file = 0;
        const char* url = NULL;

        // コマンドライン引数の解析
        for (int i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-d") == 0) {
                        download_html = 1;
                } else if (strcmp(argv[i], "-c") == 0) {
                        download_css = 1;
                } else if (strcmp(argv[i], "-dl") == 0) {
                        download_file = 1;
                } else {
                        url = argv[i];
                }
        }

        if (!url || (download_html && download_file)) {
                fprintf(stderr, "Usage: %s [-d] [-c] [-dl] [http:// or https://]hostname\n", argv[0]);
                exit(1);
        }

        if (download_file) {
                // 指定されたURLからファイルをダウンロード
                download_file_https(url);
        } else {
                // URLスキーム（http:// または https://）を確認
                if (strncmp(url, "https://", 8) == 0) {
                        fetch_html_https(url + 8, download_html); // "https://" を除いた部分を渡す
                        if (download_css) {
                                fetch_css_https(url + 8, 1); // CSSのダウンロード
                        }
                } else if (strncmp(url, "http://", 7) == 0) {
                        fetch_html(url + 7, download_html); // "http://" を除いた部分を渡す
                        if (download_css) {
                                // CSSのダウンロード（HTTP版の関数が必要）
                                // fetch_css(url + 7, 1);
                        }
                } else {
                        // スキームが指定されていない場合
                        if (can_connect_https(url)) {
                                fetch_html_https(url, download_html);
                                if (download_css) {
                                        fetch_css_https(url, 1);
                                }
                        } else {
                                fetch_html(url, download_html);
                                if (download_css) {
                                        // fetch_css(url, 1); // HTTP版の関数が必要
                                }
                        }
                }
        }

        return 0;
}