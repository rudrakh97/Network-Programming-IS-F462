#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MSGSIZE 500
#define ARGSSIZE 21
#define charcnt 15
#define wordcnt 20
#define linecnt 5


#define processMax 20
#define mpMAX 10000

int pipeCount;
int processCount;
int fd[processMax-1][2];

char *EXIT = "exit";
char *sc[] = {"sc", "-i", "-d", "-f", "-a", "-e"};
char *arr[] = {"|", "<", ">", ">>", "||", "|||", "&"};
char *comma = ",";
char *mp_output;

char *flagIO[linecnt][3];
char *args[linecnt][ARGSSIZE];
char  sc_lookup[linecnt][wordcnt][charcnt];

int restart=0, executeSC = 0, cmdcnt=0, cmdno=-1, multi_pipe=0, isbg=0;
int file_input = -1, file_output = -1;

char *msg, *tok, *rest;
int i,j,idx,cs,chk,a,b,c;
//--------------------------------------------------

void fd_init(){
	for(int i=0; i<processMax-1; ++i){
		fd[i][0] = 0;
		fd[i][1] = 0;
	}
}

void sc_lookup_init(){
	for(int i=0; i<linecnt; ++i){
		for(int j=0; j<wordcnt; ++j){
			for(int k=0; k<charcnt; ++k){
				sc_lookup[i][j][k] = '\0';
			}
		}
	}
}

void args_init(){
	for(int i=0; i<linecnt; ++i){
		for(int j=0; j<ARGSSIZE; ++j){
			args[i][j]=NULL;
		}
	}
}

void flagIO_init(){
	for(int i=0; i<linecnt; ++i){
		for(int j=0; j<ARGSSIZE; ++j){
			flagIO[i][j]=NULL;
		}
	}
}

void print_args_flagIO(){
	for(int i=0; i<linecnt; ++i){
		printf("args[%d]: ", i);
		for(int j=0; j<ARGSSIZE; ++j){
			printf("%s ", args[i][j]);
		}
		printf("\n");
		for(int j=0; j<3; ++j){
			printf("flagIO[%d][%d]:%s\n",i,j,flagIO[i][j]);
		}
		printf("\n");
	}
}

//---------------------------------------------------

int shortcut_mode(){

	if(strcmp(tok, sc[0])!=0) return -1; // not valid

	if( (tok = strtok_r(rest, " \n", &rest)) == NULL){
		perror("sc token error");
		exit(0);
	}

	i=0, cs=0, chk=0;
	a=0, b=0, c=0;

	if(strcmp(tok, sc[1]) ==0) cs = 1; // -i
	if(strcmp(tok, sc[2]) ==0) cs = 2; // -d 
	if(strcmp(tok, sc[3]) ==0) cs = 3; // -f 

	if(strcmp(tok, sc[4]) ==0){ // -a
		for(a=0; a<linecnt; ++a){
			printf("%d <cmd>: ", a);
			for(b=0; b<wordcnt; ++b){
				for(c=0; c<charcnt; ++c){
					printf("%c", sc_lookup[a][b][c]);
				}
				printf(" ");
			}
			printf("\n");
		}
		memset(msg, '\0', sizeof(msg));
		printf(">$ ");
		return -2;
	}

	if(strcmp(tok, sc[5]) ==0) cs = 5; // -e

	if(cs == 0){
		perror("syntax error");
		exit(0);
	}

	if((tok = strtok_r(rest, " \n", &rest)) == NULL){
		perror("sc <index> error");
		exit(0);
	}

	while(tok[i] != '\0'){
		if( !( tok[i] >= '0' && tok[i] <= '9') ){
			chk = 1;
			break;
		}
		++i;
	}

	if(chk == 1){
		perror("<index> is not a number");
		exit(0);
	}

	idx = (int) strtol(tok, NULL, 10);

	switch(cs){
		case(1):{ // sc -i <index> <cmd>
			a = idx;
			for(b=0; b<wordcnt; ++b){
				for(c=0; c<charcnt; ++c){
					sc_lookup[a][b][c] = '\0';
				}
			}
			b = 0;
			while( b < wordcnt && (tok = strtok_r(rest, " \n", &rest)) != NULL){
				c = 0;
				while( c < charcnt && tok[c] != '\0'){
					sc_lookup[a][b][c] = tok[c];
					++c;
				}
				++b;
			}
			break;
		}
		case(2):{ // sc -d <index>
			a = idx;
			if( a < 0 || a >= linecnt) break;
			for(b=0; b<wordcnt; ++b){
				for(c=0; c<charcnt; ++c){
					sc_lookup[a][b][c] = '\0'; 
				}
			}
			break;
		}
		case(3):{ // sc -f <index>
			a = idx;
			if(a < 0 || a >= linecnt) break;
			printf("%d <cmd>: ", a);
			for(b=0; b<wordcnt; ++b){
				for(c=0; c<charcnt; ++c){
					printf("%c", sc_lookup[a][b][c]);
				}
				printf(" ");
			}
			printf("\n");
			break;
		}
		case(5):{ // sc -e <index>
			a = idx;
			if(a < 0 || a >= linecnt) break;
			executeSC = 1;
			memset(msg, ' ', sizeof(msg));
			for(i=0; i<MSGSIZE; ++i){
				msg[i] = ' ';
			}
			for(i=0, b=0; i<MSGSIZE && b < wordcnt; ++i, ++b){
				for(c=0; c < charcnt; ++c, ++i){
					msg[i] = sc_lookup[a][b][c];
				} 
			}
			for(i=0; i<MSGSIZE; ++i){
				if(msg[i]=='\0') msg[i] = ' ';
			}
			for(i=MSGSIZE-1; i>=0; --i){
				if(msg[i]==' ') msg[i] = '\0';
				else break;
			}
			break;
		}
	}

	if(executeSC==1) return -2;

	memset(msg, '\0', sizeof(msg));
	printf(">$ ");
	return -2;
}

