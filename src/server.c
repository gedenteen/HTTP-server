#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h> //maloc()

#define BUFFER_SIZE 1024
#define PAGE_404    "page404.html"

char* concat(const char *s1, const char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);

    char *result = malloc((len1 + len2 + 1) * sizeof(char));

    if (!result) {;
        perror("concat (memory allocation)");
        return NULL;
    }

    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);

    return result;
}

char * read_file(const char *path_to_html,
                 const char *filename)
{
    char *path_plus_filename = concat(path_to_html, filename);

    FILE* file = fopen(path_plus_filename, "r");
    if (file == NULL) {
        // If impossible to open the file, then open the file with error 404
        printf("%s does NOT exist \n", path_plus_filename);
        free(path_plus_filename);
        if (strcmp(PAGE_404, filename)) {
            return read_file(path_to_html, PAGE_404);
        }
        else {
            // If we are here, it means that it is impossible to read the file with error 404
            return NULL;
        }
    }
        
    printf("%s does exist \n", path_plus_filename);
    free(path_plus_filename);
    path_plus_filename = NULL;

    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* temp = malloc(sizeof(char) * (fsize+1));
    char ch;
    int i = 0;
    while ((ch = fgetc(file)) != EOF) {
        temp[i] = ch;
        i++;
    }
    fclose(file);
    return temp;
}

int run_server(const char *ip_addr,
               const int port,
               const char *path_to_html)
{
    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("webserver (socket)");
        return 1;
    }
    printf("socket created successfully\n");

    // If the socket is already in use, then reinitialize it
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        perror("setsockopt");
        return 1;
    }

    // Create the address to bind the socket to
    struct sockaddr_in host_addr;
    int host_addrlen = sizeof(host_addr);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    host_addr.sin_addr.s_addr = inet_addr(ip_addr);

    // Create client address
    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);

    // Bind the socket to the address
    if (bind(sockfd, (struct sockaddr *)&host_addr, host_addrlen) != 0) {
        perror("webserver (bind)");
        return 1;
    }
    printf("socket successfully bound to address\n");

    // Listen for incoming connections
    if (listen(sockfd, SOMAXCONN) != 0) {
        perror("webserver (listen)");
        return 1;
    }
    printf("server listening for connections\n");

    char buffer[BUFFER_SIZE];
    // char resp[] = "HTTP/1.0 200 OK\r\n"
    //               "Server: webserver-c\r\n"
    //               "Content-type: text/html\r\n\r\n"
    //               "<html>hello, world</html>\r\n";

    char http_header[] = "HTTP/1.0 200 OK\r\n"
                         "Server: webserver-c\r\n"
                         "Content-type: text/html\r\n\r\n";


    for (;;) {
        // Accept incoming connections
        int newsockfd = accept(sockfd,
                               (struct sockaddr *)&host_addr,
                               (socklen_t *)&host_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");

        // Get client address
        int sockn = getsockname(newsockfd, (struct sockaddr *)&client_addr,
                                (socklen_t *)&client_addrlen);
        if (sockn < 0) {
            perror("webserver (getsockname)");
            continue;
        }

        // Read from the socket
        int valread = read(newsockfd, buffer, BUFFER_SIZE);
        if (valread < 0) {
            perror("webserver (read)");
            continue;
        }

        // Read the request
        char method[BUFFER_SIZE], filename[BUFFER_SIZE], version[BUFFER_SIZE];
        sscanf(buffer, "%s %s %s", method, filename, version);
        printf("[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), method, version, filename);


        // Copy file contents (filename without '/' in begin)
        char *file_contents = read_file(path_to_html, (filename+1));
        if (file_contents == NULL) {
            // TODO
            return 0;
        }

        int response_size = 2048;
        char *response = (char*)malloc(response_size);
        strcat(response, http_header);
        strcat(response, file_contents);
        strcat(response, "\r\n\r\n");
        printf("\nresponse:\n%s",response);

        // Write to the socket
        int valwrite = write(newsockfd, response, response_size);
        if (valwrite < 0) {
            perror("webserver (write)");
            continue;
        }

        close(newsockfd);
        free(file_contents);
        free(response);
    }

    return 0;
}
