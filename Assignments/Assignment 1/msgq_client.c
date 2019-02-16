#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER "/serverqueue"
#define U_CLIENT "/uclientqueue"
#define MAX_MSGS 10
#define MAX_MSG_SIZE 1024
#define MAX_USERS 50
#define MAX_USERNAME_SIZE 20
#define MAX_GROUPS 10

mqd_t server, uclient;

char in_buffer[MAX_MSG_SIZE+10], out_buffer[MAX_MSG_SIZE+10];

int open_server()
{
	if((server = mq_open(SERVER, O_RDWR)) == -1)
	{
		perror(">> Server open error");
		return 1;
	}

	printf(">> Server open successful ...\n");
	return 0;

}

int open_uclient()
{
	if((uclient = mq_open(U_CLIENT, O_RDWR)) == -1)
	{
		perror(">> Universal client open error");
		return 1;
	}

	printf(">> Universal client open successful ...\n");
	return 0;

}

int read_uclient_queue()
{
	if((mq_receive(uclient, in_buffer, MAX_MSG_SIZE+10, NULL)) == -1)
	{
		perror(">> Server queue read error");
		return 1;
	}
	printf(">> ");
	puts(in_buffer);
	return 0;
}

int send_server_queue()
{
	if((mq_send(uclient, out_buffer, strlen(out_buffer)+1, 0)) == -1)
	{
		perror(">> Server queue write error");
		return 1;
	}
	printf(">> Request sent to server\n");
	return 0;
}

int main()
{
	if( open_server() > 0 )
		exit(1);
	if( open_uclient() > 0 )
		exit(1);
	while(1)
	{
		memset(out_buffer, '\0', sizeof(out_buffer));
		printf(">> ");
		gets(out_buffer);
		send_server_queue();
		memset(in_buffer, '\0', sizeof(in_buffer));
		read_uclient_queue();
	}
	mq_close(server);
	mq_close(uclient);
	return 0;	
}