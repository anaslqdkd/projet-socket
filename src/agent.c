#include <netinet/in.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1000

int dialogSocket;
int clilen;

struct sockaddr_in cli_addr;
struct sockaddr_in serv_addr;

void func (int sockfd) {
    /*TODO: add encryption with openssl*/
    char buff[BUFFER_SIZE];
    while (1) {
        bzero (buff, BUFFER_SIZE);

        read (sockfd, buff, sizeof (buff));

        printf ("Reçu de l'orchestrateur : %s", buff);
        FILE* fp = popen (buff, "r");
        bzero (buff, BUFFER_SIZE);

        bool valid_command = false;

        while (fgets (buff, sizeof (buff), fp) != NULL) {
            write (sockfd, buff, strlen (buff));
            valid_command = true;
        }
        if (!valid_command) {
            char* error_message = "La commande envoyée n'est pas valide !\n";
            write (dialogSocket, error_message, strlen (error_message));
        }
        pclose (fp);
    }
}

int main (int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int port = atoi(argv[1]);

    int serverSocket = socket (PF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror ("Erreur dans la création de la socket.");
        exit (1);
    }
    bzero (&serv_addr, sizeof (serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    serv_addr.sin_port        = htons (port);

    if (bind (serverSocket, (struct sockaddr*)&serv_addr, sizeof (serv_addr)) < 0) {
        perror ("servecho: erreur bind\n");
        exit (1);
    }
    if (listen (serverSocket, SOMAXCONN) < 0) {
        perror ("servecho: erreur listen\n");
        exit (1);
    }
    printf ("Listening on port %s\n", argv[1]);
    dialogSocket = accept (serverSocket, (struct sockaddr*)&cli_addr, (socklen_t*)&clilen);
    if (dialogSocket < 0) {
        perror ("servecho : erreur accept\n");
        exit (1);
    }

    printf ("Orchestrateur connecté !\n");
    while (1) {
        func (dialogSocket);
    }

    close (serverSocket);
    return 0;
}
