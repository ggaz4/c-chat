#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <signal.h>
#include <stdbool.h>
#include <fcntl.h>

// our libraries
#include "network.h"
#include "logging.h"



#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     29000
#define BUFLEN          2048
#define REGISTER_BYTE   'R'
#define CONNECT_BYTE    'C'
#define LISTEN_BYTE     'L'

//volatile sig_atomic_t sigint_received = 0;
volatile sig_atomic_t sigint_received = 0;

void sigint_handler(int s) {
	log_info("[server] SIGINT handler called");
	sigint_received = 1;
}

typedef struct RegisteredUser {
	u_int32_t id;
	char *username;
	char *ip_addr;
	int port;
	char operation;
	struct RegisteredUser *next;
} RegisteredUser;
RegisteredUser *users_list_head = NULL;

uint32_t uint32_random(void) {
	static uint32_t Z;
	if (Z & 1) {
		Z = (Z >> 1);
	} else {
		Z = (Z >> 1) ^ 0x7FFFF159;
	}
	return Z;
}

RegisteredUser *create_registered_user() {
	RegisteredUser *temp = (RegisteredUser *) malloc(sizeof(struct RegisteredUser));
	temp->id = -1;
	temp->username = NULL;
	temp->ip_addr = NULL;
	temp->port = -1;
	temp->operation = '-';
	temp->next = NULL;
	return temp;
}

RegisteredUser *add_registered_user(RegisteredUser **head, char *username) {
	RegisteredUser *temp;
	RegisteredUser *p;

	temp = create_registered_user();
	temp->id = uint32_random();
	temp->username = username;
	if (*head == NULL) {
		*head = temp;
	} else {
		p = *head;
		while (p->next != NULL) {
			p = p->next;
		}
		p->next = temp;
	}
	return temp;
}

RegisteredUser *search_registered_user(RegisteredUser *head, const char *username) {
	RegisteredUser *p = head;
	if (p == NULL) {
		return NULL;
	}

	while (p->next != NULL) {
		if (p->username == username) {
			return p;
		}
		p = p->next;
	}
	return NULL;
}

void free_registered_users_list(RegisteredUser *head) {
	RegisteredUser *p = head;
	while (p) {
		struct RegisteredUser *tmp = p;
		p = p->next;
		free(tmp);
	}
}

void print_registered_user(RegisteredUser *user) {
	log_info("id: %du\nusername: %s\noperation: %c\nIP: %s\nport: %d", user->id, user->username, user->operation, user->ip_addr, user->port);
}

void usage(void) {
	const char *message = "\tserver [-p port]\n"
	                      "\tserver -h\n";
	const char *options = "\t-p port\t\tServer's port\n"
	                      "\t-a     \t\tListen to ANY address not just localhost\n"
	                      "\t-h     \t\tThis help message\n";
	log_usage(message, options);
	exit(EXIT_SUCCESS);
}

void print_cli_variables() {

}

void received_bytes_increase_and_report(const size_t *rxb, size_t *t_rxb) {
	*t_rxb += *rxb;
	log_info("[server] received bytes from client: %zu", *rxb);
	log_info("[server] Total received bytes from client so far: %zu", *t_rxb);
}

void transmitted_bytes_increase_and_report(const size_t *txb, size_t *t_txb) {
	*t_txb += *txb;
	log_info("[server] Transmitted bytes to client: %zu", *txb);
	log_info("[server] Total transmitted bytes to client so far: %zu", *t_txb);
}

