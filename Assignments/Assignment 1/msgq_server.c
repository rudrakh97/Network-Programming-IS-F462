#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER "/sq"
#define MAX_MSGS 10
#define MAX_MSG_SIZE 1024
#define PERMISSIONS 0660
#define MAX_USERS 50
#define MAX_USERNAME_SIZE 20
#define MAX_GROUPS 1000

mqd_t server, client;

int user_lookup[MAX_USERS];
int lookup_matrix[MAX_GROUPS][MAX_USERS];
char in_buffer[MAX_MSG_SIZE+10], out_buffer[MAX_MSG_SIZE+10], clientq[15];

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

int read_server_queue()
{
	memset(in_buffer, '\0', sizeof(in_buffer));
	if((mq_receive(server, in_buffer, MAX_MSG_SIZE+10, NULL)) == -1)
	{
		perror(">> Server queue read error");
		return 1;
	}
	printf(">> Message received ...\n");
	return 0;
}

void set_clientq(int client_pid)
{
	memset(clientq, '\0', sizeof(clientq));
	sprintf(clientq,"/%dq",client_pid);
}

int send_client_queue(int client_pid)
{
	// TO DO
	set_clientq(client_pid);
	printf(">> Opening client queue: %s\n", clientq);
	if((client = mq_open(clientq, O_WRONLY)) == -1)
	{
		perror("Client queue open error");
		return 1;
	}
	if((mq_send(client, out_buffer, strlen(out_buffer)+1,0)) == -1)
	{
		perror(">> Client queue write error");
		return 1;
	}
	mq_close(client);
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
				int pow = 1;
				while(pow*10 <= i)
					pow*=10;
				int tmp = i;
				while(pow > 0)
				{
					out_buffer[ind] = (tmp/pow)+'0';
					ind++;
					tmp %= pow;
					pow /= 10;
				}
				out_buffer[ind] = ' ';
				ind++;
				break;
			}
	}
	int pow = 1, pid = 0, j = strlen(in_buffer)-1;
	while(!isspace(in_buffer[j]))
	{
		pid += pow*(in_buffer[j]-'0');
		pow*=10;
		j--;
	}
	if( send_client_queue(pid) == 1)
		return 1;
	return 0;
}

int join()
{
	// TO DO
	memset(out_buffer, '\0', sizeof(out_buffer));
	int ind = 6;
	while(isspace(in_buffer[ind]) && in_buffer[ind] != '\0')
		ind++;
	int group_id = -1;
	while(in_buffer[ind] != '\0' && !isspace(in_buffer[ind]))
	{
		if(group_id < 0) group_id = 0;
		group_id *= 10;
		group_id += (in_buffer[ind]-'0');
		ind++;
	}
	while(isspace(in_buffer[ind]) && in_buffer[ind] != '\0')
		ind++;
	printf("%c\n", in_buffer[ind]);
	int client_pid = 0;
	while(in_buffer[ind] != '\0' && !isspace(in_buffer[ind]))
	{
		client_pid *= 10;
		client_pid += (in_buffer[ind]-'0');
		ind++;
	}
	printf(">> Adding %d to group id %d...\n", client_pid, group_id);
	if(group_id < 0 || client_pid == 0)
	{
		strcpy(out_buffer, "Couldn't join group. Make sure entered group id is non negative!");
		return 0;
	}
	for(ind=0;ind<MAX_USERS;++ind)
	{
		if(user_lookup[ind] == client_pid)
			break;
	}
	if(ind == MAX_USERS)
	{
		for(ind=0;ind<MAX_USERS;++ind)
		if(user_lookup[ind] == 0)
		{
			user_lookup[ind] = client_pid;
			break;
		}
	}
	if(ind == MAX_USERS)
	{
		strcpy(out_buffer, "Couldn't join group. User limit exceeded!");
		return 0;
	}
	lookup_matrix[group_id][ind] = 1;
	strcpy(out_buffer, "Joined group!");
	if(send_client_queue(client_pid) == 1)
		return 1;
	return 0;
}

int message()
{
	printf(">> Message recieved: ");
	puts(in_buffer);
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
		return 1;
	return 2;
}

void exit_handler(int sig_no)
{
	mq_close(server);
	mq_unlink(SERVER);
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

	while(1)
	{
		memset(in_buffer, '\0', sizeof(in_buffer));
		read_server_queue();
		int flag; // type of message
		memset(out_buffer, '\0', sizeof(out_buffer));
		if((flag = message()) == 1) // not a request
		{
			int group_no = 0, i = 1,j=0;
			while(!isspace(in_buffer[i]) && in_buffer[i] != '\0')
			{
				group_no *= 10;
				group_no += (in_buffer[i]-'0');
				i++;
			}
			while(isspace(in_buffer[i]) != 0 && in_buffer[i] != '\0')
				i++;
			while(in_buffer[i]!='\0')
			{
				out_buffer[j] = in_buffer[i];
				i++;
				j++;
			}
			int pow = 1, pid = 0; j = strlen(out_buffer)-1;
			while(!isspace(out_buffer[j]))
			{
				pid += pow*(out_buffer[j]-'0');
				pow*=10;
				j--;
			}
			printf(">> Relaying message to group %d ...\n", group_no);
			int legal = -1;
			for(int i=0;i<MAX_USERS;++i)
			{
				if(user_lookup[i] == pid)
				{
					if(lookup_matrix[group_no][i]==0)
					{
						legal = 0;
						break;
					}
					else
						legal = 1;
				}
			}
			if(legal != 1)
			{
				strcpy(out_buffer, "Join group before posting!");
				send_client_queue(pid);
				continue;
			}
			for(int i=0;i<MAX_USERS;++i)
			{
				if(lookup_matrix[group_no][i] == 1 && user_lookup[i] != pid)
				{
					send_client_queue(user_lookup[i]);
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
