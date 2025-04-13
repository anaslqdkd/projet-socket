#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1000
#define MAX_AGENT_NAME 50
#define NUM_AGENTS 1

struct sockaddr_in serv_addr[NUM_AGENTS];
char agent_names[NUM_AGENTS][MAX_AGENT_NAME];

void send_command (SSL* ssl, const char* agent_name) {
    char buff[BUFFER_SIZE];
    int n;
    bzero (buff, sizeof (buff));
    printf ("Commande pour %s : ", agent_name);
    n = 0;
    while ((buff[n++] = getchar ()) != '\n') {
    }

    SSL_write (ssl, buff, strlen (buff));
    bzero (buff, sizeof (buff));
    SSL_read (ssl, buff, sizeof (buff));
    printf ("Réponse de %s : %s", agent_name, buff);

    if ((strncmp (buff, "exit", 4)) == 0) {
        printf ("Fermeture de la connexion avec %s...\n", agent_name);
    }
}

int main (int argc, char** argv) {
    if (argc < NUM_AGENTS + 2 || argc > 2 * NUM_AGENTS + 2) {
        fprintf (stderr,
        "Usage: %s <IP1> <IP2> <IP3> <port> [name1 name2 name3]\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    int port = atoi (argv[NUM_AGENTS + 1]);
    int sockfds[NUM_AGENTS];
    SSL* ssl_connections[NUM_AGENTS];

    // Optional agent names
    for (int i = 0; i < NUM_AGENTS; ++i) {
        if (argc > NUM_AGENTS + 2 + i) {
            strncpy (agent_names[i], argv[NUM_AGENTS + 2 + i], MAX_AGENT_NAME - 1);
            agent_names[i][MAX_AGENT_NAME - 1] = '\0';
        } else {
            snprintf (agent_names[i], MAX_AGENT_NAME, "Agent %d", i + 1);
        }
    }

    // Init OpenSSL
    SSL_library_init ();
    SSL_load_error_strings ();
    OpenSSL_add_all_algorithms ();

    SSL_CTX* ctx = SSL_CTX_new (TLS_client_method ());
    if (ctx == NULL) {
        ERR_print_errors_fp (stderr);
        exit (EXIT_FAILURE);
    }

    // Load the certificate and private key for the orchestrator
    if (SSL_CTX_use_certificate_file (
        ctx, "certs/orchestrator-cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp (stderr);
        exit (EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file (ctx, "certs/orchestrator-key.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp (stderr);
        exit (EXIT_FAILURE);
    }

    // Verify the private key matches the certificate
    if (!SSL_CTX_check_private_key (ctx)) {
        fprintf (stderr, "La clé privée ne correspond pas au certificat.\n");
        exit (EXIT_FAILURE);
    }

    // Setup and connect to each agent with SSL
    for (int i = 0; i < NUM_AGENTS; ++i) {
        sockfds[i] = socket (PF_INET, SOCK_STREAM, 0);
        if (sockfds[i] < 0) {
            perror ("Erreur socket");
            exit (EXIT_FAILURE);
        }

        bzero (&serv_addr[i], sizeof (serv_addr[i]));
        serv_addr[i].sin_family      = AF_INET;
        serv_addr[i].sin_port        = htons ((ushort)port);
        serv_addr[i].sin_addr.s_addr = inet_addr (argv[i + 1]);

        if (connect (sockfds[i], (struct sockaddr*)&serv_addr[i],
            sizeof (serv_addr[i])) < 0) {
            fprintf (stderr, "Erreur de connexion à %s (%s)\n", agent_names[i],
            argv[i + 1]);
            close (sockfds[i]);
            exit (EXIT_FAILURE);
        }

        // Setup SSL over the connected socket
        SSL* ssl = SSL_new (ctx);
        SSL_set_fd (ssl, sockfds[i]);

        if (SSL_connect (ssl) <= 0) {
            fprintf (stderr, "Échec SSL avec %s (%s)\n", agent_names[i], argv[i + 1]);
            ERR_print_errors_fp (stderr);
            SSL_free (ssl);
            close (sockfds[i]);
            exit (EXIT_FAILURE);
        }

        ssl_connections[i] = ssl;

        printf ("Connecté en TLS à %s (%s) avec succès !\n", agent_names[i], argv[i + 1]);
    }

    // Command loop
    while (1) {
        int target;
        printf ("\nÀ quel agent voulez-vous envoyer la commande ? (1 à %d, ou "
                "0 pour quitter): ",
        NUM_AGENTS);
        scanf ("%d", &target);
        while (getchar () != '\n')
            ; // Clear input buffer

        if (target == 0) {
            printf ("Fermeture des connexions...\n");
            break;
        }

        if (target < 1 || target > NUM_AGENTS) {
            printf ("Numéro d'agent invalide.\n");
            continue;
        }

        send_command (ssl_connections[target - 1], agent_names[target - 1]);
    }

    // Cleanup
    for (int i = 0; i < NUM_AGENTS; ++i) {
        SSL_shutdown (ssl_connections[i]);
        SSL_free (ssl_connections[i]);
        close (sockfds[i]);
    }
    SSL_CTX_free (ctx);
    EVP_cleanup ();
    return 0;
}
