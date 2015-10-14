#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_PRIME 101

#define TABLE_SIZE 32768 * HASH_PRIME

unsigned int hash_gen(char *str); // don't do this!

typedef struct hash_t hash_t;
struct hash_t {
	char *key;
	char *value;
	hash_t *next;
};

// oh yes, initialise here, it's much faster
hash_t *hash_table[TABLE_SIZE] = { NULL };
 
void hash_table_free(void)
{
	for (int i = 0; i < TABLE_SIZE; i++) {
		hash_t *cursor = hash_table[i];
		while (cursor) {
			free(cursor->key);
			free(cursor->value);
			hash_t *next = cursor->next;
			free(cursor);
			cursor = next;
		}
	}
}

void hash_table_init(void)
{
	return;
	
	// this is so expensive!!!
	for (int i = 0; i < TABLE_SIZE; i++) {
		hash_table[i] = NULL;		
	}
}

void hash_table_insert(char *key, char *value)
{
	unsigned int index = hash_gen(key);

	// this is bogus but so am I!	
	hash_t *cursor = hash_table[index];
	if (cursor == NULL) {
		hash_table[index] = cursor = calloc(1, sizeof(hash_t));
		cursor->key = strdup(key);
		cursor->value = strdup(value);
		cursor->next = NULL;
		return;
		// first entry no collisions!
	}

	// collisisions!
	while (cursor->next) {
		cursor = cursor->next;
	}

	if (cursor->next == NULL) {
		cursor->next = calloc(1, sizeof(hash_t));	
		cursor = cursor->next;
		cursor->key = strdup(key);
		cursor->value = strdup(value);
		cursor->next = NULL;
	}
}

unsigned int hash_gen(char *str)
{
	char *p = str;
	unsigned int res = 0;

	while (*p) {
		res += (unsigned int) *p;
		p++;
	}

	return res % TABLE_SIZE;
}

hash_t * hash_table_search(char *key)
{
	unsigned int idx = hash_gen(key);
	hash_t *tmp = hash_table[idx];
	if (!tmp)
		return NULL;
	
	return (hash_t *) tmp; 
}

int main(void)
{
	char *key = "al";
	char *value = "password";

	hash_table_insert(key, value);
	hash_table_insert("ed", "gowanlea");

	hash_t *found = hash_table_search("al");	
	if (found) {
		printf("%s and %s\n", found->key, found->value);
	}

	found = hash_table_search("ed");
	if (found) {
		printf("%s and %s\n", found->key, found->value);
	} else
		printf("not found!\n");

	hash_table_free();
	
	return EXIT_SUCCESS;
}
