#ifndef __BOGOMEM_H__
#define __BOGOMEM_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>


void Bork(char *fmt, ...)
{
	va_list ap;

	char buf[8192] = { 0 };

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);

	fprintf(stderr, "%s\n", buf);
	exit(EXIT_FAILURE);
} 

void *Malloc(ssize_t len);

#include <errno.h>
#include <limits.h>
#include <string.h>
#define PID_MAX 32768

typedef unsigned int uint32;

typedef struct memobj memobj_t;
struct memobj {
	pid_t pid;
	char *key;
	ssize_t len;
	int count;
	void *data;
	memobj_t *next;	
};

#define TABLE_SIZE 101

memobj_t memory[TABLE_SIZE];

int _GetHash(char *key)
{
	int res = 0;
	char *p = key;
	while (*p) {
		res += *p;
		p++;
	}

	return *p % TABLE_SIZE;
}

void *SetMemObj(pid_t pid, char *data, char *key, ssize_t len)
{
	int hash = _GetHash(key);
	memobj_t *object = &memory[hash];

	while (object->next) {
		if (object->pid == pid && 0 == strcmp(key, object->key))
			return object;
		object = object->next;
	}

	if (object->next == NULL) {
		object->next = calloc(1, sizeof(memobj_t));
		object = object->next;

 		object->pid = pid;
		object->key = strdup(key);
		object->len = len;
		object->data = Malloc(len);
		if (object->data == NULL)
			Bork("Malloc()");
		
		memcpy(object->data, data, len);
		object->next = NULL;
		++object->count;
	
		return object;
	}	
	
	return NULL;
}

char *PrintMemObj(pid_t pid, char *key) 
{
	int hash = _GetHash(key);
	memobj_t *object = &memory[hash];

	int found = 0;
	while (object) {
		if (pid == object->pid && 0 == strcmp(key, object->key)) {
			return (char *) object->data;
			++found; // !!!
		}
		object = object->next;
	}

	if (!found)
		return "Cannot find entry!";
}

void FreeMemObj(pid_t pid, memobj_t *o, char *key)
{
	int hash = _GetHash(key);
	memobj_t *object = &memory[hash];
	memobj_t *prev = NULL;
	object = object->next;

	while (object) {
		if (pid == object->pid && 0 == strcmp(key, object->key)) {
			free (object->key);
			free (object->data);

			if (object->next) 
				prev->next = object->next;
			else if (prev)
				prev->next = NULL;
			
			free (object);
			object = NULL; //whoan!

			return;
		}
		prev = object;
		object = object->next;
	}
}

void *Malloc(ssize_t len)
{
	void *ptr = malloc(len);
	if (!ptr)
		Bork("malloc()");

	return ptr;
}


void DumpData(void) 
{
	memobj_t *m = NULL;
	int i = 0;
	for (i = 0; i < TABLE_SIZE; i++) {
		m = &memory[i];
		while (m) {
			if (m->key) {
				printf("key is %s data is %s\n", m->key, m->data);
				printf("pid is %d data is %s\n\n", m->pid, m->data);
			}
			m = m->next;
		}
	
	}
}

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SUPER_SIZE sizeof(memobj_t *) * TABLE_SIZE

void init(void)
{
	int x;
	key_t key = 12345678;
	char *shm;

	int id = shmget(key, SUPER_SIZE, IPC_CREAT | 0666);
	
	shm = shmat(id, NULL, 0);
	
	for (x = 0; x < TABLE_SIZE; x++) {
		memory[x].pid = 0;
		memory[x].data = NULL;
		memory[x].key = NULL;
		memory[x].count = 0;
	}

}
//void *SetMemObj(pid_t pid, char *key, ssize_t len)

void ProcessCommand(int sock, char *buf) 
{
	if (0 == strcmp(buf, "QUIT")) {
		exit(EXIT_SUCCESS);
	}
	
	char delim[] = " ";
	char *p = strtok(buf, delim);
	char *command = strdup(p);

	char *a, *key, *len;
	int count = 0;
	while (p) {
		if (count == 0)
			a = strdup(p);
		else if (count == 1)
			key = strdup(p);
		else if (count == 2)
			len = strdup(p);
	
		p = strtok(NULL, delim);
		++count;
	}
	if (count < 1)
		return;
	
	char msg[8192] = { 0 };
	
	if (0 == strcmp(command, "PUT")) {
		SetMemObj((ssize_t)atoi(a), key, (int) atoi(len), key);
		sprintf(msg, "new entry made\n");
		write(sock, msg, strlen(msg));
		printf("ADDED %d %s %d\n", atoi(a), key, len);
	} else if (0 == strcmp(command, "GET")) {
		char *data = PrintMemObj(atoi(key), key);
		write(sock, data, strlen(data));
	}
	DumpData();
}


#endif
