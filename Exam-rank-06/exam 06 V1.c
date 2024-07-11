#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

const int BUFFER_SIZE = 42 * 4096;
typedef struct client {
    int id;
    char msg[BUFFER_SIZE];
} t_client;

t_client clients[1024];

int size = 0, limit = 0;
fd_set fd_master, fd_write, fd_read;
char buffer2[BUFFER_SIZE], buffer[BUFFER_SIZE];

void fatal_error() {
    write(2, "Fatal error\n", 12);
    exit(1);
}

void send_all(int sender) {
    for (int fd = 0; fd <= size; fd++) {
        if (FD_ISSET(fd, &fd_write) && fd != sender) {
            send(fd, buffer, strlen(buffer), 0);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

    bzero(&clients, sizeof(clients));

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0)
        fatal_error();

    size = server;
    FD_ZERO(&fd_master);
    FD_SET(server, &fd_master);

    struct sockaddr_in sockaddr, cli;
    socklen_t len = sizeof(sockaddr);
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(2130706433);
    sockaddr.sin_port = htons(atoi(argv[1]));

    if ((bind(server, (const struct sockaddr *)&sockaddr, sizeof(sockaddr))) < 0)
        fatal_error();
    if (listen(server, 128) < 0)
        fatal_error();

    while (1) {
        fd_read = fd_write = fd_master;
        if (select(size + 1, &fd_read, &fd_write, NULL, NULL) < 0)
            continue;

        for (int fd = 0; fd <= size; fd++) {
            if (FD_ISSET(fd, &fd_read)) {
                if (fd == server) {
                    int client = accept(server, (struct sockaddr *)&cli, &len);
                    if (client < 0)
                        continue;
                    if (client > size)
                        size = client;
                    clients[client].id = limit++;
                    FD_SET(client, &fd_master);
                    sprintf(buffer, "server: client %d just arrived\n", clients[client].id);
                    send_all(client);
                } else {
                    int byte = recv(fd, buffer2, BUFFER_SIZE, 0);
                    if (byte <= 0) {
                        sprintf(buffer, "server: client %d just left\n", clients[fd].id);
                        send_all(fd);
                        FD_CLR(fd, &fd_master);
                        close(fd);
                    } else {
                        int msg_len = strlen(clients[fd].msg);
                        for (int i = 0, j = msg_len; i < byte && j < BUFFER_SIZE; i++, j++) {
                            clients[fd].msg[j] = buffer2[i];
                            if (clients[fd].msg[j] == '\n') {
                                clients[fd].msg[j] = '\0';
                                sprintf(buffer, "client %d: %s\n", clients[fd].id, clients[fd].msg);
                                send_all(fd);
                                bzero(&clients[fd].msg, BUFFER_SIZE);
                                msg_len = 0;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
