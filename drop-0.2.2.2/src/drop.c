/* 
  Copyright (c) 2015, Al Poole <netstar@gmail.com>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of the <organization> nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#define PROGRAM_NAME "drop"
#define PROGRAM_VERSION "0.2.2.2 \"Saint Eusebius\""

#define _DEFAULT_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/wait.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUF_MAX 8192

#define h_addr h_addr_list[0]

#define SLASH '/'

#ifndef strlcpy
size_t strlcpy(char *d, char const *s, size_t n)
{
	return snprintf(d, n, "%s", s);
}

size_t strlcat(char *d, char const *s, size_t n)
{
	return snprintf(d, n, "%s%s", d, s);
}
#endif

#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_SIZE 1024

key_t key;
int shmid;
char *connection_broken = NULL;

int check_connection(void)
{
        if (*connection_broken) {
                fprintf(stderr, "we had to abort! server is running???\n\n");
                shmdt(connection_broken);
		exit(1 << 8);
        }
	
	return (int) *connection_broken;
}

bool debugging = false;
int run_once = 1;
char *directory = NULL;
char *hostname = NULL; 
const char *username = NULL;
const char *password = NULL;

#define REMOTE_URI "/"
#define REMOTE_PORT 23221 
// no SSL for now
int use_https_ssl = 0;


static const int parallel_max = 16;
unsigned int zip_sleep_interval = 5;

void Error(char *fmt, ...)
{
	char message[BUF_MAX] = { 0 };
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(message, BUF_MAX, fmt, ap);
	fprintf(stderr, "\nError: %s\n", message);
	va_end(ap);

	exit(EXIT_FAILURE);
}

void init_ssl(void)
{
	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();
}

char *file_name_from_path(char *path)
{
	if (path == NULL) {
		return NULL;
	}

	char *t = strrchr(path, '/');
	if (t)
	{
		t++;
		return t;
	}

	return path;
}

BIO *bio = NULL;

BIO *Connect_SSL(char *hostname, int port)
{
	//BIO *bio = NULL;
	char bio_addr[BUF_MAX] = { 0 };
	
	snprintf(bio_addr, sizeof(bio_addr), "%s:%d", hostname, port);
	
	SSL_library_init();
	
	SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
	SSL *ssl = NULL;
	
	bio = BIO_new_ssl_connect(ctx);
	if (bio == NULL)
	{
		Error("BIO_new_ssl_connect");
	}
	
	BIO_get_ssl(bio, &ssl);
	SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	BIO_set_conn_hostname(bio, bio_addr);
	
	if (BIO_do_connect(bio) <= 0)
	{
		Error("SSL Unable to connect");
	}

	return bio;
}

int Connect(char *hostname, int port)
{
	if (use_https_ssl)
	{
		Connect_SSL(hostname, port);
		return 1;
	}
	
	int sock;
	struct hostent *host;
	struct sockaddr_in host_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		Error("socket()");
	}

	host = gethostbyname(hostname);
	if (host == NULL)
	{
		Error("gethostbyname()");
	}

	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(port);
	host_addr.sin_addr = *((struct in_addr *)host->h_addr);
	memset(&host_addr.sin_zero, 0, 8);

	int status = connect(sock, (struct sockaddr *)&host_addr,
			     sizeof(struct sockaddr));

	if (status == 0)
	{
		return sock;
	}

	return 0;
}

ssize_t Read(int sock, char *buf, int len)
{
	if (use_https_ssl)
	{
		return BIO_read(bio, buf, len);
	}
	
	return read(sock, buf, len);
}

ssize_t Write(int sock, char *buf, int len)
{
	int bytes = 0;

	if (debugging)
	{
		printf("%s", buf);
	}

	if (use_https_ssl)
	{
		bytes = BIO_write(bio, buf, len);
		return bytes;
	}

	bytes = write(sock, buf, len);	
	
	return bytes;
}

int Close(int sock)
{
	if (use_https_ssl)
	{
		BIO_free_all(bio); bio = NULL;
		return 0;
	}
	
	return close(sock);
}

bool authenticate(void)
{
	int sock = Connect(hostname, REMOTE_PORT);
	if (!sock)
	{
		Error("Could not Connect()");
	}
	
	char post[BUF_MAX] = { 0 };
	char *fmt =
		"POST %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Username: %s\r\n"
		"Password: %s\r\n"
		"Action: AUTH\r\n\r\n";
	
	snprintf(post, sizeof(post), fmt, REMOTE_URI, hostname,
		username, password);
	Write(sock, post, strlen(post));
	
	char buf[BUF_MAX] = { 0 };
	
	ssize_t len = 0;
	
	len = Read(sock, buf, sizeof(buf));
	
	buf[len] = '\0';
	
	// bogus but works
	#define AUTH_STATUS_STR "status: "
	int status = 1000;
	char *start_of_code = strstr(buf, AUTH_STATUS_STR);
	if (start_of_code)
	{
		start_of_code = strchr(start_of_code, ':');
		char *status_start;
		++start_of_code;
		status_start = start_of_code;
		while (*start_of_code != '\r')
		{
			start_of_code++;
		}
		if (*start_of_code == '\r')
		{
			*start_of_code = '\0';
			status = atoi(status_start);
		}
	}
	
	if (status != 0) 
	{
		Error("Invalid username or password!");
	}
	
	Close(sock);
	
	return true;
}

bool remote_file_del(char *file)
{
	char path[PATH_MAX] = { 0 };
	snprintf(path, sizeof(path), "%s%c%s", directory, SLASH, file);
	if (debugging)
	{
		printf("the path is %s\n", path);
	}

	int sock = Connect(hostname, REMOTE_PORT);
	if (!sock)
	{
		fprintf(stderr, "Could not connect()\n");
		return false;
	}

	int content_length = 0;

	char *file_from_path = file_name_from_path(path);

	char dirname[PATH_MAX] = { 0 };
	snprintf(dirname, sizeof(dirname), "%s", directory);
        char *dir_from_path = file_name_from_path(dirname);

	char post[BUF_MAX] = { 0 };
	char *fmt =
		"POST %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Content-Length: %d\r\n"
		"Username: %s\r\n"
		"Password: %s\r\n"
		"Directory: %s\r\n"
		"Filename: %s\r\n"
		"Action: DEL\r\n\r\n";

	snprintf(post, sizeof(post), fmt, REMOTE_URI, hostname,
		 content_length, username, password, dir_from_path, file_from_path);

	Write(sock, post, strlen(post));

	Close(sock);

	return true;
}

bool remote_file_add(char *file)
{
	char path[PATH_MAX] = { 0 };
	snprintf(path, sizeof(path), "%s%c%s", directory, SLASH, file);
	if (debugging)
	{
		printf("the path is %s\n", path);
	}

	char dirname[PATH_MAX] = { 0 };
	snprintf(dirname, sizeof(dirname), "%s", directory);

	int sock = Connect(hostname, REMOTE_PORT);
	if (!sock)
	{
		fprintf(stderr, "Could not connect()\n");
		return false;
	}

	struct stat fstats;
	if (strlen(path) == 0)
	{
		return false;
	}

	if (stat(path, &fstats) < 0)
	{
		return false;
	}

	FILE *f = fopen(path, "rb");
	if (f == NULL)
	{
		Error("Unable to open filename %s, this should not happen!",
		      path);
	}

#define CHUNK 1024

	char buffer[CHUNK + 1] = { 0 };

	int content_length = fstats.st_size;

	char *file_from_path = file_name_from_path(path);
	char *dir_from_path = file_name_from_path(dirname);

	char post[BUF_MAX] = { 0 };
	char *fmt =
		"POST %s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"Content-Length: %d\r\n"
		"Username: %s\r\n"
		"Password: %s\r\n"
		"Filename: %s\r\n"
		"Directory: %s\r\n"
		"Action: ADD\r\n\r\n";

	snprintf(post, sizeof(post), fmt, REMOTE_URI, hostname,
		 content_length, username, password, file_from_path, dir_from_path);

	Write(sock, post, strlen(post));

	int total = 0;

	int size = content_length;
	while (size)
	{
		while (1)
		{
			int count = fread(buffer, 1, CHUNK, f);
			//ssize_t bytes = send(sock, buffer, count, 0);
			// this is exactly the same with 0 flag
		 	ssize_t bytes = Write(sock, buffer, count);
			if (bytes < count) {
				return false;
			}
			if (bytes == 0)
			{
				break;
			}
			else if (bytes < 0) {
				return false;	
			} 
			else
			{
				size -= bytes;
				total += bytes;
			}
		}
	}

	if (size != 0)
	{
		Error("wtf?");
	}
	
	Close(sock);
	fclose(f);

	return true;
}

typedef struct File_t File_t;
struct File_t
{
	char path[PATH_MAX];
	unsigned int mode;
	ssize_t size;
	unsigned int mtime;
	int changed;
	File_t *next;
};

// haircut anyone???
void chomp(char *string)
{
	char *s = string;

	while (*s)
	{
		if (*s == '\r' || *s == '\n')
		{
			*s = '\0';
			return;
		}
		s++;
	}
}

void file_list_free(File_t * list)
{
	File_t *c = list;

	while (c)
	{
		File_t *next = c->next;
		free(c);

		c = next;
	}
}


void file_list_add(File_t * list, char *path, ssize_t size, unsigned int mode,
		 unsigned int mtime)
{
	File_t *c = list;

	while (c->next)
	{
		c = c->next;
	}

	if (c->next == NULL)
	{
		c->next = calloc(1, sizeof(File_t));
		if (c->next == NULL)
		{
			Error("calloc()");
		}

		c = c->next;
		c->next = NULL;

		char *p = file_name_from_path(path);
		
		strlcpy(c->path, p, PATH_MAX);
		c->mode = mode;
		c->size = size;
		c->mtime = mtime;
		c->changed = 0x00;
	}
}

#define FILE_ADD 0x01
#define FILE_MOD 0x02
#define FILE_DEL 0x03

File_t *file_exists(File_t * list, char *filename)
{
	File_t *f = list;

	while (f)
	{
		if (!strcmp(f->path, filename))
		{
			return f;
		}
		f = f->next;
	}

	return NULL;
}

int act_on_file_del(File_t * first, File_t * second)
{
	File_t *f = first;
	int isChanged = 0;
	
	while (f)
	{
		File_t *exists = file_exists(second, f->path);
		if (!exists)
		{
			f->changed = FILE_DEL;
			printf("del file %s\n", f->path);
			isChanged++;
		}

		f = f->next;
	}

	return isChanged;
}

int act_on_file_mod(File_t * first, File_t * second)
{
	File_t *f = second;
	int isChanged = 0;
	
	while (f)
	{
		File_t *exists = file_exists(first, f->path);
		if (exists)
		{
			if (f->mtime != exists->mtime)
			{
				printf("mod file %s\n", f->path);
				f->changed = FILE_MOD;
				isChanged++;
			}
		}

		f = f->next;
	}
	
	return isChanged;
}

int act_on_file_add(File_t * first, File_t * second)
{
	File_t *f = second;
	int isChanged = 0;
	
	while (f)
	{
		File_t *exists = file_exists(first, f->path);
		if (!exists)
		{
			f->changed = FILE_ADD;
			printf("add file %s\n", f->path);
			isChanged++;
		}

		f = f->next;
	}
	
	return isChanged;
}

/* Better hide this! 
void RemoveDirectory(char *path)
{
	DIR *d = NULL;
	struct dirent *dirent = NULL;
	int i = 0;

	char *directories[8192] = { NULL }; 	

	d = opendir(path);
	if (d == NULL)
	{
		Error("opendir()");	
	}

	while ((dirent = readdir(d)) != NULL)
	{
		char full_path[PATH_MAX] = { 0 };
		snprintf(full_path, sizeof(full_path), "%s%c%s", path, SLASH, dirent->d_name);
		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, "..") )
		{
			continue;
		}
		struct stat fs;
		stat (full_path, &fs);

		if (S_ISDIR(fs.st_mode))
		{
			directories[i++] = strdup(full_path);
		} 
		else
		{
			unlink(full_path);
		}
	}

	i = 0;
	while (directories[i] != NULL)
	{
		RemoveDirectory(directories[i]);
		rmdir(directories[i]);
		free(directories[i++]);
	}
	rmdir(path);

	closedir(d);
}
*/

