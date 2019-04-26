#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <netinet/in.h> 
#include <string.h> 
#include <fcntl.h> // for open
#include <sys/fcntl.h> // for open
#include <unistd.h> // for close
#include <sys/stat.h>

void
child_proc(int conn)
{
	char buf[1024] ;
	char * data = 0x0, * code = 0x0, * input=0x0;
	int len = 0 ;
	int s ;
	char mode ;
	FILE *fp;

	while ( (s = recv(conn, buf, 1023, 0)) > 0 ) {//receive something
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
    printf("%s\n", data);
    printf("len %d\n", len);
    mode= data[len-1];
    data[len-1] = ' ';
	printf("mode : %c\n", mode);
	//mode bit 이랑 code는 shutdown 하지말고 기다리게 하자..
    code = data;
    while (len > 0 && (s = send(conn, data, len, 0)) > 0) {
        data += s ;
        len -= s ;
    }
    shutdown(conn, SHUT_WR) ;
    
    
    data = 0x0;
    len=0;
    
	if(mode=='0'){//case of code
        printf("receive code and gcc start\n");
//        while ( (s = recv(conn, buf, 1023, 0)) > 0 ) {//recieve code
//            buf[s] = 0x0 ;
//            if (data == 0x0) {
//                data = strdup(buf) ;
//                len = s ;
//            }
//            else {
//                data = realloc(data, len + s + 1) ;
//                strncpy(data + len, buf, s) ;
//                data[len + s] = 0x0;
//                len += s ;
//            }
//        }
        printf("code : %s\n",code);
      
        
		FILE *fp;
		fp = fopen("file.c", "w");
        if (fp == NULL){
            printf("Error opening file\n");
            exit(1);
        }

		fputs(code, fp);
		fclose(fp);

		system("gcc file.c");
        printf("receive code and gcc end\n");
        
        while (len > 0 && (s = send(conn, data, len, 0)) > 0) {//send for assure(?)
            data += s ;
            len -= s ;
        }
        shutdown(conn, SHUT_WR) ;


	}
	else if(mode=='1'){//receive input file
        printf("receive input and exe start and send\n");
        printf("    receive input\n");
		FILE* input_fp = fopen("1.in", "w");
        FILE* output_fp = fopen("1.out", "w"); fclose(output_fp);
        
		while ( (s = recv(conn, buf, 1023, 0)) > 0 ) {//recieve input file
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
        printf("    receive inputend\n");
        
        input=data;//copy data to input
        while (len > 0 && (s = send(conn, data, len, 0)) > 0) {//send for assure(?)
            data += s ;
            len -= s ;
        }
        shutdown(conn, SHUT_WR) ;
        
        
		fputs(data, input_fp);//input data in 1.in
		fclose(input_fp);
        printf("    exe start\n");
		int input_fd = open("1.in", O_RDONLY | O_CREAT, 0644) ;
		int output_fd = open("1.out", O_WRONLY | O_CREAT, 0644) ;

    	dup2(STDIN_FILENO,input_fd);//file->stdin
		dup2(output_fd,STDOUT_FILENO);//stdout->file
		system("./a.out");
		close(input_fd);//write data to file
		close(output_fd);//save
        printf("    exe end\n");
        printf("    result send start\n");
		char buffer[1024] ;
		output_fp = fopen("1.out" ,"r");
		while( fgets(buffer, 1024, output_fp) != NULL) {
			data = buffer ;
			len = strlen(buffer) ;
			s = 0 ;
			while (len > 0 && (s = send(conn, data, len, 0)) > 0) {
				data += s ;
				len -= s ;
			}
		}
		fclose(output_fp);
        printf("    result send end\n");
		shutdown(conn, SHUT_WR) ;
        printf("receive input and exe start and send end\n");


	}
    printf("precess finish\n");
	
}

int 
main(int argc, char const *argv[]) 
{ 
	int listen_fd, new_socket ; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 

	char buffer[1024] = {0}; 

	listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/) ;
	if (listen_fd == 0)  { 
		perror("socket failed : "); 
		exit(EXIT_FAILURE); 
	}
	
	memset(&address, '0', sizeof(address)); 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY /* the localhost*/ ; 
	address.sin_port = htons(8564); 
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

