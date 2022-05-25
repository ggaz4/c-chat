#ifndef C_CHAT_STRUCTURES_H
#define C_CHAT_STRUCTURES_H

#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>



typedef struct RegisteredUser {
	//u_int32_t id;
	char *username;
	char *connected_with;
	char *ip_addr;
	int port;
	char operation;
	struct RegisteredUser *next;
} RegisteredUser;

RegisteredUser *create_registered_user();

RegisteredUser *add_registered_user(RegisteredUser **head, char *username);

RegisteredUser *search_registered_user(RegisteredUser *head, const char *username);

void delete_registered_user(RegisteredUser **head, char *username);

void free_registered_users_list(RegisteredUser *head);

void print_registered_user(RegisteredUser *user);

void print_all_registered_users(RegisteredUser *head);

uint32_t uint32_random(void);

#endif //C_CHAT_STRUCTURES_H
