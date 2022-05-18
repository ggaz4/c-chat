#include <stdio.h>
#include <stdlib.h>
#include "network.h"

char *get_char_pointer_from_malloc(int value){
	char *buffer;
	buffer = malloc(128 * sizeof(char));
	snprintf(buffer, 128, "Library value is %d", value);
	printf("MESSAGE --> %s\n", buffer);
    printf("ADDRESS --> %p\n", (void *) &buffer);
	return buffer;
}

void free_char_pointer(char **pointer){
	free(*pointer);
	*pointer = NULL;
}
