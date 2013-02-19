/*
**	selectserver.c -- a stream socket select server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define PORT "3490"

#define BACKLOG 10

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(void)
{
	int sockfd, new_fd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	socklen_t sin_size;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	fd_set readfds, tmpfds;
	int fdmax;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}

	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);

	while (1)
	{
		int i;

		fdmax = sockfd;
		tmpfds = readfds;
		printf("server: waiting for connections...\n");

		if (select(fdmax + 1, &tmpfds, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(2);
		}

		for (i = 0; i <= fdmax; ++i)
		{
			if (FD_ISSET(i, &readfds))
			{
				if (i == sockfd)
				{
					sin_size = sizeof their_addr;
					new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
					if (new_fd == -1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(new_fd, &readfds);
						if (new_fd > fdmax)
						{
							fdmax = new_fd;
						}
						inet_ntop(their_addr.ss_family, 
								get_in_addr((struct sockaddr *)&their_addr), 
								s, sizeof s);
						printf("selectserver: got connection from %s"
								" on socket %d\n", s, new_fd);
					}
				}
				else
				{
					if (send(new_fd, "Hello, client!", 14, 0) == -1)
					{
						perror("send");
					}

					close(new_fd);
					FD_CLR(i, &readfds);
				}
			}
		}
	}
	
	return 0;
}