void multipipe_controlblock(){
	cmdno = cmdcnt;
	++cmdcnt;
	tok = strtok_r(rest," \n",&rest); // sort
	j = 1;
	args[cmdno][0] = tok;
	tok = strtok_r(rest, " \n", &rest); // ,
	while(j<ARGSSIZE && tok!=NULL && strcmp(tok, comma)!=0){
		if(strcmp(tok,arr[2])==0){
			tok = strtok_r(rest, " \n", &rest);
			if(tok==NULL){perror("invalid token"); exit(0);}
			flagIO[cmdno][1] = tok; tok = strtok_r(rest, " \n", &rest);
			continue;
		}
		if(strcmp(tok,arr[3]) == 0){
		tok = strtok_r(rest, " \n", &rest);
		if(tok==NULL){perror("invalid token"); exit(0);}
		flagIO[cmdno][2] = tok; tok = strtok_r(rest, " \n", &rest);
		continue;
		}
		args[cmdno][j] = tok;
		++j;
		tok = strtok_r(rest, " \n", &rest);
	}
}

void input_redirection_controlblock(int pno){
	if(flagIO[pno][0]!=NULL){
		if((file_input = open(flagIO[pno][0], O_RDONLY, S_IRWXU))<0){perror("< op"); exit(0);}
		dup2(file_input,0);
		close(file_input);
	}
}

void output_redirection_controlblock(int pno){
	if(flagIO[pno][1]!=NULL){
		if((file_output = open(flagIO[pno][1], O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU))<0){perror("> op"); exit(0);}
		dup2(file_output,1);
		close(file_output);
	}
}

void append_redirection_controlblock(int pno){
	if(flagIO[pno][2]!=NULL){
		if((file_output = open(flagIO[pno][2], O_CREAT | O_WRONLY | O_APPEND, S_IRWXU))<0){perror(">> op"); exit(0);}
		dup2(file_output,1);
		close(file_output);
	}
}

void handler()
{
	printf("Termination signal sent to child\n");
}

//----------------------------------------------------