int main(int argc, char const *argv[]) {
	/* socket variables */
	int server_fd = -1;                 /* listen file descriptor   */
	int connection_fd = -1;             /* conn file descriptor     */
	int server_port = SERVER_PORT;      /* server port		        */
	const char *server_ip = SERVER_IP;  /* server IP		        */
	in_addr_t server_in_addr = INADDR_LOOPBACK;
	int optval = 1;                     /* socket options	        */
	struct sockaddr_in server_addr;     /* server socket address    */
	int server_addr_len;                /* server address length    */
	struct sockaddr_in client_addr;     /* client socket address    */
	int client_addr_len;                /* client address length    */
	size_t rxb = 0;                     /* received bytes	        */
	size_t t_rxb = 0;                   /* total received bytes     */
	size_t txb = 0;                     /* transmitted bytes	    */
	size_t t_txb = 0;                   /* total transmitted bytes  */
	char plaintext[BUFLEN];             /* plaintext buffer	        */
	int plaintext_len = 0;              /* plaintext size	        */

	/* command line variables */
	int opt = 0;                       /* cmd options		        */
	int any_addr = 0;                  /* listen to any address    */


	/* general purpose variables */
	char *ip_address_buffer = NULL;         /* address str representation */
	unsigned char buffer[INET_ADDRSTRLEN];
	char *tmp;                              /* temp pointer for conventions */
	int i;                                  /* temp int counter             */
	int err;                                /* errors		                */


	/* initialize */
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	memset(&client_addr, 0, sizeof(struct sockaddr_in));

	/* get cmd options */
	while ((opt = getopt(argc, (char *const *) argv, "p:**ah")) != -1) {
		switch (opt) {
			case 'p':
				server_port = (int) strtol(optarg, &tmp, 10);
				break;
			case 'a':
				any_addr = 1;
				server_ip = "0.0.0.0";
				server_in_addr = INADDR_ANY;
				break;
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			default:
				break;
		}
	}

	// socket init
	if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
		log_with_errno("Socket call failed");
		exit(EXIT_FAILURE);
	}

	//make socket non blocking to handle SIGINT
	fcntl(server_fd, F_SETFL, O_NONBLOCK);

	server_addr.sin_port = htons(server_port);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(server_in_addr);

	// set socket options like "ERROR on binding: Address already in use"
	if ((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(optval))) < 0) {
		close(server_fd);
		log_with_errno("[server] Socket setsockopt failed");
		exit(EXIT_FAILURE);
	}

	//bind the socket
	if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
		close(server_fd);
		log_with_errno("[server] Socket bind failed");
		exit(EXIT_FAILURE);
	}



	//listen for connections on socket
	if (listen(server_fd, 2)) {
		close(server_fd);
		log_with_errno("[server] Socket listen failed");
		exit(EXIT_FAILURE);
	}
	log_info("[server] Awaiting for client connections on IP: %s", inet_ntoa(server_addr.sin_addr));

	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigint_handler);
	signal(SIGKILL, sigint_handler);

	while (!sigint_received) {
		if ((connection_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len)) == -1) {
			if (errno == EAGAIN | errno == EWOULDBLOCK) {
				//log_info("[server] EWOULDBLOCK was raised");
				continue;
			}
			close(server_fd);
			log_with_errno("[server] Socket accept failed");
			continue;
		}
		log_info("[server] client connected with ip address: %s and port: %d", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);

		/* STAGE1: Receive the initial message (REGISTER USER) */
		log_info("[server] Waiting for client to send init message");
		if ((rxb = (size_t) recv(connection_fd, &plaintext, sizeof(plaintext), 0)) == -1) {
			close(connection_fd);
			log_with_errno("[server] Socket error receiving initial message");
		}
		if (rxb == 0) {
			log_info("[server] connection terminated before sending init message");
			close(connection_fd);
			continue;
		}
		received_bytes_increase_and_report(&rxb, &t_rxb);

		log_info("[server] Initial message from client: '%s'", plaintext);

		if (plaintext[0] != REGISTER_BYTE) {
			log_info("[server] Wrong initial byte %c --> should be %c", plaintext[0], REGISTER_BYTE);
			log_info("[server] Closing connection");
			close(connection_fd);
			break;
		}

		char username[rxb];
		strncpy(username, plaintext + 1, rxb - 1);
		log_info("[server] Username sent from client: %s", username);

		// check if username exists, otherwise add it to the list
		RegisteredUser *user = search_registered_user(users_list_head, username);
		if (user != NULL) {
			log_info("[server] User '%s' already registered", user->username);
			log_info("[server] Closing connection");
			close(connection_fd);
		}
		user = add_registered_user(&users_list_head, username);

		log_info("[server] Successfully added user %s to the list", username);
		print_registered_user(user);

		// send reply to client with status_code: 200 OK
		memset(&plaintext, 0, sizeof(plaintext));
		strcpy(plaintext, "200OK");
		log_info("[server] sending response to client: %s", plaintext);
		if ((txb = send(connection_fd, &plaintext, strlen((const char *) &plaintext) + 1, 0)) == -1) {
			close(server_fd);
			close(connection_fd);
			log_with_errno("[server] sending message to client failed.");
		}
		transmitted_bytes_increase_and_report(&txb, &t_txb);


		// STAGE2: get operation from client
		log_info("[server] Waiting for client to send operation message");
		if ((rxb = (size_t) recv(connection_fd, &plaintext, sizeof(plaintext), 0)) == -1) {
			close(connection_fd);
			log_with_errno("[server] Socket error receiving operation message");
		}
		if (rxb == 0) {
			log_info("[server] connection terminated before sending operation message");
			close(connection_fd);
			continue;
		}
		received_bytes_increase_and_report(&rxb, &t_rxb);

		log_info("[server] Operation message from client: '%s'", plaintext);

		switch (plaintext[0]) {
			case CONNECT_BYTE:
				log_info("[server] Client wants to connect (chat) with someone at IP: and PORT: ");
				break;
			case LISTEN_BYTE:
				i = 2;
				int j;
				char c;
				char listen_ip[INET_ADDRSTRLEN];
				char listen_port_tmp[6];

				c = plaintext[i];
				j = 0;
				while ((c = plaintext[i]) != ' ') {
					listen_ip[j] = c;
					i++;
					j++;
				}
				listen_ip[j] = '\0';

				i++;
				j = 0;
				while ((c = plaintext[i]) != '\0') {
					listen_port_tmp[j] = c;
					i++;
					j++;
				}
				listen_port_tmp[j] = '\0';

				int listen_port = (int) strtol(listen_port_tmp, NULL, 10);
				log_info("[server] Client wants to listen for someone to (chat) with on IP: %s and PORT: %d", listen_ip, listen_port);
				user->operation = LISTEN_BYTE;
				user->ip_addr = listen_ip;
				user->port = listen_port;
				print_registered_user(user);
				break;
			default:
				log_info("[server] Wrong initial byte %c --> should be %c or %c", plaintext[0], CONNECT_BYTE, LISTEN_BYTE);
				log_info("[server] Closing connection");
				close(connection_fd);
				break;
		}
		close(connection_fd);
	}

	// cleanup
	log_info("[server] cleanup..");
	free_registered_users_list(users_list_head);
	log_info("[server] freed registered users list");
	close(server_fd);
	log_info("[server] closed server socket");
	log_info("[server] exiting");
	exit(EXIT_SUCCESS);
}