int rename_directory(char *path)
{
	ssize_t len = strlen(path);

	char moved[len + 2];

	memset(&moved, 0, len + 2);

	snprintf(moved, len + 2, ".%s", path);
	printf("move dir %s to %s\n", path, moved);

	return rename(path, moved);
}

bool create_tar_file(char *path)
{
	char tar_file_path[PATH_MAX] = { 0 };
	snprintf(tar_file_path, sizeof(tar_file_path), "%s.tar.gz", path);
	char *args[5] = { 0 };

	args[0] = "tar";
	args[1] = "-zcf";
	args[2] = tar_file_path;
	args[3] = path;
	args[4] = NULL;

	pid_t pid = fork();
	if (pid < 0)
	{
		Error("fork()");
	}
	else if (pid == 0)
	{
		execvp(args[0], args);
		exit(1);
	}
	
	pid = wait(NULL);
	//RemoveDirectory(path);
	rename_directory(path);
		
	return true;
}


bool create_zip_file(char *path)
{
	char zip_file_path[PATH_MAX] = { 0 };
	snprintf(zip_file_path, sizeof(zip_file_path), "%s.zip", path);

	char *args[5] = { 0 };
/*	
	char *vbscript =
	"ArchiveFolder Wscript.Arguments.Item(0), Wscript.Arguments.Item(1)\r\n"
	"Sub ArchiveFolder (zipFile, sFolder)\r\n"
	"With CreateObject(\"Scripting.FileSystemObject\")\r\n"
        "zipFile = .GetAbsolutePathName(zipFile)\r\n"
        "sFolder = .GetAbsolutePathName(sFolder)\r\n"
	"	With .CreateTextFile(zipFile, True)\r\n"
        "		.Write Chr(80) & Chr(75) & Chr(5) & Chr(6) & String(18, chr(0))\r\n"
        "	End With\r\n"
	"End With\r\n"
	"With CreateObject(\"Shell.Application\")\r\n"
	"	.NameSpace(zipFile).CopyHere .NameSpace(sFolder).Items\r\n"
	"	Do Until .NameSpace(zipFile).Items.Count = _\r\n"
        "		.NameSpace(sFolder).Items.Count\r\n"
	"	WScript.Sleep 1000\r\n"
        "	Loop\r\n"
	"End With\r\n"
	"End Sub\r\n";
*/	
	#define VBSCRIPT_PATH "zip.vbs"
	
	args[0] = "cscript";
	args[1] = VBSCRIPT_PATH;
	args[2] = zip_file_path;
	args[3] = path;
	args[4] = NULL;
	pid_t p = fork();
	if (p < 0)
		Error("fork");
	else if (p == 0)
	{
		execvp(args[0], args);
		exit(1);
	}

	
	p = wait(NULL);
	rename_directory(path);	

	return true;
}

