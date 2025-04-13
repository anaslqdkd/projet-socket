#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1000
#define NUM_AGENTS 3
#define MAX_AGENT_NAME 50

struct sockaddr_in serv_addr[NUM_AGENTS];
char agent_names[NUM_AGENTS][MAX_AGENT_NAME];

void send_command(int sockfd, const char* agent_name) {
    char buff[BUFFER_SIZE];
    int n;
    bzero(buff, sizeof(buff));
    printf("Commande pour %s : ", agent_name);
    n = 0;
    while ((buff[n++] = getchar()) != '\n') {}

    write(sockfd, buff, sizeof(buff));
    bzero(buff, sizeof(buff));
    read(sockfd, buff, sizeof(buff));
    printf("Réponse de %s : %s", agent_name, buff);

    if ((strncmp(buff, "exit", 4)) == 0) {
        printf("Fermeture de la connexion avec %s...\n", agent_name);
    }
}

int main(int argc, char** argv) {
    if (argc < NUM_AGENTS + 2 || argc > 2 * NUM_AGENTS + 2) {
        fprintf(stderr, "Usage: %s <IP1> <IP2> <IP3> <port> [name1 name2 name3]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[NUM_AGENTS + 1]);
    int sockfds[NUM_AGENTS];

    // Optional agent names
    for (int i = 0; i < NUM_AGENTS; ++i) {
        if (argc > NUM_AGENTS + 2 + i) {
            strncpy(agent_names[i], argv[NUM_AGENTS + 2 + i], MAX_AGENT_NAME - 1);
            agent_names[i][MAX_AGENT_NAME - 1] = '\0'; // Ensure null-termination
        } else {
            snprintf(agent_names[i], MAX_AGENT_NAME, "Agent %d", i + 1);
        }
    }

    // Setup and connect to each agent
    for (int i = 0; i < NUM_AGENTS; ++i) {
        sockfds[i] = socket(PF_INET, SOCK_STREAM, 0);
        if (sockfds[i] < 0) {
            perror("Erreur socket");
            exit(EXIT_FAILURE);
        }

        bzero(&serv_addr[i], sizeof(serv_addr[i]));
        serv_addr[i].sin_family = AF_INET;
        serv_addr[i].sin_port = htons((ushort)port);
        serv_addr[i].sin_addr.s_addr = inet_addr(argv[i + 1]);

        if (connect(sockfds[i], (struct sockaddr*)&serv_addr[i], sizeof(serv_addr[i])) < 0) {
            fprintf(stderr, "Erreur de connexion à %s (%s)\n", agent_names[i], argv[i + 1]);
            exit(EXIT_FAILURE);
        } else {
            printf("Connecté à %s (%s) avec succès !\n", agent_names[i], argv[i + 1]);
        }
    }

    // Command loop
    while (1) {
        int target;
        printf("\nÀ quel agent voulez-vous envoyer la commande ? (1 à %d, ou 0 pour quitter): ", NUM_AGENTS);
        scanf("%d", &target);
        while (getchar() != '\n'); // Clear input buffer
    
        if (target == 0) {
            printf("Fermeture des connexions...\n");
            break;
        }
    
        if (target < 1 || target > NUM_AGENTS) {
            printf("Numéro d'agent invalide.\n");
            continue;
        }
    
        send_command(sockfds[target - 1], agent_names[target - 1]);
    }
    

    for (int i = 0; i < NUM_AGENTS; ++i) {
        close(sockfds[i]);
    }

    return 0;
}
