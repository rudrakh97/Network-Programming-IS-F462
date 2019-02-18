#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER "/sq"
#define U_CLIENT "/ucq"
#define MAX_MSGS 10
#define MAX_MSG_SIZE 1024
#define PERMISSIONS 0660
#define MAX_USERS 50
#define MAX_USERNAME_SIZE 20
#define MAX_GROUPS 10

mqd_t server, uclient, client;

char in_buffer[MAX_MSG_SIZE+10], out_buffer[MAX_MSG_SIZE+10], clientq[15], pid[15];

int open_server()
{
	if((server = mq_open(SERVER, O_WRONLY)) == -1)
	{
		perror(">> Server open error");
		return 1;
	}

	printf(">> Server open successful ...\n");
	return 0;

}

int open_uclient()
{
	if((uclient = mq_open(U_CLIENT, O_RDONLY)) == -1)
	{
		perror(">> Universal client open error");
		return 1;
	}

	printf(">> Universal client open successful ...\n");
	return 0;

}

void set_clientq()
{
	memset(clientq, '\0', sizeof(clientq));
	sprintf(clientq,"/%dq",getpid());
	memset(pid, '\0', sizeof(pid));
	sprintf(pid," %d",getpid());
}

int open_client()
{
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_MSGS;
	attr.mq_msgsize = MAX_MSG_SIZE;
	attr.mq_curmsgs = 0;
	set_clientq();
	if((client = mq_open(clientq, O_RDONLY | O_CREAT, PERMISSIONS, &attr)) == -1)
	{
		perror(">> Client start error");
		return 1;
	}
	printf(">> Client open successful: %s ...\n", clientq);
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

int read_client_queue()
{
	if((mq_receive(client, in_buffer, MAX_MSG_SIZE+10, NULL)) == -1)
	{
		perror(">> Client queue read error");
		return 1;
	}
	printf(">> ");
	puts(in_buffer);
	return 0;
}

int send_server_queue()
{
	strcat(out_buffer, pid);
	if((mq_send(server, out_buffer, strlen(out_buffer)+1, 0)) == -1)
	{
		perror(">> Server queue write error");
		return 1;
	}
	printf(">> Request sent to server\n");
	return 0;
}

void exit_handler(int sig_no)
{
	mq_close(server);
	mq_unlink(SERVER);
	mq_close(uclient);
	mq_unlink(U_CLIENT);
	mq_close(client);
	mq_unlink(clientq);
	printf("\n>> Client exited\n");
	signal(sig_no, SIG_DFL);
	raise(sig_no);
}

int main()
{
	// Handle closing of server
	signal(SIGINT, exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGTSTP, exit_handler);

	if( open_server() > 0 )
		exit(1);
	if( open_uclient() > 0 )
		exit(1);
	if( open_client() > 0 )
		exit(1);

	int fpid = fork();
	if(fpid == 0)
	{
		while(1)
		{
			memset(out_buffer, '\0', sizeof(out_buffer));
			printf(">> ");
			gets(out_buffer);
			send_server_queue();
			memset(in_buffer, '\0', sizeof(in_buffer));
			if(out_buffer[0] == '!' && out_buffer[1] == 'l' && out_buffer[2] == 'i' && out_buffer[3] == 's' && out_buffer[4] == 't') // !list
			{
				read_uclient_queue();
				continue;
			}
		}
	}
	else
	{
		while(1)
			read_client_queue();
	}
	return 0;
}
