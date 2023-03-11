#include "server.h"

int main(void)
{
    // Default settings
    char ip_addr[] = "127.0.0.1";
    int port = 8800;
    char path_to_html[] = "./html/";

    run_server(ip_addr, port, path_to_html);

    return 0;
}
