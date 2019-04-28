#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <sched.h>


typedef struct Threda_data {
    int conn;
    char filename[128];
}Thread_data;


void timeover(int sig)
{
    pid_t pid;
    printf("kill\n");
    kill(pid,SIGKILL);
    
}

void *compile( void *thread_data )
{
    Thread_data *data = (Thread_data *)thread_data;
    int input_fd;
    int output_fd;
    
    int conn = data->conn;
    char *filename = data->filename;
    char infilepath[128];
    char outfilepath[128];
    char message[128];
    
    
    sprintf(infilepath, "%s.in",filename);
    sprintf(outfilepath, "%s.out",filename);
    FILE* output_fp = fopen(outfilepath, "w"); fclose(output_fp);
    char * input;
    
    
    input_fd = open(infilepath, O_RDONLY | O_CREAT, 0644) ;
    output_fd = open(outfilepath, O_WRONLY | O_CREAT, 0644) ;
    
    dup2(input_fd,STDIN_FILENO);//file->stdin
    dup2(output_fd,STDOUT_FILENO);//stdout->file
    
    sprintf(message, "./%s",filename);
    system(message);
    
    

    close(input_fd);//write data to file
    close(output_fd);//save
    
    fclose(output_fp);
    // result send end
    shutdown(conn, SHUT_WR) ;
    
    char buffer[1024] ;
    output_fp = fopen(outfilepath ,"r");
    char * output = 0x0;
    int len=0;
    int s=0;
    
    while( fgets(buffer, 1024, output_fp) != NULL) {
        output = buffer ;
        len = strlen(buffer) ;
        s = 0 ;
    }
    
    while (len > 0 && (s = send(conn, output, len, 0)) > 0) {
        output += s ;
        len -= s ;
    }
    
    sprintf(message, "rm %s %s.c %s.in %s.out",filename,filename,filename,filename);
    system(message);

}

void *watch( void *thread_data  ){
    Thread_data *data = (Thread_data *)thread_data;
    
    int conn = data->conn;
    char message[128];
    char *filename = data->filename;
    signal(SIGALRM, timeover);
    
    alarm(3);
    printf("kill?");
    char * output = 0x0;
    char buffer[128] ="timeout" ;
    output = buffer;
    int len=strlen(output);
    int s=0;
   
    
    while (len > 0 && (s = send(conn, output, len, 0)) > 0) {
        output += s ;
        len -= s ;
    }
    
    //fclose(output_fp);
    // result send end
    shutdown(conn, SHUT_WR) ;
    
    sprintf(message, "rm %s %s.c %s.in %s.out",filename,filename,filename,filename);
    system(message);
    
}



void child_proc(int conn)
{
	char buf[1024] ;
	char * data = 0x0, * orig = 0x0 ;
	int len = 0 ;
	int s ;
	int fd_o;
	int fd_e;
	char mode ;
    char filename[64];
    char * token1;
    char * token2;

	while ( (s = recv(conn, buf, 1023, 0)) > 0 ) {
		buf[s] = 0x0 ;
		if (data == 0x0) {
			data = strdup(buf) ;
			len = s ;
		}
		else {
			data = realloc(data, len + s + 1) ;
			strncpy(data + len, buf, s) ;
			data[len + s] = 0x0 ;
			len += s ;
		}

	}
    token1 = strtok(data, " ");
    mode = token1[0];
    token2 = strtok(NULL, " ");
    strcpy(filename, token2);
    printf("mode : %c, filename : %s\n", mode, filename);
    
    data = data + strlen(token1) + strlen(token2) + 2;
    len = len - strlen(token1) - strlen(token2) - 2;
    
    printf("%s\n", data);

	if (mode == '0'){
        char codefilepath[128];
        char errfilepath[128];
		char buffer[1024];
        
        sprintf(codefilepath, "%s.c",filename);
        sprintf(errfilepath, "%s.err",filename);

		FILE *fp;
		fp = fopen(codefilepath, "w");

		fputs(data, fp);
		fclose(fp);

		//fd_o = open("file.out", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
		fd_e = open(errfilepath, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

		//dup2(fd_o, 1);
		dup2(fd_e, 2);

		//close(fd_o);
		close(fd_e);
        char message[128];
        sprintf(message, "gcc %s -o %s",codefilepath,filename);
		system(message);
		//execl("gcc", "gcc", "file.c", (char*)0x0);

        
		fp = fopen(errfilepath, "r");
		if ( fgets(buffer, 1024, fp) == NULL) {
			printf("No Error!\n");
			data = 0x0;
			data = strdup("No Error!");
			len = strlen("No Error!");
			s = 0;
			while (len > 0 && (s = send(conn, data, len, 0)) > 0) {
				data += s ;
				len -= s ;
			}
			shutdown(conn, SHUT_WR) ;
			fclose(fp);
		} else {	
			fclose(fp);
			fp = fopen(errfilepath, "r");
			while( fgets(buffer, 1024, fp) != NULL) {
				printf("%s\n", buffer);
				data = buffer ;
				len = strlen(buffer) ;
				s = 0 ;
				while (len > 0 && (s = send(conn, data, len, 0)) > 0) {
					data += s ;
					len -= s ;
				}
			}	
			fclose(fp);

			shutdown(conn, SHUT_WR) ;
		}
        sprintf(message, "rm %s",errfilepath);
        system(message);
	} else if (mode == '1'){
        
        char infilepath[128];
        sprintf(infilepath, "%s.in",filename);
        char* input= 0x0;
        
        FILE* input_fp = fopen(infilepath, "w");
        
        input = data;//copy data to input
        
        fputs(input, input_fp);//input data in 1_1.in
        fclose(input_fp);
        
        Thread_data *thread_data;
        
        thread_data = (Thread_data *)malloc(sizeof(Thread_data));
        
        thread_data->conn = conn;
        strcat(thread_data->filename,filename);
        
        pthread_t thread1, thread2;

        pthread_create( &thread1, NULL, compile, (void*) thread_data);
        pthread_create( &thread2, NULL, watch, (void*) thread_data);

        pthread_join( thread1, NULL);
        pthread_join( thread2, NULL);

        

        

        
        
    }
}

int
main (int argc, char **argv)
{ 
	int listen_fd, new_socket ; 
	struct sockaddr_in address; 
	int opt = 1;
    int port =0;
    char c;
	int addrlen = sizeof(address);
    while ((c = getopt (argc, argv, "p")) != -1){
        switch (c)
        {
            case 'p'://get port and ip
                port = atoi(argv[2]);
                break;
        }
    }

	char buffer[1024] = {0}; 

	listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/) ;
	if (listen_fd == 0)  { 
		perror("socket failed : "); 
		exit(EXIT_FAILURE); 
	}
	
	memset(&address, '0', sizeof(address)); 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY /* the localhost*/ ; 
	address.sin_port = htons(port);
	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed : "); 
		exit(EXIT_FAILURE); 
	} 

	while (1) {
		if (listen(listen_fd, 16 /* the size of waiting queue*/) < 0) { 
			perror("listen failed : "); 
			exit(EXIT_FAILURE); 
		} 

		new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
		if (new_socket < 0) {
			perror("accept"); 
			exit(EXIT_FAILURE); 
		} 

		if (fork() > 0) {
			child_proc(new_socket) ;
		}
		else {
			close(new_socket) ;
		}
	}
} 