int main(){

	sc_lookup_init();
	args_init();
	flagIO_init();

	signal(SIGINT, handler);

	msg = (char*) malloc(MSGSIZE * sizeof(char));
	memset(msg, '\0', sizeof(msg));
	printf(">$ ");

	while(1){
		if(executeSC==0){
			args_init();
			flagIO_init();
			while(fgets(msg,MSGSIZE,stdin)==NULL);
		}

		fd_init();
		restart=0, executeSC = 0, cmdcnt=0, cmdno=-1, multi_pipe=0;
		
		rest = msg;
		tok = strtok_r(rest, " \n", &rest);
		
		if(strcmp(tok, EXIT)== 0) exit(0);
		if(shortcut_mode()==-2) continue;

		while(1){
			cmdno = cmdcnt;
			++cmdcnt;
			for(int i=0; i<ARGSSIZE; ++i) args[cmdno][i]=NULL;
			for(int i=0; i<3; ++i) flagIO[cmdno][i] = NULL;
			
			j = 1;
			file_input = -1;
			file_output = -1;
			args[cmdno][0] = tok;
			
			while(j<ARGSSIZE && (tok=strtok_r(rest," \n",&rest))!=NULL){
				if(strcmp(tok,arr[0]) == 0) break;
				if(strcmp(tok,arr[1]) == 0){
					tok = strtok_r(rest, " \n", &rest);
					if(tok==NULL){perror("invalid token"); exit(0);}
					flagIO[cmdno][0] = tok;
					continue;
				}
				if(strcmp(tok,arr[2]) == 0){
					tok = strtok_r(rest, " \n", &rest);
					if(tok==NULL){perror("invalid token"); exit(0);}
					flagIO[cmdno][1] = tok;
					continue;
				}
				if(strcmp(tok,arr[3]) == 0){
					tok = strtok_r(rest, " \n", &rest);
					if(tok==NULL){perror("invalid token"); exit(0);}
					flagIO[cmdno][2] = tok;
					continue;
				}
				if(strcmp(tok,arr[4]) == 0){ // ls || sort , wc
					multi_pipe = 2;
					for(int z=0; z<2; ++z) multipipe_controlblock();
					break;
				}
				if(strcmp(tok,arr[5]) == 0){ // ls ||| sort , wc , wc
					multi_pipe = 3;
					for(int z=0; z<3; ++z) multipipe_controlblock();
					break;
				}
				if(strcmp(tok,arr[6]) == 0){
					isbg = 1;
					break;
				}
				args[cmdno][j] = tok;
				++j;
			}
			if(tok==NULL) break;
			if(j==ARGSSIZE && strcmp(tok, arr[0])!=0){
				printf("out-of-bounds:args[%d][]\n",cmdno);
				restart = 1;
				break;
			}
			if((tok = strtok_r(rest," \n",&rest))==NULL) break;
		}
		
		if(restart==1) continue;
	
		processCount = cmdcnt;

		if(multi_pipe==0){
			pipeCount = processCount - 1;
		} else{
			pipeCount = processCount - multi_pipe;
			processCount -= multi_pipe;
		}

		for(int i=0; i<pipeCount; ++i){
			pipe(fd[i]);
		}

		for(int i=0; i<processCount; ++i){
			if(fork()==0){

				signal(SIGINT, SIG_DFL);

				input_redirection_controlblock(i);
				output_redirection_controlblock(i);
				append_redirection_controlblock(i);

				if(i==0){
					if(flagIO[i][1]==NULL && flagIO[i][2]==NULL)dup2(fd[0][1], 1);
				} else if(i==processCount-1){
					if(flagIO[i][0]==NULL)dup2(fd[i-1][0], 0);
					if(multi_pipe > 0)dup2(fd[i][1], 1);
					else
					{
						// SET GROUP
					}
				} else {
					if(flagIO[i][0]==NULL)dup2(fd[i-1][0], 0);
					if(flagIO[i][1]==NULL && flagIO[i][2]==NULL)dup2(fd[i][1], 1);
				}
				
				for(int j=0; j<pipeCount; ++j){
					close(fd[j][0]);
					close(fd[j][1]);
				}
				
				execvp(args[i][0], args[i]);
			}
		}

		if(multi_pipe==0){
			for(int i=0; i<pipeCount; ++i){
				close(fd[i][0]);
				close(fd[i][1]);
			}
		}else{
			for(int i=0; i<=pipeCount-2; ++i){
				close(fd[i][0]);
				close(fd[i][1]);
			}
			
			for(int y=0, z; y<=pipeCount-2; ++y){
				wait(&z);
			}
			
			mp_output = malloc(sizeof(char) * mpMAX);
			
			read(fd[pipeCount-1][0], mp_output, mpMAX);
			close(fd[pipeCount-1][0]); 
			close(fd[pipeCount-1][1]);
			
			for(int z=0; z<multi_pipe; ++z){
				pipe(fd[pipeCount + z]);
				write(fd[pipeCount + z][1], mp_output, mpMAX);
			}
			
			free(mp_output);
			
			for(int z=0; z<multi_pipe; ++z){
				if(fork()==0){
					// SET GROUP
					dup2(fd[pipeCount + z][0], 0);
					output_redirection_controlblock(pipeCount + z);
					append_redirection_controlblock(pipeCount + z);
					
					for(int y=0; y<multi_pipe; ++y){
						close(fd[pipeCount + y][0]);
						close(fd[pipeCount + y][1]);
					}
					
					execvp(args[pipeCount + z][0], args[pipeCount + z]);
				}
			}
			
			for(int z=0; z<multi_pipe; ++z){
				close(fd[pipeCount + z][0]);
				close(fd[pipeCount + z][1]);
			}
		}

		while(wait(NULL) > 0);
		memset(msg, '\0', sizeof(msg));
		printf(">$ ");
	}

	return 0;
}