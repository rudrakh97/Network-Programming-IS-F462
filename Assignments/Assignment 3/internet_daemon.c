#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define MAXSERVNAMESIZE 20
#define MAXPATHLEN 100
#define MAXSERVICES 50
#define SERVER_ADDRESS INADDR_ANY
#define SERVER_PORT 9090

struct service
{
	char serv_name[MAXSERVNAMESIZE];
	char socket_type[10];
	char protocol[10];
	int iswait;
	char path[MAXPATHLEN];
	int service_socket;
	int isused;
};

struct sockaddr_in server_address;
int server_socket;
int epoll_fd;
struct epoll_event event;
struct epoll_event *events;
struct service service_list[MAXSERVICES];

int server_init()
{
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if(server_socket == -1)
	{
		return -1;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	server_address.sin_addr.s_addr = SERVER_ADDRESS;

	if(bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1)
	{
		return -1;
	}

	if(listen(server_socket, TCPLIMIT) == -1)
	{
		return -1;
	}
	printf("[+] Server initialised. Running on port %d ...\n", SERVER_PORT);
	return 0;
}

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

void read_config()
{

}

void search_and_call(int service_fd)
{
	int index = -1;
	for(int i=0;i<MAXSERVICES;++i)
	{
		if(service_list[i].service_socket == service_fd)
		{
			index = i;
			break;
		}
	}
	if(index == -1)
		return;
	if(service_list[index].iswait == 1)
	{
		if(service_list[index].isused == 1)
			return;
		service_list[index].isused = 1;
	}
	dup2(service_fd, 0);
	execvp(service_list[index].path);
}

void exit_handler(int sig_no)
{
	close(server_socket);
	printf("\n[-] Server closed\n");
	signal(sig_no, SIG_DFL);
	raise(sig_no);
}

int main()
{
	// Handle closing of server
	signal(SIGINT, exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGTSTP, exit_handler);

	// Initialise server socket
	if(server_init() == -1)
	{
		perror("[-] Server initialisation error");
		exit(1);
	}
	
	if(non_block_fd(server_socket) == -1)
	{
		perror("[-] Socket non blocking error");
		exit(1);
	}

	if(listen(server_socket, TCPLIMIT) == -1)
	{
		perror("[-] listen");
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

	read_config();

	printf("[+] EPOLL setup done ...\n");

	events = calloc(MAXSERVICES, sizeof(event));

	while(1)
	{
		int active_n = epoll_wait(epoll_fd, events, MAXSERVICES, -1);
		for(int i=0;i<active_n;++i)
		{
			if(events[i].data.fd == server_socket)
			{

			}
			else
			{
				int pid = fork();
				if(pid == 0)
				{
					search_and_call(events[i].data.fd);
					exit(0);
				}
			}
		}
	}

	close(server_socket);
	return 0;
}