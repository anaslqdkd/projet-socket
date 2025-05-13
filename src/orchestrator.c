#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1000
#define MAX_AGENT_NAME 50
#define NUM_AGENTS 3
#define MAX_COMMAND_LENGTH 256

// Structure to pass data to threads
typedef struct {
    int agent_id;
    SSL* ssl;
    char name[MAX_AGENT_NAME];
    int running;
    pthread_mutex_t mutex;
    char command_buffer[BUFFER_SIZE];
    int has_command;
} agent_thread_data_t;

// Global variables
agent_thread_data_t agent_data[NUM_AGENTS];
pthread_t agent_threads[NUM_AGENTS];
pthread_mutex_t global_print_mutex;

void* agent_handler (void* arg);
void send_command_to_agent (int agent_id, const char* command);
void setup_ssl_connections (char** argv, int port);
void cleanup_connections ();

int main (int argc, char** argv) {
    if (argc < NUM_AGENTS + 2 || argc > 2 * NUM_AGENTS + 2) {
        fprintf (stderr,
        "Usage: %s <IP1> <IP2> <IP3> <port> [name1 name2 name3]\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    int port = atoi (argv[NUM_AGENTS + 1]);

    for (int i = 0; i < NUM_AGENTS; ++i) {
        if (argc > NUM_AGENTS + 2 + i) {
            strncpy (agent_data[i].name, argv[NUM_AGENTS + 2 + i], MAX_AGENT_NAME - 1);
            agent_data[i].name[MAX_AGENT_NAME - 1] = '\0';
        } else {
            snprintf (agent_data[i].name, MAX_AGENT_NAME, "Agent %d", i + 1);
        }
        agent_data[i].agent_id    = i;
        agent_data[i].running     = 1;
        agent_data[i].has_command = 0;
        pthread_mutex_init (&agent_data[i].mutex, NULL);
    }

    pthread_mutex_init (&global_print_mutex, NULL);

    setup_ssl_connections (argv, port);

    for (int i = 0; i < NUM_AGENTS; ++i) {
        if (pthread_create (&agent_threads[i], NULL, agent_handler,
            (void*)&agent_data[i]) != 0) {
            perror ("Erreur de creation de thread");
            exit (EXIT_FAILURE);
        }
    }

    char input_buffer[MAX_COMMAND_LENGTH];
    int target;

    // Main command loop
    while (1) {
        printf ("\nChoose an agent to send a command (1-%d) or 0 to exit: ", NUM_AGENTS);
        if (scanf ("%d", &target) != 1) {
            // Clear input buffer on error
            while (getchar () != '\n')
                ;
            continue;
        }

        // Clear the newline from the input buffer
        while (getchar () != '\n')
            ;

        if (target == 0) {
            break;
        }

        if (target < 1 || target > NUM_AGENTS) {
            printf ("Invalid agent number.\n");
            continue;
        }

        printf ("Command for %s: ", agent_data[target - 1].name);
        if (fgets (input_buffer, MAX_COMMAND_LENGTH, stdin) == NULL) {
            continue;
        }

        // Remove trailing newline
        input_buffer[strcspn (input_buffer, "\n")] = 0;

        // Send command to the selected agent
        send_command_to_agent (target - 1, input_buffer);

        // Check if exit command
        if (strcmp (input_buffer, "exit") == 0) {
            printf ("Closing connection with %s...\n", agent_data[target - 1].name);
        }
    }

    // Signal all threads to exit and wait for them
    for (int i = 0; i < NUM_AGENTS; ++i) {
        agent_data[i].running = 0;
        pthread_join (agent_threads[i], NULL);
    }

    // Cleanup all resources
    cleanup_connections ();

    return 0;
}

void* agent_handler (void* arg) {
    agent_thread_data_t* data = (agent_thread_data_t*)arg;
    char response[BUFFER_SIZE];

    while (data->running) {
        // Check if there's a command to send
        pthread_mutex_lock (&data->mutex);
        int has_command = data->has_command;
        char command[BUFFER_SIZE];
        if (has_command) {
            strcpy (command, data->command_buffer);
            data->has_command = 0;
        }
        pthread_mutex_unlock (&data->mutex);

        if (has_command) {
            // Send command to agent
            SSL_write (data->ssl, command, strlen (command));

            // Read response
            bzero (response, sizeof (response));
            int bytes = SSL_read (data->ssl, response, sizeof (response));

            if (bytes > 0) {
                // Thread-safe printing
                pthread_mutex_lock (&global_print_mutex);
                printf ("\n[%s Response]: %s\n", data->name, response);
                printf (
                "\nChoose an agent to send a command (1-%d) or 0 to exit: ", NUM_AGENTS);
                fflush (stdout);
                pthread_mutex_unlock (&global_print_mutex);
            }
        }

        usleep (100000);
    }

    return NULL;
}

// Send a command to an agent via its thread
void send_command_to_agent (int agent_id, const char* command) {
    pthread_mutex_lock (&agent_data[agent_id].mutex);
    strncpy (agent_data[agent_id].command_buffer, command, BUFFER_SIZE - 1);
    agent_data[agent_id].command_buffer[BUFFER_SIZE - 1] = '\0';
    agent_data[agent_id].has_command                     = 1;
    pthread_mutex_unlock (&agent_data[agent_id].mutex);
}

// Setup SSL connections to all agents
void setup_ssl_connections (char** argv, int port) {
    struct sockaddr_in serv_addr[NUM_AGENTS];
    int sockfds[NUM_AGENTS];

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

    // Connect to each agent
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
            fprintf (stderr, "Erreur de connexion à %s (%s)\n",
            agent_data[i].name, argv[i + 1]);
            close (sockfds[i]);
            exit (EXIT_FAILURE);
        }

        // Setup SSL over the connected socket
        SSL* ssl = SSL_new (ctx);
        SSL_set_fd (ssl, sockfds[i]);

        if (SSL_connect (ssl) <= 0) {
            fprintf (stderr, "Échec SSL avec %s (%s)\n", agent_data[i].name, argv[i + 1]);
            ERR_print_errors_fp (stderr);
            SSL_free (ssl);
            close (sockfds[i]);
            exit (EXIT_FAILURE);
        }

        agent_data[i].ssl = ssl;
        printf ("Connecté en TLS à %s (%s) avec succès !\n", agent_data[i].name,
        argv[i + 1]);
    }
}

void cleanup_connections () {
    printf ("Fermeture des connexions....");

    for (int i = 0; i < NUM_AGENTS; ++i) {
        SSL_shutdown (agent_data[i].ssl);
        SSL_free (agent_data[i].ssl);
        pthread_mutex_destroy (&agent_data[i].mutex);
    }
    pthread_mutex_destroy (&global_print_mutex);
    EVP_cleanup ();
}
