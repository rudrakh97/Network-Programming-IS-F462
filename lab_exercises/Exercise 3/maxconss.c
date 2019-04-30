#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

// Client side won't allow more than 65535 different connections
#define MAX_TCP 66000

int count = 0, isparent = 1, break_loop = 0, counter = 0;
int child_pids[MAX_TCP];

void parent_count() // increases TCP connection count
{
    count++;
}

void handler() // Signals children to exit and breaks connection loop
{
    break_loop = 1;
    for(int i=0;i<MAX_TCP && child_pids[i]>0;++i)
        kill(SIGKILL, child_pids[i]);
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf("[-] Enter a URL to check. Exiting ...\n");
        exit(1);
    }
    if(argc > 2)
    {
        printf("[-] Program expected single argument. Found %d arguments. Exiting ...\n", argc-1);
        exit(1);
    }

    // Confirm URL
    char url[256];
    memset(url, '\0', sizeof(url));
    strncpy(url, argv[1], strlen(argv[1]));
    printf("[+] URL to be checked: %s\nPress [ENTER] to continue...", url);
    getchar();

    // Get IP address from hostname
    struct hostent *HostInfo;
    HostInfo = gethostbyname(url);
    if(!HostInfo)
    {
        printf("[-] Couldn't resolve host name\n");
        exit(1);
    }
    char ip[INET_ADDRSTRLEN];
    char **ptr = HostInfo->h_addr_list;
    printf("Address list: \n");
    while(*ptr != NULL)
    {
        printf("Aliases: %s\n",inet_ntop(HostInfo->h_addrtype, *ptr, ip, sizeof(ip)));
        ++ptr;
    }
    printf("[+] IP considered: %s\n",ip);

    signal(SIGUSR1, parent_count);
    signal(SIGUSR2, handler);

    int server_socket;
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(80);
    inet_pton(AF_INET, ip, &(server_address.sin_addr.s_addr));

    while(!break_loop)
    {
        int pid = fork();
        // Each child opens a new TCP connection
        if(pid == 0)
        {
            isparent = 0;
            server_socket = socket(AF_INET, SOCK_STREAM, 0);
            int connection_status = connect(server_socket, (struct sockaddr*)(&server_address), sizeof(server_address));
            if(connection_status != -1)
            {
                // increase counter in parent
                kill(getppid(), SIGUSR1);
                // keep connection alive till server limit reached
                wait(NULL);
                exit(0);
            }
            else
            {
                // Server limit reached. Send signal to parent to stop loop
                kill(getppid(), SIGUSR2);
                exit(0);
            }
        }
        else
        {
            // store child pids
            child_pids[counter] = pid;
            counter ++;
            wait(NULL);
        }
    }

    printf("[+] Total number of parellal TCP connections allowed: %d\n", count);
	return 0;
}
