#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

int n,k,l,m,i,z,level,sigcnt,pid,done;
FILE *fptr;

int get_rand_pid()
{
    int id;
    FILE* tmp_ptr = fopen("process_list.txt","r");
    int ind = (rand()%(n*k));
    for(int j=0;j<=ind;++j)
    {
        fscanf(tmp_ptr, "%d", &id);
    }
    close(tmp_ptr);
    return id;
}

int get_rand_sig()
{
    return (1 + (rand()%31));
}

void ACK(int signum)
{
    if(level == 0 || level == 1) signum++;
    else if(level == 2 && done) signum++;
    if(!done)
        done = 1;
}

int main()
{
    fptr = fopen("process_list.txt","a");
    srand(time(0));
    scanf("%d %d %d %d", &n, &k, &l, &m);
    // printf("Grand-parent:%d\n",getpid());
    for(int j=1;j<32;++j)
    {
        if(j == 9 || j == 17)
            continue;
        signal(j, ACK);
    }
    level = 0;
    sigcnt = 0;
    done = 0;
    for(i=0;i<n;++i)
    {
        pid = fork();
        if(pid == 0)
        {
            // printf("Parent:%d\n",getpid());
            level = 1;
            for(z=0;z<k;++z)
            {
                pid = fork();
                if(pid == 0)
                {
                    // printf("Child:%d\n",getpid());
                    fprintf(fptr, "%d\n", getpid());
                    level = 2;
                    break;
                }
            }
            break;
        }
    }
    if(level == 2 && i == n-1 && z == k-1)
    {
        close(fptr);
        FILE* tmp_ptr = fopen("process_list.txt","r");
        for(int j=0;j<n*k;++j)
        {
            fscanf(tmp_ptr, "%d", &pid);
            // printf("%d\n", pid);
            kill(pid, SIGUSR1);
        }
        close(tmp_ptr);
    }
    
    if(level == 0)
    {
        while(sigcnt < n) 
        {
            wait(NULL);
            sigcnt ++;
        }
        // printf("Process %d exiting because all it's children exited.\n", getpid());
        exit(0);
    }
    if(level == 1)
    {
        while(sigcnt < k) 
        {
            wait(NULL);
            sigcnt ++;
        }
        // printf("Process %d exiting because all it's children exited.\n", getpid()); 
        exit(0);
    }
    if(level == 2)
    {
        int tmp;
        while(!done) pause();
        do
        {
            for(int j=0;j<m;++j)
            {
                int id = get_rand_pid();
                int sig = get_rand_sig();
                if(sig == 9 || sig == 17 || (kill(id,0) == 0))
                {
                    --j;
                    // printf("PID: %d and SIGNAL: %d is illegal. Continuing ... \n", id, sig);
                    continue;
                }
                // printf("PID: %d and SIGNAL: %d is legal. DONE\n", id, sig);
                kill(id,sig);
            }
        } while(tmp = sigcnt >= l);
        printf("Process %d recieved %d number of signals. So terminating.\n", getpid(), tmp);
        exit(0);
    }
    return 0;
}