#include <strings.h>
#include <errno.h>

File_t *files_in_directory(const char *path)
{
	DIR *d = NULL;
	struct dirent *dirent = NULL;
	d = opendir(path);
	if (d == NULL)
	{
		Error("opendir()");
	}

	File_t *list = calloc(1, sizeof(File_t));
	if (list == NULL)
	{
		Error("calloc()");
	}

	while ((dirent = readdir(d)) != NULL)
	{
		if (!strncmp(dirent->d_name, ".", 1))
		{
			continue;
		}

		unsigned int unix_time_now = time(NULL);

		char path_full[PATH_MAX] = { 0 };
		snprintf(path_full, PATH_MAX, "%s%c%s", path, SLASH,
			 dirent->d_name);
		struct stat fs;
		stat(path_full, &fs);

		if (S_ISDIR(fs.st_mode))
		{
			if (fs.st_mtime >= unix_time_now)
			{
				continue;
				// Let directories bloom!
			}
			//sleep(zip_sleep_interval);
			create_tar_file(dirent->d_name);		
		}
		else
		{
			file_list_add(list, path_full, fs.st_size, fs.st_mode,
				    fs.st_mtime);
		}
	}

	closedir(d);

	return list;
}

#define STATE_FILE_FORMAT "%s\t%u\t%u\t%u"
char *drop_config_directory = NULL;

