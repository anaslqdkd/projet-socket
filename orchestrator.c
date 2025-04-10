#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct sockaddr_in serv_addr;
#define BUFFER_SIZE 1000

void func (int sockfd) {
    char buff[BUFFER_SIZE];
    int n;
    while (1) {
        bzero (buff, sizeof (buff));
        printf ("Enter a command to execute: ");
        n = 0;
        while ((buff[n++] = getchar ()) != '\n') {
        }
        write (sockfd, buff, sizeof (buff));
        bzero (buff, sizeof (buff));
        read (sockfd, buff, sizeof (buff));
        printf ("From Server : %s", buff);

        if ((strncmp (buff, "exit", 4)) == 0) {
            printf ("Client Exit...\n");
            break;
        }
    }
}

int main (int argc, char** argv) {
    int sockfd;
    sockfd = socket (PF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror ("erreur socket");
        exit (1);
    }

    bzero ((char*)&serv_addr, sizeof (serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons ((ushort)atoi (argv[2]));
    serv_addr.sin_addr.s_addr = inet_addr (argv[1]);

    if (connect (sockfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr)) < 0) {
        perror ("cliecho : erreur connect");
        exit (1);
    } else {
        printf ("connected to the server\n");
    }
    func (sockfd);
    close (sockfd);
}
