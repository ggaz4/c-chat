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



#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     29000

int LOG_LEVEL = INFO_LEVEL; // must do this before any log_*() call


volatile sig_atomic_t sigint_received = 0;
RegisteredUser *users_list_head = NULL;

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
	//region variables declaration
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
	socklen_t client_addr_len;          /* client address length    */
	size_t rxb = 0;                     /* received bytes	        */
	size_t t_rxb = 0;                   /* total received bytes     */
	size_t txb = 0;                     /* transmitted bytes	    */
	size_t t_txb = 0;                   /* total transmitted bytes  */
	char plaintext[BUFLEN];             /* plaintext buffer	        */
	int plaintext_len = 0;              /* plaintext size	        */

	/* this array size is calculated by counting the characters of this example string
	 * that represents a socket address  '127.0.0.1:15000'
	 *
	 * 127.0.0.1    --> INET_ADDRSTRLEN     --> 16 bytes
	 * :            --> semicolon character --> 1 byte
	 * 15000        --> port string         --> max 5 bytes
	 * */
	char server_addr_str[INET_ADDRSTRLEN + 1 + 6];
	char client_addr_str[INET_ADDRSTRLEN + 1 + 6];


	/* command line variables */
	int opt = 0;                       /* cmd options		        */
	int any_addr = 0;                  /* listen to any address    */


	/* general purpose variables */
	char *ip_address_buffer = NULL;         /* address str representation */
	unsigned char buffer[INET_ADDRSTRLEN];
	char *tmp;                              /* temp pointer for conventions */
	int i;                                  /* temp int counter             */
	int err;                                /* errors		                */

	//endregion

	//region get command line flag options
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
	//endregion

	// region initialize server (listening) socket
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

	//endregion

	sprintf(server_addr_str, "%s:%d", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
	log_info("[server] awaiting for client connections on '%s'", server_addr_str);

	signal(SIGINT, sigint_handler);


	char first_byte;
	char username[256];
	while (!sigint_received) {
		client_addr_len = sizeof(client_addr);
		memset(&client_addr, 0, sizeof(struct sockaddr_in));
		memset(&first_byte, '\0', sizeof(char));
		memset(username, '\0', sizeof(username));
		memset(client_addr_str, '\0', sizeof(client_addr_str));

		if ((connection_fd = accept4(server_fd, (struct sockaddr *) &client_addr, &client_addr_len, 0)) == -1) {
			//make socket non-blocking to handle SIGINT
			if (errno == EAGAIN | errno == EWOULDBLOCK) { continue; }
			log_with_errno("[server] Socket accept failed");
			continue;
		}

		// disable nonblock flag so, it won't crash the program later
		fcntl(server_fd, F_SETFL, ~O_NONBLOCK);

		sprintf(client_addr_str, "%s:%d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
		log_info("[server] client connected from '%s'", client_addr_str);

		//region receive the initial message from the client (REGISTER or UNREGISTER)
		/* Initial Message from client format: '{char}{username}\0'
		 * {char}       --> can be either 'R' or 'U' (1 byte)
		 * {username}   --> a string with max size 256 bytes (with terminating character '\0'
		 *                  so a max 255 characters string
		 * */
		log_debug("[server] waiting for client to send initial message");
		if ((rxb = (size_t) recv(connection_fd, plaintext, sizeof(plaintext), 0)) == -1) {
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
		log_info("[server] initial message from client: '%s'", plaintext);
		first_byte = plaintext[0];
		if (first_byte != REGISTER_BYTE && first_byte != UNREGISTER_BYTE) {
			log_error("[server] wrong initial byte %c --> should be one of [%c, %c]", first_byte, REGISTER_BYTE, UNREGISTER_BYTE);
			log_error("[server] closing connection");
			close(connection_fd);
			continue;
		}

		/* extract the username of the connected client from the message */
		memset(username, 0, sizeof(username));
		strncpy(username, plaintext + 1, rxb - 1);
		log_debug("[server] username sent from client: %s", username);
		//endregion

		/* check if user with this username exists, otherwise add it to the list */
		RegisteredUser *user = search_registered_user(users_list_head, username);

		// unregister mode
		if (first_byte == UNREGISTER_BYTE) {
			if (user == NULL) {
				// send response back to the client that user was not found
				log_info("[server] user '%s' is not registered to the server", username);
				prepare_status_code((char *) &plaintext, 404, "NOTFOUND");
			} else {
				// send response back to the client that user was deleted successfully
				delete_registered_user(&users_list_head, username);
				log_debug("[server] successfully deleted user '%s' from the list", username);
				prepare_status_code((char *) &plaintext, 200, "OK");
			}

			// and close connection no matter what the reply was
			log_debug("[server] sending response to client: %s", plaintext);
			if ((txb = send(connection_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
				close(connection_fd);
				close(server_fd);
				log_with_errno("[server] sending message to client failed.");
				exit(EXIT_FAILURE);
			}
			transmitted_bytes_increase_and_report(&txb, &t_txb, "server", 1);
			log_info("[server] closing connection with '%s'", client_addr_str);
			close(connection_fd);

			print_all_registered_users(users_list_head);
			continue;
		}

		//region register mode - the program will only continue here if this is a register mode*/
		if (user != NULL) {
			// send response back to the client that user already exists and close connection
			log_info("[server] user '%s' already registered", user->username);
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

		// send response back to the client that user was added successfully
		user = add_registered_user(&users_list_head, username);
		log_debug("[server] successfully added user '%s' to the list", username);
		prepare_status_code((char *) &plaintext, 200, "OK");
		log_debug("[server] sending response to client: %s", plaintext);
		if ((txb = send(connection_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
			close(connection_fd);
			close(server_fd);
			log_with_errno("[server] sending message to client failed.");
			exit(EXIT_FAILURE);
		}
		transmitted_bytes_increase_and_report(&txb, &t_txb, "server", 1);

		// get operation message from client
		log_debug("[server] waiting for client to send operation message");
		memset(plaintext, 0, sizeof(plaintext));
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
		first_byte = plaintext[0];
		if (first_byte != CONNECT_BYTE && first_byte != LISTEN_BYTE) {
			log_error("[server] wrong operation byte %c --> should be one of [%c, %c]", first_byte, CONNECT_BYTE, LISTEN_BYTE);
			log_error("[server] closing connection");
			close(connection_fd);
			continue;
		}

		/* extract the information from the message */
		char *token = NULL;

		// listen mode
		if (first_byte == LISTEN_BYTE) {
			char listen_ip[INET_ADDRSTRLEN];
			int listen_port;

			i = 0;
			token = strtok(&plaintext[2], " ");
			while (token) {
				if (i == 0) {
					strcpy(listen_ip, token);
				} else if (i == 1) {
					listen_port = (int) strtol(token, NULL, 10);
					break;
				}
				token = strtok(NULL, " ");
				i++;
			}

			log_info("[server] user '%s' waits to chat at '%s:%d'", user->username, listen_ip, listen_port);

			user->operation = LISTEN_BYTE;
			user->port = listen_port;
			strcpy(user->ip_addr, listen_ip);

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
		}

		// connect mode
		if (first_byte == CONNECT_BYTE) {
			char connect_with_username[256];
			token = strtok(&plaintext[2], " ");
			strcpy(connect_with_username, token);

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

			// send reply that user exists and append the appropriate IP and PORT of the found user
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
		}

		//endregion

		log_info("[server] closing connection with '%s'", client_addr_str);
		close(connection_fd);

		print_all_registered_users(users_list_head);
	}

	//region cleanup
	log_info("[server] cleanup..");
	free_registered_users_list(users_list_head);
	log_info("[server] freed registered users list");
	close(server_fd);
	log_info("[server] closed server socket");
	log_info("[server] exiting");
	exit(EXIT_SUCCESS);
	//endregion
}