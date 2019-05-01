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
#include <mqueue.h>

#define MAXMSGSIZE 512
#define MAXEVENTS 10
#define MAXCLIENTS 50
#define MAXUSERNAMESIZE 50
#define TCPLIMIT 5
#define SERVER_ADDRESS INADDR_ANY
#define SERVER_PORT 9090
#define SERVERQ "/sq"
#define PERMISSIONS 0666

struct client
{
	char username[MAXUSERNAMESIZE];
	struct sockaddr_in client_address;
	int client_socket;
};

struct sockaddr_in server_address;
int server_socket;
int epoll_fd;
struct epoll_event event;
struct epoll_event *events;
struct client client_list[MAXCLIENTS];
char inbuff[MAXMSGSIZE], outbuff[MAXMSGSIZE];
int queue_size = 0;
mqd_t server;

void client_list_init()
{
	for(int i=0;i<MAXCLIENTS;++i)
		memset(client_list[i].username, '\0', sizeof(client_list[i].username));
}

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

int open_server_queue()
{
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = MAXEVENTS;
	attr.mq_msgsize = MAXMSGSIZE*2;
	attr.mq_curmsgs = 0;

	if((server = mq_open(SERVERQ, O_RDWR | O_CREAT, PERMISSIONS, &attr)) == -1)
	{
		perror("[-] Server queue init error");
		return -1;
	}

	printf("[+] Server queue init successful ...\n");
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

int add_client(char username[], struct sockaddr_in client_address, int client_socket)
{
	int index = -1;
	for(int i=0;i<MAXCLIENTS;++i)
	{
		if(strlen(client_list[i].username) < 4)
		{
			index = i;
			break;
		}
	}
	if(index < 0)
		return index;
	sprintf(username, "User%d", index);
	strcpy(client_list[index].username, username);
	client_list[index].client_address.sin_family = client_address.sin_family;
	client_list[index].client_address.sin_port = client_address.sin_port;
	client_list[index].client_address.sin_addr.s_addr = client_address.sin_addr.s_addr;
	client_list[index].client_socket = client_socket;
	return index;
}

void remove_client(int index)
{
	memset(client_list[index].username, '\0', sizeof(client_list[index].username));
	close(client_list[index].client_socket);
}

int check(char username[])
{
	for(int i=0;i<MAXCLIENTS;++i)
		if(strcmp(username, client_list[i].username) == 0)
			return i+1;
	return 0;
}

void relay_msg(char buff[], int dest_index, int source_index)
{
	char tmp[MAXMSGSIZE];
	if(source_index >= 0)
		sprintf(tmp, "%s: %s", client_list[source_index].username, buff);
	else
		sprintf(tmp, "Server: %s", buff);
	send(client_list[dest_index].client_socket, tmp, sizeof(tmp), 0);
}

void process(int index)
{
	if(index <= 0)
		return;
	memset(outbuff, '\0', sizeof(outbuff));
	if(strcmp(inbuff, "LIST") == 0)
	{
		printf("[+] Log: LIST command recieved\n");
		sprintf(outbuff, "List of users online-");
		int j = strlen(outbuff);
		for(int i=0;i<MAXCLIENTS;++i)
		{
			if(client_list[i].username[0] == '\0')
				continue;
			char tmp[MAXUSERNAMESIZE+10]; memset(tmp, '\0', sizeof(tmp));
			sprintf(tmp," %d)%s", (i+1), client_list[i].username);
			for(int k=0;k<strlen(tmp); ++k)
				outbuff[k+j] = tmp[k];
			j += strlen(tmp);
		}
		relay_msg(outbuff, index-1, -1);
	}
	else if(inbuff[0] == 'J' && inbuff[1] == 'O' && inbuff[2] == 'I', inbuff[3] == 'N' && inbuff[4] == ' ') // JOIN
	{
		printf("[+] Log: JOIN command recieved\n");
		int i = 5;
		while(isspace(inbuff[i]) && i < MAXMSGSIZE)
			i++;
		char target[MAXUSERNAMESIZE]; 
		memset(target, '\0', sizeof(target));
		int j = 0;
		while(!isspace(inbuff[i]) && i < MAXMSGSIZE && inbuff[i]!='\0')
		{
			target[j] = inbuff[i];
			j++; 
			i++;
		}
		memset(client_list[index-1].username, '\0', sizeof(client_list[index-1].username));
		strcpy(client_list[index-1].username, target);
		sprintf(outbuff, "Username updated as %s", target);
		relay_msg(outbuff, index-1, -1);
	}
	else if(inbuff[0] == 'U' && inbuff[1] == 'M' && inbuff[2] == 'S' && inbuff[3] == 'G' && inbuff[4] == ' ') // UMSG
	{
		printf("[+] Log: UMSG command recieved\n");
		int i = 5;
		while(isspace(inbuff[i]) && i < MAXMSGSIZE)
			i++;
		char target[MAXUSERNAMESIZE]; 
		memset(target, '\0', sizeof(target));
		int j = 0;
		while(!isspace(inbuff[i]) && i < MAXMSGSIZE)
		{
			target[j] = inbuff[i];
			j++; 
			i++;
		}
		int status = check(target);
		if(status == 0)
		{
			sprintf(outbuff, "Server: User not online. Use LIST command to see who is online.");
			relay_msg(outbuff, index-1, -1);
		}
		else
		{
			memmove(inbuff, inbuff+5+strlen(target), strlen(inbuff)-4-strlen(target));
			relay_msg(inbuff, status-1, index-1);
			sprintf(outbuff, "Message relayed to %s", target);
			relay_msg(outbuff, index-1, -1);
		}
	}
	else if(inbuff[0] == 'B' && inbuff[1] == 'M' && inbuff[2] == 'S' && inbuff[3] == 'G' && inbuff[4] == ' ') // BMSG
	{
		printf("[+] Log: BMSG command recieved\n");
		char buffer[MAXMSGSIZE]; memset(buffer, '\0', sizeof(buffer));
		int j = 5;
		while(inbuff[j] != '\0' && j < MAXMSGSIZE)
		{
			buffer[j-5] = inbuff[j];
			j++;
		}
		puts(buffer);
		for(int i=0;i<MAXCLIENTS;++i)
		{
			if(client_list[i].username[0] == '\0')
				continue;
			relay_msg(buffer, i, index-1);
		}
	}
	else if(inbuff[0] == 'L' && inbuff[1] == 'E' && inbuff[2] == 'A' && inbuff[3] == 'V') // LEAV
	{
		char tmp[MAXUSERNAMESIZE];
		memset(tmp, '\0', sizeof(tmp));
		strcpy(tmp, client_list[index-1].username);
		remove_client(index-1);
		printf("[-] Log: Client %s removed ...\n", tmp);
	}
	else
	{
		sprintf(outbuff,"Command %s invalid. Usage:\t1)JOIN <username>\t2)BMSG <msg>\t3)UMSG <target> <msg>\t4)LIST",inbuff);
		relay_msg(outbuff, index-1, -1);
	}
}

void push_back(char inbuff[], int index)
{
	char msg[MAXMSGSIZE];
	memset(msg, '\0', sizeof(msg));
	sprintf(msg,"%d,%s",index,inbuff);
	// senf message
	if(mq_send(server, msg, sizeof(msg)+1, 0) == -1)
	{
		perror("[-] Push error");
		return;
	}
	printf("[+] Message queued ...\n");
	queue_size ++;
}

void pop_front()
{
	char msg[2*MAXMSGSIZE];
	memset(msg, '\0', sizeof(msg));
	// read message
	if(mq_receive(server, msg, sizeof(msg)+1, NULL) == -1)
	{
		perror("[-] Pop error");
		return;
	}
	printf("[+] Message processing ...\n");
	queue_size --;
	int index = 0, ind = 0;
	while(ind < strlen(msg))
	{
		if(msg[ind] == ',')
			break;
		index *= 10;
		index += (msg[ind]-'0');
		ind++;
	}
	ind++;
	int tmp = ind;
	memset(inbuff, '\0', sizeof(inbuff));
	while(ind < strlen(msg))
	{
		inbuff[ind-tmp] = msg[ind];
		ind++;
	}
	process(index);
}

void read_message(int client_socket)
{
	memset(inbuff, '\0', sizeof(inbuff));
	recv(client_socket, inbuff, sizeof(inbuff), 0);
	int index = -1;
	for(int i=0;i<MAXCLIENTS;++i)
	{
		if(client_list[i].client_socket == client_socket)
		{
			index = i;
			break;
		}
	}
	printf("[+] Pushing message into queue ...\n");
	push_back(inbuff, index+1);
	// process(index+1);
}

void accept_connection()
{
	while(1)
	{
		int client_socket;
		struct sockaddr_in client_address;
		int addr_size = sizeof(struct sockaddr_in);
		client_socket = accept(server_socket, (struct sockaddr*)&client_address, &addr_size);
		if(client_socket == -1)
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else
			{
				perror("[-] Accept error");
				break;
			}
		}
		printf("[+] Connected to %s on remote port %d\n", inet_ntoa(client_address.sin_addr), (int) ntohs(client_address.sin_port));
		if(non_block_fd(client_socket) == -1)
		{
			perror("[-] Client fd could not be set to non-block");
			close(client_socket);
			continue;
		}
		event.data.fd = client_socket;
		event.events = EPOLLIN | EPOLLET;
		if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1)
		{
			perror("[-] Client fd could not be added to poll pool");
			close(client_socket);
			continue;
		}
		char username[MAXUSERNAMESIZE];
		sprintf(username, "User");
		int index;
		if(index = add_client(username, client_address, client_socket) == -1)
		{
			printf("[-] Server load exceeded!");
			close(client_socket);
			continue;
		}
		printf("[+] Log: Client registered at %d\n", index);
		memset(outbuff, '\0', sizeof(outbuff));
		sprintf(outbuff, "You have been registered as %s. Use JOIN <uname> to change.", username);
		send(client_socket, outbuff, sizeof(outbuff), 0);
	}
}

void exit_handler(int sig_no)
{
	close(server_socket);
	close(server);
	mq_unlink(SERVERQ);
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

	if(open_server_queue() == -1)
		exit(1);
	// Set client list to default values 0.0.0.0, localhost
	client_list_init();

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

	printf("[+] EPOLL setup done ...\n");

	events = calloc(MAXEVENTS, sizeof(event));

	while(1)
	{
		int active_n = epoll_wait(epoll_fd, events, MAXEVENTS, -1);
		for(int i=0;i<active_n;++i)
		{
			if(events[i].data.fd == server_socket)
			{
				accept_connection();
			}
			else
			{
				read_message(events[i].data.fd);
			}
		}
		if(queue_size > 0)
		{
			printf("[+] Processing message from queue ...\n");
			pop_front();
		}
	}

	close(server_socket);
	return 0;
}