#include "server.h"

#include <arpa/inet.h> // socket
#include <stdio.h>
#include <string.h> // strcmp()
#include <unistd.h> // read(), write()
#include <stdlib.h> // maloc()

#define BUFFER_SIZE 1024
#define PAGE_404    "/page404.html"

const char http_header[] = "HTTP/1.0 200 OK\r\n"
                           "Server: webserver-c\r\n"
                           "Content-type: text/html\r\n\r\n";
const char msg_resource_unavailable[] = "<html>Error: Resource unavailable."
                                        " The path to HTML files may be incorrectly"
                                        " specified.</html>";
const char msg_forbidden_extension[] = "<html>Sorry, it is allowed to access files"
                                       " only with the .html extension";

char* concat_string(const char *s1, const char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);

    char *result = malloc((len1 + len2 + 1) * sizeof(char));

    if (!result) {;
        perror("webserver (concat_string)");
        return NULL;
    }

    memcpy(result, s1, len1);
    memcpy(result + len1, s2, len2 + 1);

    return result;
}

char * read_file(const char *path_to_html,
                 const char *filename)
{
    // Check file extension (allowed to open only *.html)
    char *file_extension = (char*)filename + (strlen(filename) - 5);
    printf("file_extension = %s\n", file_extension);
    if (strcmp(file_extension, ".html") != 0) {
        printf("it is allowed to access files only with the .html extension\n");
        char *buffer = (char*)malloc(sizeof(msg_resource_unavailable));
        strcpy(buffer, msg_forbidden_extension);
        return buffer;
    }

    // Try to open the file
    char *path_plus_filename = concat_string(path_to_html, filename);

    FILE* file = fopen(path_plus_filename, "r");
    if (file == NULL) {
        // If impossible to open the file, then open the file with error 404
        printf("%s does NOT exist \n", path_plus_filename);
        free(path_plus_filename);
        if (strcmp(PAGE_404, filename)) {
            return read_file(path_to_html, PAGE_404);
        }
        else {
            printf("can't open %s\n", PAGE_404);
            char *buffer = (char*)malloc(sizeof(msg_resource_unavailable));
            strcpy(buffer, msg_resource_unavailable);
            return buffer;
        }
    }
        
    printf("%s does exist \n", path_plus_filename);
    free(path_plus_filename);
    path_plus_filename = NULL;

    // Determine number of characters in the file
    if (fseek(file, 0, SEEK_END) != 0 ) {
        perror("webserver (fseek)");
    }
    long fsize = ftell(file);
    if (fsize == -1) {
        perror("webserver (ftell)");
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        perror("webserver (fseek)");
    }

    // Read file into buffer
    char* buffer = malloc(sizeof(char) * (fsize + 1));
    fread(buffer, sizeof(char), fsize, file);
    buffer[fsize] = '\0';
    fclose(file);
    return buffer;
}

void request_processing(int sockfd, 
                        struct sockaddr_in *host_addr,
                        int host_addrlen,
                        const char *path_to_html)
{
    // Create client address
    struct sockaddr_in client_addr;
    int client_addrlen = sizeof(client_addr);

    char buffer[BUFFER_SIZE];

    for (;;) {
        // Accept incoming connections
        int newsockfd = accept(sockfd,
                               (struct sockaddr *)host_addr,
                               (socklen_t *)&host_addrlen);
        if (newsockfd < 0) {
            perror("webserver (accept)");
            continue;
        }
        printf("connection accepted\n");

        // Get client address
        int sockn = getsockname(newsockfd,
                                (struct sockaddr *)&client_addr,
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
        char method[BUFFER_SIZE], uri[BUFFER_SIZE], version[BUFFER_SIZE];
        sscanf(buffer, "%s %s %s", method, uri, version);
        printf("\n[%s:%u] %s %s %s\n", inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), method, version, uri);

        // Check last symbol in uri. If it is '/', then add "index.html"
        if (uri[strlen(uri) - 1] == '/') {
            strcat(uri, "index.html");
        }

        // Copy file contents into char buffer
        char *file_contents = read_file(path_to_html, uri);
        // Assert: read_file() can not return NULL

        char *response = concat_string(http_header, file_contents);
        //printf("\nresponse:\n%s",response);

        // Write to the socket
        int valwrite = write(newsockfd, response, strlen(response) * sizeof(char));
        if (valwrite < 0) {
            perror("webserver (write)");
            continue;
        }

        close(newsockfd);
        free(file_contents);
        free(response);
    }
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
        perror("webserver (setsockopt)");
        return 1;
    }

    // Create the address to bind the socket to
    struct sockaddr_in host_addr;
    int host_addrlen = sizeof(host_addr);

    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(port);
    host_addr.sin_addr.s_addr = inet_addr(ip_addr);


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

    request_processing(sockfd,
                       &host_addr,
                       host_addrlen,
                       path_to_html);

    return 0;
}
