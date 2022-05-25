#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "structures.h"



RegisteredUser *create_registered_user() {
	RegisteredUser *temp = malloc(sizeof(RegisteredUser));
	//temp->id = -1;

	temp->username = malloc(256 * sizeof(char));
	temp->connected_with = malloc(256 * sizeof(char));
	temp->ip_addr = malloc(INET_ADDRSTRLEN * sizeof(char));

	memset(temp->username, '\0', strlen(temp->username));
	memset(temp->connected_with, '\0', strlen(temp->connected_with));
	memset(temp->ip_addr, '\0', strlen(temp->ip_addr));
	temp->port = -1;
	temp->operation = '\0';
	temp->next = NULL;
	return temp;
}

RegisteredUser *add_registered_user(RegisteredUser **head, char *username) {
	RegisteredUser *temp;
	RegisteredUser *p;

	temp = create_registered_user();
	//temp->id = uint32_random();
	strcpy(temp->username, username);
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

	while (p != NULL) {
		if (strcmp(p->username, username) == 0) {
			return p;
		}
		p = p->next;
	}
	return NULL;
}

void delete_registered_user(RegisteredUser **head, char *username) {
	RegisteredUser *p = *head;
	RegisteredUser *prev = NULL;

	// If head node itself holds the key to be deleted
	if (*head != NULL && strcmp((*head)->username, username) == 0) {
		*head = (*head)->next; // Changed head
		free(p); // free old head
		return;
	}

	// Search for the key to be deleted, keep track of the
	// previous node as we need to change 'prev->next'
	while (p != NULL && strcmp(p->username, username) != 0) {
		prev = p;
		p = p->next;
	}

	// If key was not present in linked list
	if (p == NULL) {
		return;
	}

	// Unlink the node from linked list
	prev->next = p->next;

	free(p); // free node from linked list
	p = NULL;
}

void free_registered_users_list(RegisteredUser *head) {
	RegisteredUser *p = head;
	struct RegisteredUser *tmp;
	while (p) {
		tmp = p;
		p = p->next;
		free(tmp);
	}
}

void print_registered_user(RegisteredUser *user) {
	printf("username: '%s', operation: '%c', IP: '%s', port: '%d'", user->username, user->operation, user->ip_addr, user->port);
	fflush(stdout);
}

void print_all_registered_users(RegisteredUser *head) {
	if (head == NULL) {
		return;
	}

	int users_count = 0;
	RegisteredUser *p = head;
	while (p) {
		users_count++;
		print_registered_user(p);
		p = p->next;
	}

	printf("[server] Total registered users: %d", users_count);
	fflush(stdout);
}

uint32_t uint32_random(void) {
	static uint32_t Z;
	if (Z & 1) {
		Z = (Z >> 1);
	} else {
		Z = (Z >> 1) ^ 0x7FFFF159;
	}
	return Z;
}