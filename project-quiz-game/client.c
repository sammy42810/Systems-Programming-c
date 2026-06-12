//Sam Bryan
//I pledge my honor that I have abided by the Stevens Honor System.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/select.h>

void parse_connect(int argc, char** argv, int* server_fd) {
    char* ip = "127.0.0.1";
    int port = 25555; 
    int opt;
    while ((opt = getopt(argc, argv, "i:p:h")) != -1) {
        if (opt == 'i')
        {
            ip = optarg;
        }
        else if (opt == 'p') 
        {
            port = atoi(optarg);
        }
        else if (opt == 'h')
        {
            printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n", argv[0]);
            printf("\n");
            printf("-i IP_address Default to \"127.0.0.1\";\n");
            printf("-p port_number Default to 25555;\n");
            printf("-h Display this help info.\n");
            exit(EXIT_SUCCESS);
        }
        else
        {
            fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
            exit(EXIT_FAILURE);
        }
    }
    *server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE); 
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);
    if (connect(*server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    int server_fd; 
    parse_connect(argc, argv, &server_fd);
    char buffer[1024];
    int len = recv(server_fd, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        printf("%s", buffer);
    }
    char name[128];
    fgets(name, sizeof(name), stdin);
    send(server_fd, name, strlen(name), 0);
    fd_set read_set;
    while (1) {
        FD_ZERO(&read_set);
        FD_SET(server_fd, &read_set);
        FD_SET(STDIN_FILENO, &read_set);
        if (select(server_fd + 1, &read_set, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(server_fd, &read_set)) {
            int len = recv(server_fd, buffer, sizeof(buffer) - 1, 0);
            if (len <= 0) {
                break;
            }
            buffer[len] = '\0';
            printf("%s", buffer);
        }
        if (FD_ISSET(STDIN_FILENO, &read_set)) {
            char ans[2];
            int len = read(STDIN_FILENO, ans, 1);
            if (len > 0 && ans[0] != '\n') {
                send(server_fd, ans, 1, 0);
            }
        } 
    }
    close(server_fd);
    return 0;
}