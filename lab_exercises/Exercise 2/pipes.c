#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define MAX 500

char buff[MAX], buff1[MAX];

int main()
{
    int fd1[2], fd2[2], fd3[2], fd4[2], fd5[2], fd6[2], level;
    if(pipe(fd1)==-1)
    {
        perror("Pipe 1 Error");
        exit(0);
    }
    if(pipe(fd2)==-1)
    {
        perror("Pipe 2 Error");
        exit(0);
    }
    if(pipe(fd3)==-1)
    {
        perror("Pipe 3 Error");
        exit(0);
    }
    if(pipe(fd4)==-1)
    {
        perror("Pipe 4 Error");
        exit(0);
    }
    if(pipe(fd5)==-1)
    {
        perror("Pipe 5 Error");
        exit(0);
    }
    if(pipe(fd6)==-1)
    {
        perror("Pipe 6 Error");
        exit(0);
    }
    level = 0;
    for(int i=0;i<5;++i)
    {
        int pid = fork();
        if(pid == -1)
        {
            perror("Fork error");
            exit(0);
        }
        if(pid == 0)
        {
            level = i+1;
            break;
        }
    }
    if(level==0)
    {
        close(fd1[0]);
        // close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        close(fd3[0]);
        close(fd3[1]);
        close(fd4[0]);
        close(fd4[1]);
        close(fd5[0]);
        close(fd5[1]);
        // close(fd6[0]);
        close(fd6[1]);
    }
    else if(level==1)
    {
        // close(fd1[0]);
        close(fd1[1]);
        close(fd2[0]);
        // close(fd2[1]);
        close(fd3[0]);
        close(fd3[1]);
        close(fd4[0]);
        close(fd4[1]);
        close(fd5[0]);
        close(fd5[1]);
        close(fd6[0]);
        close(fd6[1]);
    }
    else if(level==2)
    {
        close(fd1[0]);
        close(fd1[1]);
        // close(fd2[0]);
        close(fd2[1]);
        close(fd3[0]);
        // close(fd3[1]);
        close(fd4[0]);
        close(fd4[1]);
        close(fd5[0]);
        close(fd5[1]);
        close(fd6[0]);
        close(fd6[1]);
    }
    else if(level==3)
    {
        close(fd1[0]);
        close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        // close(fd3[0]);
        close(fd3[1]);
        close(fd4[0]);
        // close(fd4[1]);
        close(fd5[0]);
        close(fd5[1]);
        close(fd6[0]);
        close(fd6[1]);
    }
    else if(level==4)
    {
        close(fd1[0]);
        close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        close(fd3[0]);
        close(fd3[1]);
        // close(fd4[0]);
        close(fd4[1]);
        close(fd5[0]);
        // close(fd5[1]);
        close(fd6[0]);
        close(fd6[1]);
    }
    else if(level==5)
    {
        close(fd1[0]);
        close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        close(fd3[0]);
        close(fd3[1]);
        close(fd4[0]);
        close(fd4[1]);
        /// close(fd5[0]);
        close(fd5[1]);
        close(fd6[0]);
        // close(fd6[1]);
    }
    if(level == 0)
    {
        fgets(buff, MAX, stdin);
        write(fd1[1], buff, strlen(buff)-1);
        close(fd1[1]);
        int len2 = read(fd6[0], buff1, MAX);
        if(len2 < 0)
        {
            perror("Pipe read error");
            exit(0);
        }
        close(fd6[0]);
        buff1[len2] = '\0';
        puts(buff1);
    }
    else if(level == 1)
    {
        int len = read(fd1[0], buff, MAX);
        if(len < 0)
        {
            perror("Pipe read error");
            exit(0);
        }
        close(fd1[0]);
        buff[len] = '\0';
        
        int i=0;
        while(buff[i]!='\0')
        {
            if(buff[i] >= 'a' && buff[i] <= 'z')
                buff[i] += 'A' - 'a';
            i++;
        }
        printf("C%d pid:%d ",level,getpid());
        puts(buff);
        
        write(fd2[1], buff, strlen(buff));
        close(fd2[1]);
    }
    else if(level == 2)
    {
        int len = read(fd2[0], buff, MAX);
        if(len < 0)
        {
            perror("Pipe read error");
            exit(0);
        }
        close(fd2[0]);
        buff[len] = '\0';
        
        strncpy(buff1, buff+1, strlen(buff)-1);
        printf("C%d pid:%d ",level,getpid());
        puts(buff1);
        
        write(fd3[1], buff1, strlen(buff1));
        close(fd3[1]);
    }
    else if(level == 3)
    {
        int len = read(fd3[0], buff, MAX);
        if(len < 0)
        {
            perror("Pipe read error");
            exit(0);
        }
        close(fd3[0]);
        buff[len] = '\0';
        
        strncpy(buff1, buff, strlen(buff)-1);
        printf("C%d pid:%d ",level,getpid());
        puts(buff1);
        
        write(fd4[1], buff1, strlen(buff1));
        close(fd4[1]);
    }
    else if(level == 4)
    {
        int len = read(fd4[0], buff, MAX);
        if(len < 0)
        {
            perror("Pipe read error");
            exit(0);
        }
        close(fd4[0]);
        buff[len] = '\0';
        
        strncpy(buff1, buff+1, strlen(buff)-1);
        printf("C%d pid:%d ",level,getpid());
        puts(buff1);
        
        write(fd5[1], buff1, strlen(buff1));
        close(fd5[1]);
    }
    else if(level == 5)
    {
        int len = read(fd5[0], buff, MAX);
        if(len < 0)
        {
            perror("Pipe read error");
            exit(0);
        }
        close(fd5[0]);
        buff[len] = '\0';
        
        strncpy(buff1, buff, strlen(buff)-1);
        printf("C%d pid:%d ",level,getpid());
        puts(buff1);
        
        write(fd6[1], buff1, strlen(buff1));
        close(fd6[1]);
    }
    return 0;
}
