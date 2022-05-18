#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "logging.h"

#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     29000
#define BUFLEN          2048
#define REGISTER_BYTE   'R'
#define LISTEN_BYTE     'L'
#define CONNECT_BYTE    'C'

/*
 * prints the usage message
 */
void usage(void) {
	const char *message = "\tclient -i IP -p port -m message\n"
	                      "\tclient -h\n";
	const char *options =
			"\t-i  IP     \t\tServer's IP address (IPv4 xxx.xxx.xxx.xxx OR IPv6 2001:0db8:85a3:0000:0000:8a2e:0370:7334)\n"
			"\t-p  port   \t\tServer's port\n"
			"\t-m  message\t\tMessage to server\n"
			"\t-f  file   \t\tFile to read message from\n"
			"\t-h         \t\tThis help message\n";
	log_usage(message, options);
	exit(EXIT_SUCCESS);
}
/*
 * simple chat client with RSA-based AES
 * key-exchange for encrypted communication
 */
int main(int argc, char *argv[]) {
	/* socket variables */
	int client_fd = -1;                 /* listen file descriptor   */
	int connection_fd = -1;             /* conn file descriptor     */
	int server_port = SERVER_PORT;      /* server port		        */
	const char *server_ip = SERVER_IP;  /* server IP		        */
	int optval = 1;                     /* socket options	        */
	struct sockaddr_in server_addr;     /* server socket address    */
	int server_addr_len = -1;           /* server address length    */
	struct sockaddr_in client_addr;     /* client socket address    */
	int client_addr_len = -1;           /* client address length    */
	size_t rxb = 0;                     /* received bytes	        */
	size_t t_rxb = 0;                   /* total received bytes     */
	size_t txb = 0;                     /* transmitted bytes	    */
	size_t t_txb = 0;                   /* total transmitted bytes  */
	unsigned char plaintext[BUFLEN];    /* plaintext buffer	        */
	int plaintext_len = 0;              /* plaintext size	        */
	unsigned char init_byte;


	/* command line variables */
	int opt = 0;                       /* cmd options		        */

	/* general purpose variables */
	char *ip_address_buffer = NULL;         /* address str representation */
	unsigned char buffer[INET_ADDRSTRLEN];
	char *tmp;                              /* temp pointer for conventions */
	int i;                                  /* temp int counter             */
	int err;                                /* errors		                */


	/* initialize */
	memset(&server_addr, 0, sizeof(struct sockaddr_in));
	memset(&client_addr, 0, sizeof(struct sockaddr_in));

	/* get options */
//	while ((opt = getopt(argc, argv, "i:m:p:f::h")) != -1) {
//		switch (opt) {
//			case 'i':
//				sip = strdup(optarg);
//				break;
//			case 'm':
//				msg = (unsigned char *) strdup(optarg);
//				break;
//			case 'p':
//				port = atoi(optarg);
//				break;
//			case 'h':
//			default:
//				usage();
//		}
//	}

	/* socket init */
	if ((client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == -1) {
		log_with_errno("[client] Socket call failed");
		exit(EXIT_FAILURE);
	}

	server_addr.sin_port = htons(server_port);
	server_addr.sin_family = AF_INET;
	inet_aton(server_ip, &server_addr.sin_addr);

	server_addr_len = sizeof(server_addr);

	if ((connect(client_fd, (struct sockaddr *) &server_addr, server_addr_len)) == -1) {
		close(client_fd);
		log_with_errno("[client] Socket connect failed");
		exit(EXIT_FAILURE);
	}
	log_info("[client] Connected to server with ip address: %s", inet_ntoa(server_addr.sin_addr));

	/* STAGE1: Show the initial text */
	init_byte = REGISTER_BYTE;
	char *username = "j3di";
	memset(&plaintext, 0, sizeof(plaintext));
	plaintext_len = sprintf((char *) &plaintext, "%c%s", init_byte, username);

	log_info("[client] Sending initial message to server (including null character in size): %s", plaintext);
	if ((txb = (size_t) send(client_fd, &plaintext, strlen((const char *) &plaintext) + 1, 0)) == -1) {
		close(client_fd);
		log_with_errno("[client] Socket sending init message to server failed");
		exit(EXIT_FAILURE);
	}
	log_info("[client] Transmitted bytes to server: %zu", txb);
	t_txb += txb;
	log_info("[client] Total transmitted bytes so far: %zu", t_txb);

	memset(&plaintext, 0, sizeof(plaintext));
	if ((rxb = (size_t) recv(client_fd, &plaintext, sizeof(plaintext), 0)) == -1) {
		close(client_fd);
		log_with_errno("[client] Socket error receiving initial message response");
		exit(EXIT_FAILURE);
	}
	log_info("[client] Received bytes from server: %zu", rxb);
	t_rxb += rxb;
	log_info("[client] Total received bytes so far: %zu", t_rxb);

	log_info("[client] Initial message response from server: '%s'", plaintext);

	memset(&buffer, 0, sizeof(buffer));
	strncpy((char *) &buffer, (const char *) &plaintext, 3);
	int status_code = (int) strtol((const char *) &buffer, NULL, 10);
	log_info("[client] Server responded with status code: %d", status_code);
	if (status_code != 200) {
		log_info("[client] An error has occurred with status code: %d", status_code);
		if (status_code == 409) {
			log_info("[client] 409 Conflict: user already registered with the server");
		} else {
			log_info("[client] Unknown error code");
		}
		log_info("[client] Closing connection");
		close(client_fd);
		exit(EXIT_FAILURE);
	}

	log_info("[client] User '%s' registered successfully to the server", username);

	// STAGE2: Send operation message to the server
	int operation = 16;

	switch (operation) {
		case 16:
			init_byte = LISTEN_BYTE;
			memset(&client_addr, 0, sizeof(struct in_addr));
			const char *client_ip = "127.0.0.1";
			int client_port = rand() % ((65535 - 49152 + 1)) + 49152; // dynamic ports range from 49152 to 65535
			client_addr.sin_port = htons(client_port);
			client_addr.sin_family = AF_INET;
			inet_aton(client_ip, &client_addr.sin_addr);
			client_addr_len = sizeof(client_addr);

			log_info(
					"[client] I want to listen for connections at IP: '%s' and PORT: '%d'",
					inet_ntoa(client_addr.sin_addr),
					client_port
			);
			memset(&buffer, 0, sizeof(buffer));
			memset(&plaintext, 0, sizeof(plaintext));
			sprintf((char *) &plaintext, "%c %s %d", init_byte, inet_ntoa(client_addr.sin_addr), client_port);
			log_info("[client] Sending operation message to server: %s", plaintext);
			if ((txb = send(client_fd, &plaintext, strlen((const char *) &plaintext) + 1, 0)) == -1) {
				close(client_fd);
				log_with_errno("[client] Socket sending operation message to server failed");
				exit(EXIT_FAILURE);
			}
			log_info("[client] Transmitted bytes to server: %zu", txb);
			t_txb += txb;
			log_info("[client] Total transmitted bytes so far: %zu", t_txb);




			break;
		case 32:
			break;
		default:
			log_info("[client] wrong operation %d", operation);
			close(client_fd);
			break;
	}





	/* cleanup */
	close(client_fd);

	return 0;
}

/* EOF */
