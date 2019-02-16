#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#define SERVER "/server"
#define U_CLIENT "/uclient"
#define MAX_MSGS 10
#define MAX_MSG_SIZE 1024
#define MAX_USERS 50
#define MAX_USERNAME_SIZE 20
#define MAX_GROUPS 10

mqd_t server, uclient;

char client_queues[MAX_USERS][MAX_USERNAME_SIZE];
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

	if((server = mq_open(SERVER, O_RDONLY | OCREAT, 666, &attr)) == -1)
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

	if((uclient = mq_open(U_CLIENT, O_WRONLY | OCREAT, 666, &attr)) == -1)
	{
		perror(">> Universal client start error");
		return 1;
	}

	printf(">> Universal client open successful ...\n");
	return 0;

}

int read_server_queue()
{
	if((mq_recieve(server, in_buffer, MAX_MSG_SIZE+10, NULL)) == -1)
	{
		perror(">> Server queue read error");
		return 1;
	}
	printf(">> Message recieved ...\n");
	return 0;
}

int message()
{
	if(in_buffer[0] == '!')
	{
		if(in_buffer[1] == 'l' && in_buffer[2] == 'i' && in_buffer[3] == 's' && in_buffer[4] == 't') // !list
		{
			// TO DO
			return 0;
		}
		else if(in_buffer[1] == 'j' && in_buffer[2] == 'o' && in_buffer[3] == 'i' && in_buffer[4] == 'n') // !join
		{
			// TO DO
			return 0;
		}
		return 2;
	}
	else if(in_buffer[0] == '@')
		return 0;
	return 2;
}

int send_client_queue(int client_id)
{
	// TO DO
}

int main()
{
	if( open_server() > 0 )
		exit(1);
	if( open_uclient() > 0 )
		exit(1);
	while(1)
	{
		memset(in_buffer, '\0', sizeof(in_buffer));
		read_server_queue();
		int flag;
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
			printf(">> Invalid message recieved ... Ignoring ...\n");
			continue;
		}
	}
	return 0;	
}