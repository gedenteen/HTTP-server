#include "server.h"

#include <string.h> // strcmp()
#include <stdlib.h> // atoi()
#include <linux/limits.h> // PATH_MAX
#include <stdio.h>

int main(int argc, char *argv[])
{
    const int ip_addr_max_len = 16; // 4 numbers of 3 digits + 3 dots + '\0' = 16

    // Default settings
    char ip_addr[16] = "127.0.0.1";
    int port = 8800;
    char path_to_html[PATH_MAX] = "./html";

    // Process input parameters
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0) {
            i++;
            port = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "--ipaddr") == 0) {
            i++;
            strncpy(ip_addr, argv[i], ip_addr_max_len);
        }
        else if (strcmp(argv[i], "--path") == 0) {
            i++;
            strncpy(path_to_html, argv[i], PATH_MAX);
        }
        else {
            fprintf(stderr, "error: unknown parameter %s\n", argv[i]);
        }
    }

    printf("HTTP server creating... IP-addres = %s, TCP port = %d, path to HTML files = %s\n",
           ip_addr, port, path_to_html);
    run_server(ip_addr, port, path_to_html);

    return 0;
}