#define DROP_CONFIG_DIR_NAME ".drop"
#define DROP_STATE_FILE "state_file"

void save_file_list_state(File_t * list)
{
	char state_file_path[PATH_MAX] = { 0 };

	snprintf(state_file_path, PATH_MAX, "%s%c%s", drop_config_directory,
		 SLASH, DROP_STATE_FILE);

	FILE *f = fopen(state_file_path, "w");
	if (f == NULL)
	{
		Error("SaveFileState: fopen");
	}

	File_t *c = list->next;
	while (c)
	{
		// we could refactor this...just include stat struct rather
		// than individual members...ahhh...f*ck it!
		fprintf(f, STATE_FILE_FORMAT, c->path, (unsigned int)c->size,
			(unsigned int)c->mode, (unsigned int)c->mtime);
		fprintf(f, "\n");
		
		c->changed = 0; // reset
		c = c->next;
	}

	fclose(f);
}

// This is slightly bogus but it does work...
bool get_list_from_state_file(char *text, char *buf, int *size, int *mode, int *ctime)
{
	const char delim[] = "\t\0";
	
	char *ptr = strtok(text, delim);
	if (ptr)
	{
		snprintf(buf, PATH_MAX, "%s", ptr);
		ptr = strtok(NULL, delim);
		if (ptr)
		{
			*size = atoi(ptr);
			ptr = strtok(NULL, delim);
			if (ptr)
			{
				*mode = atoi(ptr);
				ptr = strtok(NULL, delim);
				if (ptr)
				{
					*ctime = atoi(ptr);
					return true;
				
				}
			}
		}
	}
	
	return false;	
}

