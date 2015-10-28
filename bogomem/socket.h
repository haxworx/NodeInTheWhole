#ifndef __SOCKET_H__
#define __SOCKET_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include "bogomem.h"

int Server(int port)
{
    int sock, c_sock, len;
    char buf[8192] = { 0 };
    struct sockaddr_in s_addr, c_addr;
    fd_set readfds;
    puts("HERE");

    sock = socket(AF_INET, SOCK_STREAM, 0);

    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, flags | O_NONBLOCK);

    int maxfd = sock + 1;

    memset(&s_addr, 0, sizeof(s_addr));

    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = INADDR_ANY;
    s_addr.sin_port = htons(port);

    int result = bind(sock, (struct sockaddr *) &s_addr, sizeof(s_addr));
    if (result < 0)
	Bork("bind %s", strerror(errno));


    result = listen(sock, 5);
    int i;
    c_sock = 0;

    fd_set alpha_set;
    FD_ZERO(&alpha_set);
    FD_SET(sock, &alpha_set);

    for (;;) {
	readfds = alpha_set;

	char buf[8192] = { 0 };

	result = select(maxfd, &readfds, NULL, NULL, NULL);
	if (result < 0)
	    Bork("select");

	for (i = 0; i < maxfd; i++) {

	    if (FD_ISSET(i, &readfds)) {
		if (i == sock) {
		    len = sizeof(c_addr);
		    c_sock = accept(sock, (struct sockaddr *)
				    &c_addr, &len);
		    if (c_sock < 0)
			Bork("accept");
		    FD_SET(c_sock, &alpha_set);
			maxfd = c_sock + 1;

		} else {
		    read(i, buf, sizeof(buf));
		    ProcessCommand(i, buf);
		}
	    }
	}
    }
}

#endif
