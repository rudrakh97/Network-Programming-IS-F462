#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXMSGSIZE 512
#define MAXTHREADS 10
#define MAXUSERNAMESIZE 50
#define TCPLIMIT 5
#define SERVER_ADDRESS INADDR_ANY
#define SERVER_PORT 9090

struct client
{
	char username[MAXUSERNAMESIZE];
	struct sockaddr_in client_address;
	int client_socket;
};

struct sockaddr_in server_address;
int server_socket;
pthread_mutex_t lock;
struct client client_list[MAXTHREADS];

void client_list_init()
{
	for(int i=0;i<MAXTHREADS;++i)
		memset(client_list[i].username, '\0', sizeof(client_list[i].username));
}

void add_client(char username[], struct sockaddr_in client_address, int client_socket, int thread_id)
{
	memset(client_list[thread_id-1].username, '\0', sizeof(client_list[thread_id-1].username));
	strcpy(client_list[thread_id-1].username, username);
	client_list[thread_id-1].client_address.sin_family = client_address.sin_family;
	client_list[thread_id-1].client_address.sin_port = client_address.sin_port;
	client_list[thread_id-1].client_address.sin_addr.s_addr = client_address.sin_addr.s_addr;
	client_list[thread_id-1].client_socket = client_socket;
}

void remove_client(int thread_id)
{
	memset(client_list[thread_id-1].username, '\0', sizeof(client_list[thread_id-1].username));
}

int check_add(char username[])
{
	for(int i=0;i<MAXTHREADS;++i)
		if(strcmp(username, client_list[i].username) == 0)
			return (i+1);
	return 0;
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

	return 0;
}

void relay_msg(char msg[], int dest_id, int src_id)
{
	char msg_cpy[MAXMSGSIZE];
	memset(msg_cpy, '\0', sizeof(msg_cpy));
	sprintf(msg_cpy, "%s: %s", client_list[src_id].username, msg);
	send(client_list[dest_id].client_socket, msg_cpy, sizeof(msg_cpy), 0);
}

void process(char in_buffer[], char out_buffer[], int thread_id)
{
	if(strcmp(in_buffer, "LIST") == 0)
	{
		printf("[+] LIST command recieved\n");
		sprintf(out_buffer, "List of users online-");
		int j = strlen(out_buffer);
		for(int i=0;i<MAXTHREADS;++i)
		{
			if(client_list[i].username[0] == '\0')
				continue;
			char tmp[MAXUSERNAMESIZE+10]; memset(tmp, '\0', sizeof(tmp));
			sprintf(tmp," %d)%s", (i+1), client_list[i].username);
			for(int k=0;k<strlen(tmp); ++k)
				out_buffer[k+j] = tmp[k];
			j += strlen(tmp);
		}
		puts(out_buffer);
	}
	else if(in_buffer[0] == 'J' && in_buffer[1] == 'O' && in_buffer[2] == 'I', in_buffer[3] == 'N' && in_buffer[4] == ' ') // JOIN
	{
		printf("[+] JOIN command recieved\n");
		int i = 5;
		while(isspace(in_buffer[i]) && i < MAXMSGSIZE)
			i++;
		char target[MAXUSERNAMESIZE]; 
		memset(target, '\0', sizeof(target));
		int j = 0;
		while(!isspace(in_buffer[i]) && i < MAXMSGSIZE && in_buffer[i]!='\0')
		{
			target[j] = in_buffer[i];
			j++; 
			i++;
		}
		memset(client_list[thread_id-1].username, '\0', sizeof(client_list[thread_id-1].username));
		strcpy(client_list[thread_id-1].username, target);
		sprintf(out_buffer, "Server: Username updated as %s", target);
	}
	else if(in_buffer[0] == 'U' && in_buffer[1] == 'M' && in_buffer[2] == 'S' && in_buffer[3] == 'G' && in_buffer[4] == ' ') // UMSG
	{
		printf("[+] UMSG command recieved\n");
		int i = 5;
		while(isspace(in_buffer[i]) && i < MAXMSGSIZE)
			i++;
		char target[MAXUSERNAMESIZE]; 
		memset(target, '\0', sizeof(target));
		int j = 0;
		while(!isspace(in_buffer[i]) && i < MAXMSGSIZE)
		{
			target[j] = in_buffer[i];
			j++; 
			i++;
		}
		pthread_mutex_lock(&lock);
		int status = check_add(target);
		pthread_mutex_unlock(&lock);
		if(status == 0)
		{
			sprintf(out_buffer, "Server: User not online. Use LIST command to see who is online.");
		}
		else
		{
			memmove(in_buffer, in_buffer+5+strlen(target), strlen(in_buffer)-4-strlen(target));
			relay_msg(in_buffer, status-1, thread_id-1);
			sprintf(out_buffer, "Server: Message relayed to %s", target);
		}
	}
	else if(in_buffer[0] == 'B' && in_buffer[1] == 'M' && in_buffer[2] == 'S' && in_buffer[3] == 'G' && in_buffer[4] == ' ') // BMSG
	{
		printf("[+] BMSG command recieved\n");
		char buffer[MAXMSGSIZE]; memset(buffer, '\0', sizeof(buffer));
		int j = 5;
		while(in_buffer[j] != '\0' && j < MAXMSGSIZE)
		{
			buffer[j-5] = in_buffer[j];
			j++;
		}
		puts(buffer);
		for(int i=0;i<MAXTHREADS;++i)
		{
			if(client_list[i].username[0] == '\0')
				continue;
			relay_msg(buffer, i, thread_id-1);
		}
	}
	else
	{
		sprintf(out_buffer,"Command %s invalid. Usage:\t1)JOIN <username>\t2)BMSG <msg>\t3)UMSG <target> <msg>\t4)LIST",in_buffer);
	}
}