File_t *files_from_state_file(const char *state_file_path)
{
	FILE *f = fopen(state_file_path, "r");
	if (f == NULL)
	{
		Error("ListFromStateFile: %s", state_file_path);
	}

	char path[PATH_MAX] = { 0 };
	
	File_t *list = calloc(1, sizeof(File_t));
	if (list == NULL)
	{
		Error("ListFromStateFile: calloc");
	}

	char line[1024] = { 0 };
	while ((fgets(line, sizeof(line), f)) != NULL)
	{
		chomp(line);
		
		int mtime, size, mode;
		bool status = get_list_from_state_file(line, path, &size, &mode, &mtime);
		if (status)
		{
			file_list_add(list, path, size, mode, mtime);
		}
	}

	fclose(f);

	return list;
}

#define COMMAND_MAX 2048

bool process_object(File_t *object)
{
	bool success = true;
	switch (object->changed)
	{
		case FILE_ADD:
			success = remote_file_add(object->path);
			if (success) {
				printf("OK! add remote file %s\n", object->path);
			}
		break;
		
		case FILE_MOD:
			success = remote_file_add(object->path);
			if (success) {
				printf("OK! mod remote file %s\n", object->path);
			}
		break;
		
		case FILE_DEL:
			success = remote_file_del(object->path);
			if (success) {
				printf("OK! del remote file %s\n", object->path);
			}
		break;
	}

	return success;
}

#include <sys/wait.h>
int n_jobs = 0;

void wait_for_job(void)
{
	pid_t p;
	p = wait(NULL);
	if (p <=0)
	{
		Error("wait failed");
	}
	--n_jobs;
}

void wait_for_all_jobs(void)
{
	while (n_jobs)
	{
		wait_for_job();	
	}
}

void start_job(File_t *object)
{
	check_connection();
	pid_t p;
	if (n_jobs == parallel_max)
	{
		wait_for_job();
	}
	p = fork();
	if (p < 0)
	{
		Error("fork failed");
	}
	else if (p == 0)
	{
		bool status = process_object(object);
		if (!status) {
			*connection_broken = 1;
		}
		exit(1);
	}
	++n_jobs;
}

unsigned int new_repository = 0;

void compare_file_lists(File_t * first, File_t * second)
{
	bool store_state = false;
	int modifications = 0;
	
	// if a child cannot connect get out of here!
	
	modifications += act_on_file_add(first, second);
	modifications += act_on_file_del(first, second);
	modifications += act_on_file_mod(first, second);
	
	if (modifications)
	{
		File_t files[modifications];
		memset(files, 0, sizeof(File_t) * modifications);
		int i = 0;
		
		File_t *cursor = first->next;
		while (cursor)
		{
			if (cursor->changed)
			{
				memcpy(&files[i], cursor, sizeof(File_t));
				i++;
			}
			cursor = cursor->next;
		}
		
		cursor = second->next;
		while (cursor)
		{
			if (cursor->changed)
			{
				memcpy(&files[i], cursor, sizeof(File_t));
				i++;
			}
			cursor = cursor->next; //oops!
		}

		store_state = true;

		int total_files = modifications;	

		for (int i = 0; i < total_files; i++)
		{
			start_job(&files[i]);
		}

		wait_for_all_jobs();
	
		printf("done!\n\n");
	}

	if ((!*connection_broken) && (new_repository || store_state))
	{
		save_file_list_state(second);
	}
}


