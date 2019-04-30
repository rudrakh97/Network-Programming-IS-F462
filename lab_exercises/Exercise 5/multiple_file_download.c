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
#include <time.h>

// HTTP PORT
#define PORT 80

// FILENAME FOR UBUNTU ISO DOWNLOAD
#define FILENAME "ubuntu.iso"

// FILENAME FOR DOWNLOAD TEST
// #define FILENAME "file.pdf"

// PATH WITHOUT PROTOCOL TO BE GIVEN AS ARGUMENT:

// PATH FOR UBUNTU ISO HTTP DOWNLOAD:
// releases.ubuntu.com/18.04.2/ubuntu-18.04.2-desktop-amd64.iso

// PATH FOR HTTP DOWNLOAD TEST:
// www.axmag.com/download/pdfurl-guide.pdf

char message[1024], ip[INET_ADDRSTRLEN];

void request(int buffer_size)
{
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1)
    {
        perror("[-] Socket error");
        exit(1);
    }
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &(server_address.sin_addr.s_addr));

    int connection_status = connect(server_socket, (struct sockaddr*)(&server_address), sizeof(server_address));

    if(connection_status < 0)
    {
        perror("[-] Connection error");
        exit(1);
    }

    printf("[+] Connected to %s on remote port %d\n", inet_ntoa(server_address.sin_addr), (int) ntohs(server_address.sin_port));

    if( send(server_socket , message , strlen(message) , 0) < 0)
    {
        perror("[-] Request error");
        return;
    }

    puts("[+] Request sent ..."); 

    char server_reply[buffer_size];
    char *filename = FILENAME;
    int total_len = 0;
    int len = 0; 
    FILE *file = NULL;

    remove(filename);
    file = fopen(filename, "ab");

    if(file == NULL)
    {
        perror("[-] File could not opened ...\n");
        return;
    }   

    clock_t begin = clock();

    // printf("%d\n",sizeof(server_reply));

    while(1)
    {
        int received_len = recv(server_socket, server_reply , sizeof server_reply , 0);

        if( received_len < 0 ){
            perror("[-] Download failed");
            return;
        }

        if(len = 0)
            len = received_len;

        total_len += received_len;
  
        fwrite(server_reply , received_len , 1, file);

        // printf("[.] Received byte size = %d\tTotal length = %d\n", received_len, total_len); 

        if(received_len < len || received_len < buffer_size)
        {
            // puts(server_reply);
            break;
        }
    }
    clock_t end = clock();

    printf("[+] Download time with buffer size %d: %ld\n\n", buffer_size, (long double)(end-begin)/(long double)(CLOCKS_PER_SEC));

    fclose(file);
    close(server_socket);
}

int main(int argc , char *argv[])
{
    if(argc < 2)
    {
        printf("[-] Enter a URL to download. Exiting ...\n");
        exit(1);
    }
    if(argc > 2)
    {
        printf("[-] Program expected single argument. Found %d arguments. Exiting ...\n", argc-1);
        exit(1);
    }

    int index = 0;

    // Confirm URL
    char url[256], res[256];
    memset(url, '\0', sizeof(url));
    memset(res, '\0', sizeof(res));
    while(index < strlen(argv[1]) && argv[1][index] != '/')
        index ++;
    strncpy(url, argv[1], index);
    if(index < strlen(argv[1]))
    {
        for(int i=index; i<strlen(argv[1]); ++i)
            res[i-index] = argv[1][i];
    }
    else
        strcpy(res,"/");
    printf("[+] Root URL: %s\n[+] Resource path: %s\n[.] Press [ENTER] to continue...", url, res);
    getchar();

    // Get IP address from hostname
    struct hostent *HostInfo;
    HostInfo = gethostbyname(url);
    if(!HostInfo)
    {
        printf("[-] Couldn't resolve host name\n");
        exit(1);
    }
    char **ptr = HostInfo->h_addr_list;
    printf("Address list: \n");
    while(*ptr != NULL)
    {
        printf("Aliases: %s\n",inet_ntop(HostInfo->h_addrtype, *ptr, ip, sizeof(ip)));
        ++ptr;
    }
    printf("[+] IP considered: %s\n\n",ip);

    sprintf(message, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n Connection: keep-alive\r\n", res, url);

    // Requests with multiple buffer sizes
    request(10000);
    request(100);
    request(10);

    return 0;
}

