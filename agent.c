#include <netinet/in.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define SERV_PORT 2222
#define BUFFER_SIZE 1000

int dialogSocket;
int clilen;

struct sockaddr_in cli_addr;
struct sockaddr_in serv_addr;

void func (int sockfd) {
    /*TODO: add encryption with openssl*/
    char buff[BUFFER_SIZE];
    int n;
    while (1) {
        bzero (buff, BUFFER_SIZE);

        read (sockfd, buff, sizeof (buff));

        printf ("From client : %s", buff);
        FILE* fp = popen (buff, "r");
        bzero (buff, BUFFER_SIZE);

        bool valid_command = false;

        while (fgets (buff, sizeof (buff), fp) != NULL) {
            write (sockfd, buff, strlen (buff));
            valid_command = true;
        }
        if (!valid_command) {
            char* error_message = "the command is not valid\n";
            write (dialogSocket, error_message, strlen (error_message));
        }
        fclose (fp);
    }
}

int main () {
    int serverSocket;
    serverSocket = socket (PF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror ("error socket creation");
        exit (1);
    } else {
        printf ("socket created successfully\n");
    }

    bzero (&serv_addr, sizeof (serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    serv_addr.sin_port        = htons (SERV_PORT);

    if (bind (serverSocket, (struct sockaddr*)&serv_addr, sizeof (serv_addr)) < 0) {
        perror ("servecho: erreur bind\n");
        exit (1);
    }
    if (listen (serverSocket, SOMAXCONN) < 0) {
        perror ("servecho: erreur listen\n");
        exit (1);
    }
    printf ("Listening on port 2222\n");
    dialogSocket = accept (serverSocket, (struct sockaddr*)&cli_addr, (socklen_t*)&clilen);
    if (dialogSocket < 0) {
        perror ("servecho : erreur accep\n");
        exit (1);
    }

    printf ("Client connected!\n");
    while (1) {
        func (dialogSocket);
    }

    close (serverSocket);
    return 0;
}