void *runnable(void *tid)
{
	int thread_id = (int)(tid);
	printf("[+] Thread initialized ...\n", thread_id);
	int client_socket;
	struct sockaddr_in client_address;
	int address_size = sizeof(struct sockaddr_in), hasStarted = 0;
	char uname[MAXUSERNAMESIZE], out_buffer[MAXMSGSIZE], in_buffer[MAXMSGSIZE];
	memset(uname, '\0', sizeof(uname));
	while(1)
	{
		client_socket = accept(server_socket, (struct sockaddr*)&client_address, &address_size);
		if(client_socket == -1)
		{
			printf("[-] Connection accept error ...\n");
			continue;
		}
		printf("[+] Connected to %s on remote port %d\n", inet_ntoa(client_address.sin_addr), (int) ntohs(client_address.sin_port));
		memset(out_buffer, '\0',sizeof(out_buffer));
		sprintf(out_buffer,"Server: You are now connected to the server at %s running on port %d", inet_ntoa(server_address.sin_addr), (int) ntohs(server_address.sin_port));
		send(client_socket, out_buffer, sizeof(out_buffer), 0);
		// TO DO
		hasStarted = 0;
		while(!hasStarted)
		{
			memset(in_buffer, '\0', sizeof(in_buffer));
			memset(out_buffer, '\0', sizeof(out_buffer));
			recv(client_socket, in_buffer, sizeof(in_buffer), 0);
			// printf("[+] Processing: %s\n", in_buffer);
			int name = 4;
			if(in_buffer[0] == 'J' && in_buffer[1] == 'O' && in_buffer[2] == 'I' && in_buffer[3] == 'N') // JOIN
			{
				while(isspace(in_buffer[name]) && name < MAXMSGSIZE)
					name++;
				if(name == MAXMSGSIZE || isspace(in_buffer[name]))
				{
					sprintf(out_buffer, "Invalid username: JOIN <ASCII character(s) with no spaces>");
					send(client_socket, out_buffer, sizeof(out_buffer), 0);
					continue;
				}
				else
				{
					for(int i=name;!isspace(in_buffer[i]) && in_buffer[i]!='\0' && i-name < MAXUSERNAMESIZE; ++i)
						uname[i-name] = in_buffer[i];
					pthread_mutex_lock(&lock);
					int status = check_add(uname);
					if(status != 0)
					{
						sprintf(out_buffer, "Server: Username %s already in use. Try another username.", uname);
						printf("[+] %s: Username conflict\n", uname);
						send(client_socket, out_buffer, sizeof(out_buffer), 0);
						pthread_mutex_unlock(&lock);
						continue;
					}
					sprintf(out_buffer, "Server: Registering as %s. Use the command JOIN <username> again to change.", uname);
					add_client(uname, client_address, client_socket, thread_id);
					pthread_mutex_unlock(&lock);
					printf("[+] %s registered.\n", uname);
					hasStarted = 1;
					send(client_socket, out_buffer, sizeof(out_buffer), 0);
					break;
				}
			}
			else
			{
				sprintf(out_buffer, "Server: Register username to send messages. Use JOIN <username>");
				send(client_socket, out_buffer, sizeof(out_buffer), 0);
			}
		}
		hasStarted = 1;
		while(hasStarted)
		{
			memset(in_buffer, '\0', sizeof(in_buffer));
			memset(out_buffer, '\0', sizeof(out_buffer));
			if(recv(client_socket, in_buffer, sizeof(in_buffer), 0) == 0 || (strcmp(in_buffer, "LEAV") == 0))
			{
				pthread_mutex_lock(&lock);
				remove_client(thread_id);
				pthread_mutex_unlock(&lock); 
				printf("[-] %s left\n", uname);
				break;
			}
			process(in_buffer, out_buffer, thread_id);
			send(client_socket, out_buffer, sizeof(out_buffer), 0);
		}
		close(client_socket);
	}
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

	// Set client list to default values 0.0.0.0, localhost
	client_list_init();

	pthread_t thread_pool[MAXTHREADS];
	pthread_mutex_init(&lock, NULL);

	for(int i=0;i<MAXTHREADS;++i)
	{
		pthread_create(&thread_pool[i], NULL, runnable, (void *)(i+1));
	}

	for(int i=0;i<MAXTHREADS;++i)
	{
		pthread_join(thread_pool[i], NULL);
	}

	close(server_socket);

	return 0;
}