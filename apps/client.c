#define _GNU_SOURCE

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <fcntl.h>

#include "logging.h"
#include "network.h"



int LOG_LEVEL = INFO_LEVEL; // must do this before any log_*() call

#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     29000
#define BUFLEN          2048

volatile sig_atomic_t sigint_received = 0;

void sigint_handler(int s) {
	log_info("[client] SIGINT handler called");
	sigint_received = 1;
}

enum mode {
	LISTEN, CONNECT, UNKNOWN
};
typedef enum mode mode;

enum mode map_str_to_mode(const char *name) {
	int i;
	char name_to_lower[strlen(name) + 1];
	for (i = 0; i < strlen(name); i++) {
		name_to_lower[i] = name[i];
	}
	name_to_lower[i] = '\0';

	if (strcmp(name_to_lower, "listen") == 0) {
		return LISTEN;
	}

	if (strcmp(name_to_lower, "connect") == 0) {
		return CONNECT;
	}

	return UNKNOWN;
}

const char *map_mode_to_str(enum mode mode) {
	switch (mode) {
		case LISTEN:
			return "listen";
		case CONNECT:
			return "connect";
		default:
			return "unknown";
	}

}

void usage(void) {
	const char *message = "\tclient -i IP -p port -m message\n"
	                      "\tclient -h\n";
	const char *options =
			"\t-i  IP           \t\tClient's IP address (IPv4 xxx.xxx.xxx.xxx OR IPv6 2001:0db8:85a3:0000:0000:8a2e:0370:7334) [will be used only in 'listen' mode]\n"
			"\t-p  port         \t\tClient's port [will be used only in 'listen' mode]\n"
			"\t-m  mode         \t\tMode in which the client will be run available modes: [listen, connect]\n"
			"\t-u  username     \t\tUsername that will be registered to the server [your username] \n"
			"\t-c  client name  \t\tClient's username [will be used only in 'connect' mode]\n"
			"\t-h               \t\tThis help message\n";
	log_usage(message, options);
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
	/* socket variables */
	int client_fd = -1;                             /* listen file descriptor   */
	int connection_fd = -1;                         /* conn file descriptor     */
	int server_port = SERVER_PORT;                  /* server port		        */
	const char *server_ip = SERVER_IP;              /* server IP		        */
	int optval = 1;                                 /* socket options	        */
	struct sockaddr_in server_addr;                 /* server socket address    */
	int server_addr_len = -1;                       /* server address length    */
	struct sockaddr_in client_addr;                 /* client socket address    */
	int client_addr_len = -1;                       /* client address length    */
	size_t rxb = 0;                                 /* received bytes	        */
	size_t t_rxb = 0;                               /* total received bytes     */
	size_t txb = 0;                                 /* transmitted bytes	    */
	size_t t_txb = 0;                               /* total transmitted bytes  */
	char plaintext[BUFLEN];                         /* plaintext buffer	        */
	int plaintext_len = 0;                          /* plaintext size	        */
	char init_byte;
	struct sockaddr_in chat_addr;
	socklen_t chat_addr_len;



	/* general purpose variables */
	char *ip_address_buffer = NULL;         /* address str representation */
	unsigned char buffer[INET_ADDRSTRLEN];
	char *tmp;                              /* temp pointer for conventions */
	int i;                                  /* temp int counter             */
	int err;                                /* errors		                */


	/* initialize */



	/* command line variables */
	int opt = 0;

	/* getopt_long stores the option index here. */
	int opt_index = 0;
	int help_flag = 0;
	mode mode;
	char *username = NULL;
	char *client_username = NULL;
	char *listening_ip = NULL;
	int listening_port = -1;
	struct option longopts[] = {{"mode",            required_argument, NULL, 'm'},
	                            {"username",        required_argument, NULL, 'u'},
	                            {"ip",              required_argument, NULL, 'i'},
	                            {"port",            required_argument, NULL, 'p'},
	                            {"client-username", required_argument, NULL, 'c'},
	                            {"help",            no_argument, &help_flag, 1},
	                            {0}};
	while (1) {
		opt = getopt_long(argc, argv, "muipc:h::", longopts, &opt_index);
		if (opt == -1) {
			/* a return value of -1 indicates that there are no more options */
			break;
		}
		switch (opt) {
			case 'h':
				/* the help_flag and value are specified in the longopts table,
				 * which means that when the --help option is specified (in its long
				 * form), the help_flag variable will be automatically set.
				 * however, the parser for short-form options does not support the
				 * automatic setting of flags, so we still need this code to set the
				 * help_flag manually when the -h option is specified.
				 */
				help_flag = 1;
				break;
			case 'm':
				mode = map_str_to_mode(strdup(optarg));
				if (mode == UNKNOWN) {
					log_info("[client] Unknown mode given '%s'", optarg);
					exit(EXIT_FAILURE);
				}
				break;
			case 'u':
				username = strdup(optarg);
				if (strlen(username) > 256) {
					log_info("[client] Username given '%s' cannot exceed 256 characters", optarg);
					free(username);
					exit(EXIT_FAILURE);
				}
				break;
			case 'i':
				listening_ip = strdup(optarg);
				break;
			case 'p':
				listening_port = (int) strtol(optarg, NULL, 10);
				if (listening_port < 49152 || listening_port > 65535) {
					log_info("[client] Port given '%d' is not in range 49152 - 65535", listening_port);
					exit(EXIT_FAILURE);
				}
				break;
			case 'c':
				client_username = strdup(optarg);
				if (strlen(client_username) > 256) {
					log_info("[client] Username given '%s' cannot exceed 256 characters", optarg);
					free(username);
					exit(EXIT_FAILURE);
				}
				break;
			case '?':
				/* a return value of '?' indicates that an option was malformed.
				 * this could mean that an unrecognized option was given, or that an
				 * option which requires an argument did not include an argument.
				 */
				usage();
				exit(EXIT_FAILURE);
			default:
				break;
		}
	}

	/* socket init */
	if ((client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
		log_with_errno("[client] socket call failed");
		exit(EXIT_FAILURE);
	}

	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	server_addr.sin_port = htons(server_port);
	server_addr.sin_family = AF_INET;
	inet_aton(server_ip, &server_addr.sin_addr);
	server_addr_len = sizeof(server_addr);

	if ((connect(client_fd, (struct sockaddr *) &server_addr, server_addr_len)) == -1) {
		log_with_errno("[client] socket connect failed");
		close(client_fd);
		exit(EXIT_FAILURE);
	}
	log_info("[client] connected to server at '%s%d'", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

	/* STAGE1: Show the initial text */
	init_byte = REGISTER_BYTE;
	memset(&plaintext, 0, sizeof(plaintext));
	plaintext_len = sprintf(plaintext, "%c%s", init_byte, username);
	log_debug("[client] sending initial message to server '%s'", plaintext);
	if ((txb = (size_t) send(client_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
		log_with_errno("[client] socket sending initial message to server failed");
		close(client_fd);
		exit(EXIT_FAILURE);
	}
	transmitted_bytes_increase_and_report(&txb, &t_txb, "client", 1);

	memset(&plaintext, 0, sizeof(plaintext));
	if ((rxb = (size_t) recv(client_fd, plaintext, sizeof(plaintext), 0)) == -1) {
		log_with_errno("[client] socket error receiving initial message response from server");
		close(client_fd);
		exit(EXIT_FAILURE);
	}
	if (rxb == 0) {
		log_error("[client] connection terminated before receiving init message response from server");
		close(client_fd);
		exit(EXIT_FAILURE);
	}
	received_bytes_increase_and_report(&rxb, &t_rxb, "client", 1);

	log_debug("[client] initial message response from server '%s'", plaintext);

	int status_code = extract_status_code(plaintext);
	if (status_code != 200) {
		log_debug("[client] an error has occurred with status code: %d", status_code);
		if (status_code == 409) {
			log_error("[client] 409 Conflict: user '%s' already registered with the server", username);
		} else {
			log_error("[client] %d: unknown error code", status_code);
		}
		log_error("[client] closing connection");
		close(client_fd);
		exit(EXIT_FAILURE);
	}

	log_debug("[client] server responded with status code: %d", status_code);
	log_info("[client] User '%s' successfully registered to the server", username);

	// STAGE2: Send operation message to the server
	switch (mode) {
		case LISTEN:
			//region LISTEN
			init_byte = LISTEN_BYTE;
			memset(&client_addr, 0, sizeof(struct sockaddr_in));
			if (listening_ip == NULL) {
				log_info("[client] No listening IP specified. Resolving back to '127.0.0.1'");
				listening_ip = "127.0.0.1";
			}
			if (listening_port == -1) {
				log_info("[client] No port specified. Generating a random port from the valid dynamic range 49152-65535");
				srand(time(0));
				listening_port = rand() % ((65535 - 49152 + 1)) + 49152;
			}

			client_addr.sin_family = AF_INET;
			client_addr.sin_port = htons(listening_port);
			inet_aton(listening_ip, &client_addr.sin_addr);
			client_addr_len = sizeof(client_addr);

			log_debug("[client] listening mode at '%s:%d'", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

			memset(&plaintext, 0, sizeof(plaintext));
			sprintf((char *) &plaintext, "%c %s %d", init_byte, inet_ntoa(client_addr.sin_addr), listening_port);
			log_debug("[client] sending operation message to server: %s", plaintext);
			if ((txb = send(client_fd, &plaintext, strlen(plaintext) + 1, 0)) == -1) {
				log_with_errno("[client] Socket sending operation message to server failed");
				close(client_fd);
				exit(EXIT_FAILURE);
			}
			transmitted_bytes_increase_and_report(&txb, &t_txb, "client", 1);

			memset(&plaintext, 0, sizeof(plaintext));
			if ((rxb = (size_t) recv(client_fd, &plaintext, sizeof(plaintext), 0)) == -1) {
				close(client_fd);
				log_with_errno("[client] Socket error receiving operation message response");
				exit(EXIT_FAILURE);
			}
			if (rxb == 0) {
				log_error("[client] connection terminated before receiving operation message response from server");
				close(client_fd);
				exit(EXIT_FAILURE);
			}
			received_bytes_increase_and_report(&rxb, &t_rxb, "client", 1);

			log_debug("[client] operation message response from server: '%s'", plaintext);

			status_code = extract_status_code(plaintext);
			log_debug("[client] server responded with status code: %d", status_code);

			//close the connection with server
			close(client_fd);

			// and now we wait for connections (like a second server)
			if ((client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
				log_with_errno("[client] socket call failed");
				exit(EXIT_FAILURE);
			}

			//make socket non blocking to handle SIGINT
			fcntl(client_fd, F_SETFL, O_NONBLOCK);

			// set socket options like "ERROR on binding: Address already in use"
			if ((setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(optval))) < 0) {
				log_with_errno("[client] socket setsockopt failed");
				close(client_fd);
				exit(EXIT_FAILURE);
			}

			//bind the socket
			if (bind(client_fd, (struct sockaddr *) &client_addr, sizeof(client_addr)) == -1) {
				log_with_errno("[client] socket bind failed");
				close(client_fd);
				exit(EXIT_FAILURE);
			}

			//listen for connections on socket
			if (listen(client_fd, 5)) {
				log_with_errno("[client] socket listen failed");
				close(client_fd);
				exit(EXIT_FAILURE);
			}

			log_info("[client] Awaiting for client connections on '%s:%d'", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

			signal(SIGINT, sigint_handler);

			while (!sigint_received) {
				memset(&chat_addr, 0, sizeof(struct sockaddr_in));

				if ((connection_fd = accept4(client_fd, (struct sockaddr *) &chat_addr, &chat_addr_len, 0)) == -1) {
					//make socket non-blocking to handle SIGINT only for accept()
					if (errno == EAGAIN | errno == EWOULDBLOCK) { continue; }
					log_with_errno("[client] socket accept failed");
					close(client_fd);
					exit(EXIT_FAILURE);
				}

				// disable nonblock flag so, it won't crash the program later
				fcntl(client_fd, F_SETFL, ~O_NONBLOCK);


				log_info("[client] a user connected for chat from '%s:%d'", inet_ntoa(chat_addr.sin_addr), ntohs(chat_addr.sin_port));

				// before chat receive the username to make it more beautiful
				memset(plaintext, 0, sizeof(plaintext));
				if ((rxb = (size_t) recv(connection_fd, plaintext, sizeof(plaintext), 0)) == -1) {
					log_with_errno("[client] socket error receiving message");
					close(connection_fd);
					break;
				}
				if (rxb == 0) {
					log_info("[client] connection terminated");
					close(connection_fd);
					break;
				}
				received_bytes_increase_and_report(&rxb, &t_rxb, "client", 1);

				client_username = (char *) malloc(rxb + 1 * sizeof(char));
				strcpy(client_username, plaintext);
				log_debug("[client] username: '%s'", client_username);

				while (1) {

					memset(plaintext, 0, sizeof(plaintext));
					if ((rxb = (size_t) recv(connection_fd, plaintext, sizeof(plaintext), 0)) == -1) {
						log_with_errno("[client] socket error receiving message");
						close(connection_fd);
						close(client_fd);
						exit(EXIT_FAILURE);
					}
					if (rxb == 0) {
						log_info("[client] connection terminated");
						close(connection_fd);
						break;
					}
					received_bytes_increase_and_report(&rxb, &t_rxb, "client", 1);

					printf("[%s] %s\n", client_username, plaintext);
					fflush(stdout);

					// send message back as is
					log_debug("[client] sending message back to the user: '%s'", plaintext);
					if ((txb = send(connection_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
						log_with_errno("[client] socket error sending message back to the user '%s'", client_username);
						close(connection_fd);
						exit(EXIT_FAILURE);
					}
					transmitted_bytes_increase_and_report(&txb, &t_txb, "client", 1);

					printf("[%s] %s\n", username, plaintext);
					fflush(stdout);

				}

				free(client_username);

			}

			// reconnect to the server and tell him that you are not listening anymore for connections

			break;
			//endregion
		case CONNECT:
			//region CONNECT
			init_byte = CONNECT_BYTE;

			if (client_username == NULL) {
				log_error("[client] no client username provided. No one to chat with.");
				log_error("[client] closing connection");
				close(client_fd);
				exit(EXIT_FAILURE);
			}

			log_debug("[client] connection mode with user '%s'", client_username);

			// send operation message to server
			memset(&plaintext, 0, sizeof(plaintext));
			sprintf((char *) &plaintext, "%c %s", init_byte, client_username);
			log_debug("[client] sending operation message to server: %s", plaintext);
			if ((txb = send(client_fd, &plaintext, strlen(plaintext) + 1, 0)) == -1) {
				log_with_errno("[client] socket sending operation message to server failed");
				close(client_fd);
				exit(EXIT_FAILURE);
			}
			transmitted_bytes_increase_and_report(&txb, &t_txb, "client", 1);

			// receive reply from server
			memset(&plaintext, 0, sizeof(plaintext));
			if ((rxb = (size_t) recv(client_fd, plaintext, sizeof(plaintext), 0)) == -1) {
				log_with_errno("[client] socket error receiving operation message response from server");
				close(client_fd);
				exit(EXIT_FAILURE);
			}
			if (rxb == 0) {
				log_error("[client] connection terminated before receiving operation message response from server");
				close(client_fd);
				exit(EXIT_FAILURE);
			}
			received_bytes_increase_and_report(&rxb, &t_rxb, "client", 1);

			log_debug("[client] operation message response from server: '%s'", plaintext);

			status_code = extract_status_code(plaintext);
			if (status_code != 200) {
				if (status_code == 409) {
					log_error("[client] 404 Not Found: user '%s' does not exist in the server", client_username);
				} else {
					log_error("[client] %d: unknown error code", status_code);
				}
				log_error("[client] closing connection");
				close(client_fd);
				exit(EXIT_FAILURE);
			}

			log_debug("[client] server responded with status code: %d", status_code);

			// receive 2nd reply from server
//			memset(&plaintext, 0, sizeof(plaintext));
//			if ((rxb = (size_t) recv(client_fd, plaintext, sizeof(plaintext), 0 )) == -1) {
//				log_with_errno("[client] socket error receiving second operation message response");
//				close(client_fd);
//				exit(EXIT_FAILURE);
//			}
//			if (rxb == 0) {
//				log_error("[client] connection terminated before receiving second operation message response from server");
//				close(client_fd);
//				exit(EXIT_FAILURE);
//			}
//			received_bytes_increase_and_report(&rxb, &t_rxb, "client", 1);
//
//			log_debug("[client] second operation message response from server: '%s'", plaintext);

			//tokenize the reply and populate client_addr
			memset(&client_addr, 0, sizeof(struct sockaddr_in));
			client_addr.sin_family = AF_INET;
			i = 0;
			char *token = strtok(&plaintext[6], " ");
			int tmp_port;
			while (token) {
				if (i == 0) {
					inet_aton(token, &client_addr.sin_addr);
				} else if (i == 1) {
					tmp_port = (int) strtol(token, NULL, 10);
					client_addr.sin_port = htons(tmp_port);
				}
				token = strtok(NULL, " ");
				i++;
			}
			client_addr_len = sizeof(client_addr);


			log_info(
					"[client] User '%s' found. Attempting connection at '%s:%d'",
					client_username,
					inet_ntoa(client_addr.sin_addr),
					ntohs(client_addr.sin_port));

			// close connection with the server
			close(client_fd);

			// connect to the user for chat
			if ((client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
				log_with_errno("[client] socket call failed");
				exit(EXIT_FAILURE);
			}

			if ((connect(client_fd, (struct sockaddr *) &client_addr, client_addr_len)) == -1) {
				log_with_errno("[client] socket connect failed");
				close(client_fd);
				exit(EXIT_FAILURE);
			}
			log_info("[client] connected with user '%s' for chat at '%s:%d'", client_username, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

			memset(plaintext, 0, sizeof(plaintext));
			sprintf(plaintext, "%s", username);
			log_debug("[client] sending username '%s' to '%s' for recognition", username, client_username);
			if ((txb = (size_t) send(client_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
				close(client_fd);
				log_with_errno("[client] socket error sending username to '%s'", client_username);
				exit(EXIT_FAILURE);
			}
			transmitted_bytes_increase_and_report(&txb, &t_txb, "client", 1);

			while (1) {
				printf("[%s] ", username);
				fflush(stdout);
				memset(plaintext, 0, sizeof(plaintext));
				fgets(plaintext, sizeof(plaintext), stdin);

				// strip newline from message
				plaintext[strcspn(plaintext, "\r\n")] = 0;

				if (plaintext[1] == '\0' && (plaintext[0] == 'q' || plaintext[0] == 'Q')) {
					log_info("[client] terminating chat connection with %s", client_username);
					close(client_fd);
					break;
				}

				log_debug("[client] sending message '%s' to user %s", plaintext, client_username);
				if ((txb = (size_t) send(client_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
					log_with_errno("[client] socket error sending message to user '%s'", client_username);
					close(client_fd);
					exit(EXIT_FAILURE);
				}
				transmitted_bytes_increase_and_report(&txb, &t_txb, "client", 1);

				memset(plaintext, 0, sizeof(plaintext));
				if ((rxb = (size_t) recv(client_fd, plaintext, sizeof(plaintext), 0)) == -1) {
					log_with_errno("[client] socket error receiving message from user '%s'", client_username);
					close(client_fd);
					exit(EXIT_FAILURE);
				}
				if (rxb == 0) {
					log_error("[client] connection terminated unexpectedly");
					close(client_fd);
					exit(EXIT_FAILURE);
				}
				received_bytes_increase_and_report(&rxb, &t_rxb, "client", 1);

				printf("[%s] %s\n", client_username, plaintext);
				fflush(stdout);
			}

			// unregister from the server
			if ((client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
				log_with_errno("[client] socket call failed");
				exit(EXIT_FAILURE);
			}

			memset(&server_addr, 0, sizeof(struct sockaddr_in));
			server_addr.sin_port = htons(server_port);
			server_addr.sin_family = AF_INET;
			inet_aton(server_ip, &server_addr.sin_addr);
			server_addr_len = sizeof(server_addr);

			if ((connect(client_fd, (struct sockaddr *) &server_addr, server_addr_len)) == -1) {
				log_with_errno("[client] socket connect failed");
				close(client_fd);
				exit(EXIT_FAILURE);
			}
			log_info("[client] connected to server at '%s:%d'", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

			init_byte = UNREGISTER_BYTE;
			memset(&plaintext, 0, sizeof(plaintext));
			plaintext_len = sprintf(plaintext, "%c%s", init_byte, username);
			log_debug("[client] sending UNREGISTER message to server '%s'", plaintext);
			if ((txb = (size_t) send(client_fd, plaintext, strlen(plaintext) + 1, 0)) == -1) {
				log_with_errno("[client] socket sending UNREGISTER message to server failed");
				close(client_fd);
				exit(EXIT_FAILURE);
			}
			transmitted_bytes_increase_and_report(&txb, &t_txb, "client", 1);

			memset(plaintext, 0, sizeof(plaintext));
			if ((rxb = (size_t) recv(client_fd, plaintext, sizeof(plaintext), 0)) == -1) {
				log_with_errno("[client] socket error receiving unregister message response from server");
				close(client_fd);
				exit(EXIT_FAILURE);
			}
			if (rxb == 0) {
				log_error("[client] connection terminated unexpectedly");
				close(client_fd);
				break;
			}
			received_bytes_increase_and_report(&rxb, &t_rxb, "client", 1);

			close(client_fd);

			break;
			//endregion
		default:
			log_info("[client] wrong operation %d", map_mode_to_str(mode));
			close(client_fd);
			break;
	}

	return 0;
}

/* EOF */
