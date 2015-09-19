#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

int port = 0;

int Connect(const char *hostname, int port)

{
	int sock;
	struct hostent *host;
	struct sockaddr_in host_addr;

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0)
	{
		perror("socket()");
	}

	host = gethostbyname(hostname);

	if (host == NULL)
	{
		perror("gethostbyname()");
	}

	host_addr.sin_family = AF_INET;
	host_addr.sin_port = htons(port);
	host_addr.sin_addr = *((struct in_addr *)host->h_addr);
	memset(&host_addr.sin_zero, 0, 8);

	int status = connect(sock, (struct sockaddr *)&host_addr,

			     sizeof(struct sockaddr));
	close(sock);

	if (status == 0)
	{
		return 1;
	}

	return 0;
}

#define MAX_THREADS 64

struct Connection_t {
	const char *hostname;
	unsigned short int port;
};

#define PROGRAM_NAME "SpeedyScan"
#define PROGRAM_VERSION "0.0.1.1"

void banner(void)
{
	printf("Copyright (c) 2015. Al Poole <netstar@gmail.com>\n");	
	printf("%s version %s\n", PROGRAM_NAME, PROGRAM_VERSION);
	printf("Maximum threads: %d\n\n", MAX_THREADS);
}


int main(int argc, char **argv)
{
	int max_port = 65535;
	int max_threads = MAX_THREADS;

	if (argc < 2)
	{
		fprintf(stderr, "%s <host> [max port]\n", PROGRAM_NAME);
		exit(1 << 8);
	}

	if (argc == 3)
	{
		max_port = atoi(argv[2]);
	}
	else
	{
		max_port = 65535;
	}

	if (MAX_THREADS < max_port)
	{
		max_threads = max_port;
	}
	char *hostname = argv[1];

	pthread_t threads[max_threads];


	banner();

	unsigned long int time_start = time(NULL);

	while(port < max_port)
	{
		++port;
		struct Connection_t c;
		c.hostname = hostname;
		c.port = port;

	        int connected = Connect(c.hostname, port);
       		if (connected)
        	{
               		printf("Connected to %s on port %d\n", c.hostname, c.port);
        	}
	}

	unsigned long int time_end = time(NULL);

	unsigned int total_time = time_end - time_start;

	printf("Completed scan in %u seconds.\n", total_time);

	return EXIT_SUCCESS;
}