File_t *first_run(char *path)
{
	char state_file_path[PATH_MAX] = { 0 };

	snprintf(state_file_path, PATH_MAX, "%s%c%s", drop_config_directory,
		 SLASH, DROP_STATE_FILE);

	struct stat fstats;

	File_t *list = NULL;

	if (stat(state_file_path, &fstats) < 0)
	{
		if (debugging)
		{
			printf("this is the first run\n");
		}

		list = files_in_directory(path);

		if (new_repository) {
			File_t *cursor = list->next;
			while (cursor) {
				cursor->changed = FILE_ADD;			
				printf("init file %s\n", cursor->path);
				
				start_job(cursor);
				cursor = cursor->next;
			}
			wait_for_all_jobs();
			printf("done!\n");
		}
	}
	else
	{
		list = files_from_state_file(state_file_path);
	}

	return list;
}
	

unsigned int changes_interval = 2;
// 2 seconds like our old friend arcs :-)

void check_for_changes(char *path)
{
	chdir(path);
	File_t *file_list_one = first_run(path);	// FilesInDirectory(path); 
	for (;;)
	{
		sleep(changes_interval);

		if (debugging)
		{
			puts("PING!");
		}
		
		File_t *file_list_two = files_in_directory(path); 

		compare_file_lists(file_list_one, file_list_two);

		file_list_free(file_list_one);
		if (run_once) {
			file_list_free(file_list_two);
			return;
		}
		file_list_one = file_list_two;
	}
}
/*
 *
int ConfigValue(char *text, char *name, char *destination, ssize_t len)
{
	char *i = NULL;

	i = strstr(text, name);
	if (i)
	{
		i += strlen(name) + 1;
		char *e = strchr(i, '\n');
		*e = '\0';
		char *value = strdup(i);
		*e = '\n';	// don't break text
		strlcpy(destination, value, len);

		free(value);

		return 1;
	}

	return 0;
}

 
typedef struct config_t config_t;
struct config_t {
	char directory[PATH_MAX];
	char hostname[BUF_MAX];
	char username[BUF_MAX];
	char password[BUF_MAX];
};

*/
// if we use a config file, these are the option key=value keys

//#define CONFIG_DIRECTORY "DIRECTORY"
//#define CONFIG_HOSTNAME  "HOSTNAME"
//#define CONFIG_USERNAME  "USERNAME"
//#define CONFIG_PASSWORD  "PASSWORD"

/*
config_t *ConfigLoad(void)
{
	config_t *config = calloc(1, sizeof(config_t));
	
	char config_file_path[PATH_MAX] = { 0 };

	snprintf(config_file_path, PATH_MAX, "%s%c%s", DROP_CONFIG_DIRECTORY,
		 SLASH, DROP_CONFIG_DIR_NAME);

	FILE *f = fopen(config_file_path, "r");
	if (f == NULL)
	{
		Error("Could not open %s", config_file_path);
	}

	char line[BUF_MAX] = { 0 };
#define BUFSIZE 8192
#define CONFIG_MAX_LINES 20
	char map[BUFSIZE * CONFIG_MAX_LINES] = { 0 };

	int line_count = 0;
	while ((fgets(line, sizeof(line), f)) != NULL)
	{
		if (line_count >= CONFIG_MAX_LINES)
		{
			Error("Unexpected content in %s", config_file_path);
		}

		strlcat(map, line, sizeof(map));
		++line_count;
	}
	
	fclose(f);

	int result =
		ConfigValue(map, CONFIG_DIRECTORY, config->directory,
			    PATH_MAX);
	if (!result)
	{
		return NULL;
		// Could check config on missing option basis...
	}

	result = ConfigValue(map, CONFIG_USERNAME, config->username,
			     BUF_MAX);
	if (!result)
	{
		return NULL;
		// Could check config on missing option basis...
	}

	result = ConfigValue(map, CONFIG_PASSWORD, config->password,
			     sizeof(config->password));
	if (!result)
	{
		return NULL;// Could check config on missing option basis...
	}

	This should not be variable if commercial product!!!
	result = ConfigValue(map, CONFIG_HOSTNAME, config->hostname,
			     sizeof(config->hostname));
	if (!result)
	{
		return NULL;
	}
	
	return config;
}
*/

