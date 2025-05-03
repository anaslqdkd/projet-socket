#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 100000

typedef enum {
    NMAP,
    ZAP,
    NIKTO,
    NONE,
} scanner;

scanner get_scanner_type (char* command) {
    if (strstr (command, "nmap") != NULL) {
        return NMAP;
    } else if (strstr (command, "zap") != NULL ||
    strstr (command, "zap-cli") != NULL || strstr (command, "zap.sh") != NULL) {
        return ZAP;
    } else if (strstr (command, "nikto") != NULL) {
        return NIKTO;
    } else {
        return NONE;
    }
}

void run_scanner (SSL* ssl, char* command) {
    FILE* fp;
    char output[BUFFER_SIZE];
    char* full_output = NULL;
    size_t total_size = 0;
    scanner scanner   = get_scanner_type (command);

    switch (scanner) {
    case NMAP:
        printf ("Lancement du scanner Nmap avec la command : %s\n", command);
        break;
    case ZAP:
        printf ("Lancement du scanner Zap avec la command : %s\n", command);
        break;
    case NIKTO:
        printf ("Lancement du scanner Nikto avec la command : %s\n", command);
        break;
    case NONE: printf ("Command inconnue %s\n", command); break;
    }

    char cmd_with_redirect[BUFFER_SIZE];
    snprintf (cmd_with_redirect, sizeof (cmd_with_redirect), "%s 2>&1", command);

    fp = popen (cmd_with_redirect, "r");
    if (fp == NULL) {
        const char* error = "Erreur lors de l'exécution de la commande.\n";
        SSL_write (ssl, error, strlen (error));
        return;
    }

    while (fgets (output, sizeof (output), fp) != NULL) {
        size_t len = strlen (output);
        char* temp = realloc (full_output, total_size + len + 1);
        if (temp == NULL) {
            free (full_output);
            const char* error = "Erreur d’allocation mémoire.\n";
            SSL_write (ssl, error, strlen (error));
            pclose (fp);
            return;
        }

        full_output = temp;
        memcpy (full_output + total_size, output, len);
        total_size += len;
        full_output[total_size] = '\0';
    }

    pclose (fp);

    if (total_size == 0) {
        const char* msg = "Aucune sortie de la commande.\n";
        SSL_write (ssl, msg, strlen (msg));
    } else {
        SSL_write (ssl, full_output, total_size);
    }

    free (full_output);
}


void func (SSL* ssl) {
    char buff[BUFFER_SIZE];
    while (1) {
        bzero (buff, BUFFER_SIZE);
        int bytes = SSL_read (ssl, buff, sizeof (buff));
        if (bytes <= 0)
            break;

        printf ("Reçu de l'orchestrateur : %s", buff);
        buff[bytes] = '\0';
        if (strstr (buff, "nmap") != NULL) {
            run_scanner (ssl, buff);
        } else if (strstr (buff, "zap") != NULL ||
        strstr (buff, "zap-cli") != NULL || strstr (buff, "zap.sh") != NULL) {
            run_scanner (ssl, buff);
        } else if (strstr (buff, "nikto") != NULL) {
            run_scanner (ssl, buff);
        }
    }
}

int main (int argc, char* argv[]) {
    if (argc != 2) {
        fprintf (stderr, "Usage: %s <port>\n", argv[0]);
        exit (EXIT_FAILURE);
    }
    int port = atoi (argv[1]);

    // OpenSSL setup
    SSL_library_init ();
    SSL_load_error_strings ();
    OpenSSL_add_all_algorithms ();

    SSL_CTX* ctx = SSL_CTX_new (TLS_server_method ());
    if (!ctx) {
        ERR_print_errors_fp (stderr);
        exit (EXIT_FAILURE);
    }

    // Load agent's certificate and private key
    if (SSL_CTX_use_certificate_file (ctx, "certs/agent-cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp (stderr);
        exit (EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file (ctx, "certs/agent-key.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp (stderr);
        exit (EXIT_FAILURE);
    }

    // Verify that the private key matches the certificate
    if (!SSL_CTX_check_private_key (ctx)) {
        fprintf (stderr, "La clé privée ne correspond pas au certificat public\n");
        exit (EXIT_FAILURE);
    }

    // Load CA certificate to verify the orchestrator's certificate
    if (SSL_CTX_load_verify_locations (ctx, "ca/ca-cert.pem", NULL) <= 0) {
        ERR_print_errors_fp (stderr);
        exit (EXIT_FAILURE);
    }

    // Set the verification mode to verify the orchestrator's certificate
    SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_verify_depth (ctx, 1);

    // TCP socket setup
    int serverSocket = socket (AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen = sizeof (cli_addr);

    bzero (&serv_addr, sizeof (serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port        = htons (port);

    if (bind (serverSocket, (struct sockaddr*)&serv_addr, sizeof (serv_addr)) < 0) {
        perror ("Erreur bind");
        exit (1);
    }

    if (listen (serverSocket, SOMAXCONN) < 0) {
        perror ("Erreur listen");
        exit (1);
    }

    printf ("Agent écoutant sur le port %d...\n", port);

    int clientSocket = accept (serverSocket, (struct sockaddr*)&cli_addr, &clilen);
    if (clientSocket < 0) {
        perror ("Erreur accept");
        exit (1);
    }

    printf ("Orchestrateur connecté !\n");

    // TLS handshake
    SSL* ssl = SSL_new (ctx);
    SSL_set_fd (ssl, clientSocket);
    if (SSL_accept (ssl) <= 0) {
        ERR_print_errors_fp (stderr);
        close (clientSocket);
        SSL_free (ssl);
        SSL_CTX_free (ctx);
        return 1;
    }

    // Verify the orchestrator's certificate
    X509* cert = SSL_get_peer_certificate (ssl);
    if (cert == NULL) {
        fprintf (stderr, "L'orchestrateur n'a pas fourni de certificat !\n");
        SSL_shutdown (ssl);
        SSL_free (ssl);
        close (clientSocket);
        SSL_CTX_free (ctx);
        return 1;
    }

    // Check if the orchestrator's certificate is valid
    if (SSL_get_verify_result (ssl) != X509_V_OK) {
        fprintf (stderr, "Le certificat de l'orchestrateur est invalide !\n");
        X509_free (cert);
        SSL_shutdown (ssl);
        SSL_free (ssl);
        close (clientSocket);
        SSL_CTX_free (ctx);
        return 1;
    }

    printf ("Certificat de l'orchestrateur vérifié avec succès !\n");
    X509_free (cert);

    func (ssl);

    SSL_shutdown (ssl);
    SSL_free (ssl);
    close (clientSocket);
    close (serverSocket);
    SSL_CTX_free (ctx);

    return 0;
}
