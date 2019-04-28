#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXMSGSIZE 512
#define MAXUSERNAMESIZE 50
#define SERVER_ADDRESS INADDR_ANY
#define SERVER_PORT 9090

int server_socket, registered = 0;
char uname[MAXUSERNAMESIZE];
int epoll_fd;
struct epoll_event event;
struct epoll_event *events;

int non_block_fd(int socket_fd)
{
	int flags, set_flags;
	flags = fcntl(socket_fd, F_GETFL, 0);
	if(flags == -1)
		return -1;
	flags |= O_NONBLOCK;
	set_flags = fcntl(socket_fd, F_SETFL, flags);
	if(set_flags == -1)
		return -1;
	printf("[+] Socket fd %d set to non block ...\n", socket_fd);
	return 0;
}

void exit_handler(int sig_no)
{
	// close(server_socket);
	printf("\n[-] Client closed\n");
	signal(sig_no, SIG_DFL);
	raise(sig_no);
}

void to_server()
{
	char buffer[MAXMSGSIZE];
	memset(buffer, '\0', sizeof(buffer));
	printf(">> %s: ", uname);
	gets(buffer);
	if(strlen(buffer) < 4)
	{
		return;
	}
	send(server_socket, buffer, sizeof(buffer), 0);
	if(strcmp(buffer, "LEAV") == 0)
	{
		printf("[-] Exiting ...\n");
		exit(0);
	}
}

void from_server()
{
	char buffer[MAXMSGSIZE];
	memset(buffer, '\0', sizeof(buffer));
	if(recv(server_socket, buffer, sizeof(buffer), 0) == 0)
	printf("[+] %s\n", buffer);
	puts(buffer);
	if(strstr(buffer, "Server: Registering as")) // Registration confirmation
	{
		memset(uname, '\0', sizeof(uname));
		registered = 1;
		int i = 23;
		while(i-23 < MAXUSERNAMESIZE && buffer[i]!='.')
		{
			uname[i-23] = buffer[i];
			i++;
		}
		// printf("[+] Updating username as %s\n", uname);
	}
	else if(strstr(buffer, "Server: Username updated"))
	{
		memset(uname, '\0', sizeof(uname));
		int i = 28;
		while(i-28 < MAXUSERNAMESIZE && buffer[i]!='.')
		{
			uname[i-28] = buffer[i];
			i++;
		}
		// printf("[+] Updating username as %s\n", uname);
	}
}

int main()
{
	// Handle closing of server
	signal(SIGINT, exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGTSTP, exit_handler);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	// fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL) | O_NONBLOCK);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	server_address.sin_addr.s_addr = SERVER_ADDRESS;

	int connection_status = connect(server_socket, (struct sockaddr*)(&server_address), sizeof(server_address));

	memset(uname, '\0', sizeof(uname));

	if(connection_status < 0)
	{
		perror("[-] Connection error");
	}

	else
	{
		int buffer[MAXMSGSIZE];
		recv(server_socket, buffer, sizeof(buffer), 0);
		printf("[+] %s\n", buffer);
	}

	if(non_block_fd(server_socket) == -1)
	{
		perror("[-] Socket non blocking error");
		exit(1);
	}

	epoll_fd = epoll_create1(0);
	if(epoll_fd == -1)
	{
		perror("[-] epoll fd creation");
		exit(1);
	}

	event.data.fd = server_socket;
	event.events = EPOLLIN | EPOLLET;
	if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1)
	{
		perror("[-] epoll_ctl");
		exit(1);
	}

	printf("[+] EPOLL setup done ...\n");

	events = calloc(1, sizeof(event));
	strcpy(uname, "User");

	while(1)
	{
		to_server();
		int active_n = epoll_wait(epoll_fd, events, 1, 1);
		for(int i=0;i<active_n;++i)
		{
			if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN)))
			{
				perror("[-] epoll error");
				close(events[i].data.fd);
				continue;
			}
			from_server();
		}
	}
	close(server_socket);
	return 0;
}