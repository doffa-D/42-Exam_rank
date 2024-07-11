#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>


int server, client, size;
struct sockaddr_in servaddr, cli; 
socklen_t len = sizeof(cli);


int db[100000];
char *messages[100000], buffer2[1024], buffer[42];
int limit = 0;

fd_set fd_master, fd_read, fd_write;



void fatal()
{
    write(2, "Fatal error\n", strlen("Fatal error\n"));
    exit(1);
}

int extract_message(char **buf, char **msg)
{
    char    *newbuf;
    int    i;

    *msg = 0;
    if (*buf == 0)
        return (0);
    i = 0;
    while ((*buf)[i])
    {
        if ((*buf)[i] == '\n')
        {
            newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
            if (newbuf == 0)
                fatal();
            strcpy(newbuf, *buf + i + 1);
            *msg = *buf;
            (*msg)[i + 1] = 0;
            *buf = newbuf;
            return (1);
        }
        i++;
    }
    return (0);
}

char *str_join(char *buf, char *add)
{
    char    *newbuf;
    int        len;

    if (buf == 0)
        len = 0;
    else
        len = strlen(buf);
    newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
    if (newbuf == 0)
        fatal();
    newbuf[0] = 0;
    if (buf != 0)
        strcat(newbuf, buf);
    free(buf);
    strcat(newbuf, add);
    return (newbuf);
}

void send_all(int sender, char *msg)
{
    for(int fd = 0; fd <= size; fd++)
    {
        if (fd != sender && FD_ISSET(fd, &fd_write))
            send(fd, msg, strlen(msg), 0);
    }
}

int main(int ac, char **av) {
    if (ac != 2)
    {
        write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
        return 1;
    }
    
    server = socket(AF_INET, SOCK_STREAM, 0); 
    if (server == -1)
        fatal();
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
    servaddr.sin_port = htons(atoi(av[1])); 
      if ((bind(server, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        fatal();    
    if (listen(server, 0) != 0)
        fatal();
    FD_ZERO(&fd_master);
    FD_SET(server, &fd_master);
    size = server;
    while (1)
    {
        fd_read = fd_write = fd_master;
        if (select(size + 1, &fd_read, &fd_write, NULL, NULL) < 0)
            fatal();
        for(int fd = 0; fd <= size; fd++)
        {
            if (!FD_ISSET(fd, &fd_read))
                continue;
            if (fd == server)
            {
                client = accept(server, (struct sockaddr *)&cli, &len);
                if (client >= 0)
                {
                    if (client > size)
                        size = client;
                    db[client] = limit++;
                    messages[client] = NULL;
                    sprintf(buffer, "server: client %d just arrived\n", db[client]);
                    send_all(client, buffer);
                    FD_SET(client, &fd_master);
                    break;
                }
            }
            else
            {
                int byte = recv(fd, buffer2, 1023, 0);
                if (byte <= 0)
                {
                    sprintf(buffer, "server: client %d just left\n", db[fd]);
                    send_all(fd, buffer);
                    free(messages[fd]);
                    FD_CLR(fd, &fd_master);
                    close(fd);
                    break;
                }
                buffer2[byte] = 0;
                messages[fd] = str_join(messages[fd], buffer2);
                char *msg = NULL;

                while(extract_message(&(messages[fd]), &msg))
                {
                    sprintf(buffer, "client %d: ", db[fd]);
                    send_all(fd, buffer);
                    send_all(fd, msg);
                    free(msg);
                }
            }
        }
    }
    return 0;
}
