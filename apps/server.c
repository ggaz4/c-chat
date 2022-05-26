#define _GNU_SOURCE

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
#include "logging.h"
#include "network.h"
#include "structures.h"



int LOG_LEVEL = DEBUG_LEVEL; // must do this before any log_*() call

RegisteredUser *users_list_head = NULL;


#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     29000
#define BUFLEN          2048


volatile sig_atomic_t sigint_received = 0;

void prepare_status_code(char *buffer, int code, const char *message) {
	memset(buffer, 0, BUFLEN);
	sprintf(buffer, "%d%s", code, message);
}

void sigint_handler(int s) {
	log_info("[server] SIGINT handler called");
	sigint_received = 1;
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
	socklen_t client_addr_len;                /* client address length    */
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

	/* get cmd options */
	while ((opt = getopt(argc, (char *const *) argv, "p::a::h::")) != -1) {
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
		log_with_errno("[server] socket call failed");
		exit(EXIT_FAILURE);
	}

	//make socket non blocking to handle SIGINT
	fcntl(server_fd, F_SETFL, O_NONBLOCK);

	memset(&server_addr, 0, sizeof(struct sockaddr_in));
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
	if (listen(server_fd, 5)) {
		close(server_fd);
		log_with_errno("[server] Socket listen failed");
		exit(EXIT_FAILURE);
	}

	log_info("[server] Awaiting for client connections on '%s:%d'", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

	signal(SIGINT, sigint_handler);

	while (!sigint_received) {
		//SOCK_NONBLOCK --> flag for accept4()
		memset(&client_addr, 0, sizeof(struct sockaddr_in));
		if ((connection_fd = accept4(server_fd, (struct sockaddr *) &client_addr, &client_addr_len, 0)) == -1) {
			//make socket non-blocking to handle SIGINT only for accept()
			if (errno == EAGAIN | errno == EWOULDBLOCK) {continue;}
			log_with_errno("[server] Socket accept failed");
			continue;
		}

		// disable nonblock flag so, it won't crash the program later
		fcntl(server_fd, F_SETFL, ~O_NONBLOCK);


		log_info("[server] client connected from '%s:%d'", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		/* STAGE1: Receive the initial message (REGISTER USER) */
		log_debug("[server] Waiting for client to send init message");
		if ((rxb = (size_t) recv(connection_fd, &plaintext, sizeof(plaintext), 0)) == -1) {
			close(connection_fd);
			log_with_errno("[server] socket error receiving initial message");
			continue;
		}
		if (rxb == 0) {
			log_error("[server] connection terminated before receiving init message");
			close(connection_fd);
			continue;
		}
		received_bytes_increase_and_report(&rxb, &t_rxb, "server", 1);

		log_info("[server] Initial message from client: '%s'", plaintext);

		if (plaintext[0] != REGISTER_BYTE && plaintext[0] != UNREGISTER_BYTE) {
			log_error("[server] wrong initial byte %c --> should be one of [%c, %c]", plaintext[0], REGISTER_BYTE, UNREGISTER_BYTE);
			log_error("[server] closing connection");
			close(connection_fd);
			continue;
		}

		char username[rxb];
		memset(username, 0, rxb);
		strncpy(username, plaintext + 1, rxb - 1);
		log_debug("[server] username sent from client: %s", username);

		// check if username exists, otherwise add it to the list
		RegisteredUser *user = search_registered_user(users_list_head, username);

		// unregister mode
		if (plaintext[0] == UNREGISTER_BYTE) {
			if (user == NULL) {
				log_info("[server] user '%s' is not registered to the server", username);
				// send response back to client that user was not found
				prepare_status_code((char *) &plaintext, 404, "NOTFOUND");
			} else {
				// send reply to client with status_code: 200 OK
				delete_registered_user(&users_list_head, username);
				log_debug("[server] successfully deleted user '%s' from the list", username);
				prepare_status_code((char *) &plaintext, 200, "OK");
			}

			log_debug("[server] sending response to client: %s", plaintext);
			if ((txb = send(connection_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
				close(connection_fd);
				close(server_fd);
				log_with_errno("[server] sending message to client failed.");
				exit(EXIT_FAILURE);
			}
			transmitted_bytes_increase_and_report(&txb, &t_txb, "server", 1);
			log_info("[server] closing connection");
			close(connection_fd);
			continue;
		}


		// register mode
		if (user != NULL) {
			log_info("[server] user '%s' already registered", user->username);

			// send response back to client that user already exists
			prepare_status_code((char *) &plaintext, 409, "CONFLICT");
			log_debug("[server] sending response to client: %s", plaintext);
			if ((txb = send(connection_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
				close(connection_fd);
				close(server_fd);
				log_with_errno("[server] sending message to client failed.");
				exit(EXIT_FAILURE);
			}
			transmitted_bytes_increase_and_report(&txb, &t_txb, "server", 1);

			log_info("[server] closing connection");
			close(connection_fd);
			continue;
		}

		user = add_registered_user(&users_list_head, username);

		log_debug("[server] successfully added user '%s' to the list", username);

		// send reply to client with status_code: 200 OK
		prepare_status_code((char *) &plaintext, 200, "OK");
		log_debug("[server] sending response to client: %s", plaintext);
		if ((txb = send(connection_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
			close(connection_fd);
			close(server_fd);
			log_with_errno("[server] sending message to client failed.");
			exit(EXIT_FAILURE);
		}
		transmitted_bytes_increase_and_report(&txb, &t_txb, "server", 1);


		// STAGE2: get operation from client
		memset(plaintext, 0, sizeof(plaintext));
		log_debug("[server] waiting for client to send operation message");
		if ((rxb = (size_t) recv(connection_fd, plaintext, sizeof(plaintext), 0)) == -1) {
			close(server_fd);
			close(connection_fd);
			log_with_errno("[server] socket error receiving operation message");
			exit(EXIT_FAILURE);
		}
		if (rxb == 0) {
			log_error("[server] connection terminated before receiving operation message");
			close(connection_fd);
			continue;
		}
		received_bytes_increase_and_report(&rxb, &t_rxb, "server", 1);

		log_info("[server] operation message from client: '%s'", plaintext);

		char *token = NULL;
		switch (plaintext[0]) {
			case CONNECT_BYTE:
				i = 0;
				char connect_with_username[256];
				token = strtok(&plaintext[2], " ");
				while (token) {
					if (i == 0) {
						strcpy(connect_with_username, token);
					}
					token = strtok(NULL, " ");
					i++;
				}

				log_info("[server] user '%s' wants to connect (chat) with '%s'", user->username, connect_with_username);

				RegisteredUser *connect_user = search_registered_user(users_list_head, connect_with_username);
				if (connect_user == NULL) {
					log_info("[server] user '%s' does not exist", connect_with_username);

					// delete registered user because it won't connect with anyone and thus is not a valid list entry
					delete_registered_user(&users_list_head, user->username);

					// send reply that user does not exist
					prepare_status_code((char *) &plaintext, 404, "NOTFOUND");
					log_debug("[server] sending response to client: %s", plaintext);
					if ((txb = send(connection_fd, &plaintext, strlen((const char *) &plaintext) + 1, 0)) == -1) {
						close(connection_fd);
						close(server_fd);
						log_with_errno("[server] sending message to client failed.");
						exit(EXIT_FAILURE);
					}
					transmitted_bytes_increase_and_report(&txb, &t_txb, "server", 1);
					close(connection_fd);
					break;
				}

				//update current user's information
				user->operation = CONNECT_BYTE;
				strcpy(user->connected_with, connect_user->username);

				// send reply that user exists and then send a SECOND reply with the appropriate IP and PORT of the user

				//1st
				//prepare_status_code((char *) &plaintext, 200, "OK");
				memset(plaintext, 0, sizeof(plaintext));
				sprintf((char *) &plaintext, "%d%s %s %d", 200, "OK", connect_user->ip_addr, connect_user->port);
				log_debug("[server] sending response to client: %s", plaintext);
				if ((txb = send(connection_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
					close(connection_fd);
					close(server_fd);
					log_with_errno("[server] sending message to client failed.");
					exit(EXIT_FAILURE);
				}
				transmitted_bytes_increase_and_report(&txb, &t_txb, "server", 1);

				//2nd
//				memset(plaintext, 0, sizeof(plaintext));
//				sprintf((char *) &plaintext, "%s %d", connect_user->ip_addr, connect_user->port);
//				log_debug("[client] sending operation message response to client: %s", plaintext);
//				if ((txb = send(connection_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
//					close(connection_fd);
//					close(server_fd);
//					log_with_errno("[client] sending message to client failed");
//					exit(EXIT_FAILURE);
//				}
//				transmitted_bytes_increase_and_report(&txb, &t_txb, "server", 1);

				close(connection_fd);

				break;
			case LISTEN_BYTE:
				i = 0;

				char listen_ip[INET_ADDRSTRLEN];
				int listen_port;

				token = strtok(&plaintext[2], " ");
				while (token) {
					if (i == 0) {
						strcpy(listen_ip, token);
					} else if (i == 1) {
						listen_port = (int) strtol(token, NULL, 10);
					}
					token = strtok(NULL, " ");
					i++;
				}

				log_info("[server] user '%s' waits to chat at '%s:%d'", user->username, listen_ip, listen_port);

				user->operation = LISTEN_BYTE;
				strcpy(user->ip_addr, listen_ip);
				user->port = listen_port;

				// send response back to client
				prepare_status_code((char *) &plaintext, 200, "OK");
				log_debug("[server] sending response to client: %s", plaintext);
				if ((txb = send(connection_fd, &plaintext, strlen(plaintext) + 1, 0)) == -1) {
					close(connection_fd);
					close(server_fd);
					log_with_errno("[server] sending message to client failed.");
					exit(EXIT_FAILURE);
				}
				transmitted_bytes_increase_and_report(&txb, &t_txb, "server", 1);

				close(connection_fd);

				break;
			default:
				log_error("[server] wrong initial byte '%c' --> should be one of [%c, %c]", plaintext[0], CONNECT_BYTE, LISTEN_BYTE);
				log_error("[server] closing connection");
				close(connection_fd);
				break;
		}

		//print_all_registered_users(users_list_head);
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