void check_remote_auth(void)
{
	#define SANESTRING 80
	char stringuser[BUF_MAX] = { 0 };
	if (username == NULL) {	
		printf("\nUsername: "); fflush(stdout);
		fgets(stringuser, SANESTRING, stdin); chomp(stringuser);
		username = strdup(stringuser);
		if(strlen(stringuser) == 0) {
			exit(1 << 3);
		}
	}

	char echo_to_screen[BUF_MAX] = { 0 };
	snprintf(echo_to_screen, BUF_MAX, "%s@%s's password: ", username, hostname);
	char *stringpass = getpass(echo_to_screen);	
	if (strlen(stringpass) == 0) {
		exit( 1 << 3);
	}
	
	password = strdup(stringpass);
	authenticate();
}

void get_ready(void)
{
	char program_folder[PATH_MAX] = { 0 };

	pid_t pid = getpid();
	
	key = (int) pid;

	shmid = shmget(key, SHM_SIZE, 0644 | IPC_CREAT);
	if (shmid < 0) {
		perror("shmget()");
		exit(EXIT_FAILURE);
	}
	
	connection_broken = shmat(shmid, (void *) 0, 0);
	if (connection_broken == (char *)(-1)) {
		perror("shmat()");
		exit(EXIT_FAILURE);
	}

	*connection_broken = 0; // all okay

	char *user_home = getenv("HOME");
	if (user_home == NULL)
	{
		Error("Could not get ENV 'HOMEPATH'");
	}

        ssize_t len = strlen(directory);
        if (directory[len - 1] == '/') {
                directory[len - 1] = '\0';
        }
	
	snprintf(program_folder, sizeof(program_folder), "%s%c%s", directory,
		SLASH, DROP_CONFIG_DIR_NAME);
	
	drop_config_directory = strdup(program_folder);
	
	if (use_https_ssl)
	{
		init_ssl();
	}

        struct stat fstats;

        if (stat(drop_config_directory, &fstats) < 0)
        {
                mkdir(drop_config_directory, 0777);
                new_repository = 1;
        }

        if (stat(directory, &fstats) < 0)
        {
                mkdir(directory, 0777);
        }

	char state_file_path[PATH_MAX] = { 0 };
	snprintf(state_file_path, PATH_MAX, "%s%c%s", drop_config_directory,
                 SLASH, DROP_STATE_FILE);

	if (stat(state_file_path, &fstats) < 0) {
		new_repository = 1;
	}
}

// username@hostname:directory

void about(void)
{
	printf("Copyright (c) 2015. Al Poole <netstar@gmail.com>\n");
}

void show_version(void)
{
	about();
	printf("drop version %s\n", PROGRAM_VERSION);
}

void show_usage(void)
{
	printf("%s [OPTIONS] username@host:/path/to/dir\n", PROGRAM_NAME);
	printf("OPTIONS:\n");
	printf("    -l    loop forever\n");
	exit(EXIT_FAILURE);
}

#define COMMAND_ARGS_FMT "%s@%s:%s"

void get_user_host_rsrc(char *cmd)
{
	char user[BUF_MAX] = { 0 };
	char host[BUF_MAX] = { 0 };
	char rsrc[BUF_MAX] = { 0 };
	char *string_copy = strdup(cmd);

	char *s = strchr(string_copy, '@');
	if (!s) 
		show_usage();
	*s = '\0';
	snprintf(user, sizeof(user), "%s", string_copy);

	string_copy = s + 1;

	s = strchr(string_copy, ':');
	if (!s)	
		show_usage();
	*s = '\0';
	snprintf(host, sizeof(host), "%s", string_copy);

	string_copy = s + 1;
	if (!s)
		show_usage();
	
	snprintf(rsrc, sizeof(rsrc), "%s", string_copy);

	username = strdup(user);
	hostname = strdup(host);
	directory = strdup(rsrc);
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		show_usage();
	} 
	char *cmd_string = argv[1];

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-l")) {
			run_once = 0;
			cmd_string = argv[2];
		}
		
		if (!strcmp(argv[i], "-h")) {
			show_usage();
		}
	}
	
	if (cmd_string == NULL) {
		show_usage();
	}

	get_user_host_rsrc(cmd_string);

	show_version();

	check_remote_auth();

	get_ready();

	check_for_changes(directory);

	return EXIT_SUCCESS;
}
