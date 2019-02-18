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

mqd_t server, uclient;

int user_lookup[MAX_USERS];
int lookup_matrix[MAX_GROUPS][MAX_USERS];
char in_buffer[MAX_MSG_SIZE+10], out_buffer[MAX_MSG_SIZE+10];

int open_server()
{
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_MSGS;
	attr.mq_msgsize = MAX_MSG_SIZE;
	attr.mq_curmsgs = 0;

	if((server = mq_open(SERVER, O_RDONLY | O_CREAT, PERMISSIONS, &attr)) == -1)
	{
		perror(">> Server start error");
		return 1;
	}

	printf(">> Server open successful ...\n");
	return 0;

}

int open_uclient()
{
	struct mq_attr attr;
	attr.mq_flags = 0;
	attr.mq_maxmsg = MAX_MSGS;
	attr.mq_msgsize = MAX_MSG_SIZE;
	attr.mq_curmsgs = 0;

	if((uclient = mq_open(U_CLIENT, O_WRONLY | O_CREAT, PERMISSIONS, &attr)) == -1)
	{
		perror(">> Universal client start error");
		return 1;
	}

	printf(">> Universal client open successful ...\n");
	return 0;

}

int read_server_queue()
{
	if((mq_receive(server, in_buffer, MAX_MSG_SIZE+10, NULL)) == -1)
	{
		perror(">> Server queue read error");
		return 1;
	}
	printf(">> Message received ...\n");
	return 0;
}

int send_uclient_queue()
{
	if((mq_send(uclient, out_buffer, strlen(out_buffer)+1,0)) == -1)
	{
		perror(">> Server write error (Universal Client)");
		return 1;
	}
	return 0;
}

int send_client_queue(int client_id)
{
	// TO DO
	return 0;
}

void debug()
{
	puts(out_buffer);
	printf("%c\n", out_buffer[24]);
}

int list()
{
	memset(out_buffer, '\0', sizeof(out_buffer));
	strcpy(out_buffer,"List of Active groups: ");
	int ind = 22;
	for(int i=0;i<MAX_GROUPS;++i)
	{
		for(int j=0;j<MAX_USERS;++j)
			if(lookup_matrix[i][j] == 1)
			{
				if(i < 10)
				{
					out_buffer[ind] = i+'0';
					ind++;
					out_buffer[ind] = ' ';
					ind++;
				}
				else
				{
					out_buffer[ind] = (i/10)+'0';
					ind++;
					out_buffer[ind] = (i%10)+'0';
					ind++;
					out_buffer[ind] = ' ';
					ind++;
				}
				break;
			}
	}
	if( send_uclient_queue() == 1)
		return 1;
	return 0;
}

int join()
{
	// TO DO
	return 0;
}

int message()
{
	if(in_buffer[0] == '!')
	{
		if(in_buffer[1] == 'l' && in_buffer[2] == 'i' && in_buffer[3] == 's' && in_buffer[4] == 't') // !list
		{
			printf(">> Processing list request ...\n");
			if(list() != 0)
				perror(">> Listing error");
			debug();
			printf(">> List return successful\n");
			return 0;
		}
		else if(in_buffer[1] == 'j' && in_buffer[2] == 'o' && in_buffer[3] == 'i' && in_buffer[4] == 'n') // !join
		{
			printf(">> Processing join request...\n");
			if(join() != 0)
				perror(">> Could not join");
			return 0;
		}
		return 2;
	}
	else if(in_buffer[0] == '@')
		return 0;
	return 2;
}

void exit_handler(int sig_no)
{
	mq_close(server);
	mq_unlink(SERVER);
	mq_close(uclient);
	mq_unlink(U_CLIENT);
	printf("\n>> Server closed\n");
	signal(sig_no, SIG_DFL);
	raise(sig_no);
}

int main()
{
	// Handle closing of server
	signal(SIGINT, exit_handler);
	signal(SIGQUIT, exit_handler);
	signal(SIGTSTP, exit_handler);

	// Open server and universal client
	if( open_server() > 0 )
		exit(1);
	if( open_uclient() > 0 )
		exit(1);
	
	while(1)
	{
		memset(in_buffer, '\0', sizeof(in_buffer));
		read_server_queue();
		int flag; // type of message
		memset(out_buffer, '\0', sizeof(out_buffer));
		if((flag = message()) == 1) // not a request
		{
			int group_no = 0, i = 1,j=0;
			while(in_buffer[i] != ' ' || in_buffer[i] != '\0')
			{
				group_no *= 10;
				group_no += (in_buffer[i]-'0');
				i++;
			}
			i++;
			while(in_buffer[i]!='\0')
			{
				out_buffer[j] = in_buffer[i];
				i++;
				j++;
			}
			for(int i=0;i<MAX_USERS;++i)
			{
				if(lookup_matrix[group_no][i] == 1)
				{
					send_client_queue(i);
				}
			}
		}
		else if(flag == 2)
		{
			printf(">> Invalid message received ... Ignoring ...\n");
			continue;
		}
	}
	return 0;
}
