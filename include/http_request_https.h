#ifndef HTTP_REQUEST_HTTPS_H
#define HTTP_REQUEST_HTTPS_H

#ifdef __cplusplus
extern "C" {
#endif

        /**
         * Fetches HTML content from the specified hostname using HTTPS.
         *
         * @param hostname The hostname from which to fetch HTML content.
         * @param download If set to 1, the content is saved to 'index.html'.
         */
void fetch_html_https(const char *hostname, int download);

        /**
         * Fetches CSS content from the specified URL using HTTPS.
         *
         * @param url The URL from which to fetch CSS content.
         * @param download If set to 1, the content is saved to a CSS file.
         */
void fetch_css_https(const char *url, int download);

        /**
         * Checks if HTTPS connection can be established with the specified hostname.
         *
         * @param hostname The hostname to check for HTTPS support.
         * @return Non-zero if HTTPS connection is possible, 0 otherwise.
         */
int can_connect_https(const char *hostname);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_REQUEST_HTTPS_H */