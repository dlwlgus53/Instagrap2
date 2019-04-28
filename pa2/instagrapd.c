#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string.h>

char*
recev_code(int conn, char * w_ip, int w_port)
{

    char buffer[1024] ;
    char * data = 0x0, * code = 0x0 ;
    int len = 0 ;
    int s ;
    
    data = 0x0; len=0;
    printf("recev code start\n");
    while ( (s = recv(conn, buffer, 1023, 0)) > 0 ) {//receive code from submitter
        buffer[s] = 0x0 ;
        if (data == 0x0) {
            data = strdup(buffer) ;
            len = s ;
        }
        else {
            data = realloc(data, len + s + 1) ;
            strncpy(data + len, buffer, s) ;
            data[len + s] = 0x0 ;
            len += s ;
        }
    }
    code=data;
    len = strlen(data) ;
    printf("%s" ,data);
    
    printf("recev code end\n");
    //recv code end
    
    //make mode bit in code
    strcat(code, "0");
    len++;
    //make code end
    
    return code;
    
    
}

void
child_proc(int conn, char*code,char * w_ip, int w_port)//send code
{
    struct sockaddr_in serv_addr;
    int sock_fd ;
    char buffer[1024] ;
    char * data = 0x0;
    int len = 0 ;
    int s ;
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
    if (sock_fd <= 0) {
        perror("socket failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(w_port);
    if (inet_pton(AF_INET, w_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    //send code
    printf("send code start\n");
    printf("%s" ,code);
    len = strlen(code) ;
    while (len > 0 && (s = send(sock_fd, code, len, 0)) > 0) {//send code to worker
        code += s ;
        len -= s ;
    }
    shutdown(sock_fd, SHUT_WR) ;
    data = 0x0; len=0;
    while ((s = recv(sock_fd, buffer, 1023, 0)) > 0 ) {
        buffer[s] = 0x0 ;
        if (data == 0x0) {
            data = strdup(buffer) ;
            len = s ;
        }
        else {
            data = realloc(data, len + s + 1) ;
            strncpy(data + len, buffer, s) ;
            data[len + s] = 0x0 ;
            len += s ;
        }
        
    }
    printf("send code end\n");
    free(data);
}

int
child_proc2(int conn, char * w_ip, int w_port, char* folder, int n)
{
    struct sockaddr_in serv_addr;
    int sock_fd ;
    char buffer[1024], ans_buf[1024];
    char * data = 0x0,* output = 0x0 ;
    char path[128];
    int len = 0 ;
    int s ;
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
    if (sock_fd <= 0) {
        perror("socket failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(w_port);
    if (inet_pton(AF_INET, w_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    //input file open
    
    //make path

    sprintf(path,"./%s/%d.in",folder,n);
    FILE *fp = fopen(path, "r");
    if (fp == NULL){
        printf("Error opening .in file\n");
        exit(1);
    }
    data==NULL;
    while(data==NULL){
        while( fgets(buffer, 1024, fp) != NULL) {
            data = buffer ;
            len = strlen(buffer) ;
            s = 0 ;
        }
        if(buffer[0]==0x0)  continue;
    }
    //send input and wait output
    printf("send input and wait output start\n");
    //input value with code
    strcat(data, "1");
    printf("input : %s\n", data);
    len++;
    while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
        data += s ;
        len -= s ;
    }
    shutdown(sock_fd, SHUT_WR) ;
    fclose(fp);
    
    data = 0x0; len=0;
    s=0;
    buffer[0]=0x0;
    printf("wating for recv\n");
    while(data==NULL){
        while ((s = recv(sock_fd, buffer, 1023, 0)) > 0 ) {
            printf("?");
            if(buffer[0]==0x0)  continue;
            buffer[s] = 0x0 ;
            if (data == 0x0) {
                data = strdup(buffer) ;
                len = s ;
            }
            else {
                data = realloc(data, len + s + 1) ;
                strncpy(data + len, buffer, s) ;
                data[len + s] = 0x0 ;
                len += s ;
            }
        }
    }
    printf("send input and wait output end\n");

    //send input and get output end
    //test start
    sprintf(path,"./%s/%d.out",folder,n);
    fp = fopen(path, "r");
    if (fp == NULL){
        printf("Error opening .out file\n");
        exit(1);
    }
    //파일 읽기
    int size;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    
    output = malloc(size + 1);
    memset(output, 0, size + 1);
    
    fseek(fp, 0, SEEK_SET);
    fread(output, size, 1, fp);

    printf("worker output : %s\n", data) ;
    printf("file output :   %s\n", output) ;
    close(sock_fd) ;
    while(data==NULL);
    close(sock_fd) ;
    if(strcmp(data,output) == 0)
        return 1;
    else
        return 0;


}


void
child_proc3(int conn, int ans)
{

    char * data = 0x0;
    char buffer[128];
    int len = 0 ;
    int s ;
    data=buffer;
    
    sprintf(data, "%d", ans);
    len = strlen(data);
    //start send to submitter
    printf("send output start\n");
    
    while (len > 0 && (s = send(conn, data, len, 0)) > 0) {
        data += s ;
        len -= s ;
    }
    printf("send output end\n");
    //end send to submitter
    
    shutdown(conn, SHUT_WR) ;
}



int
main (int argc, char **argv)
{
    //this is for parameter
    char w_ip[126] = {0};
    int w_port=0;
    int port=0;
    char file[126] = {0};
    char c;
    
    //this is for code
    int listen_fd, new_socket ;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    
    //get parameter by opt
    while ((c = getopt (argc, argv, "p:w:")) != -1){
        switch (c){
            case 'p'://get port of instagrapd
                port = atoi(argv[2]);
                printf("port %d\n",port);
                break;
            case 'w'://get worker ip and port
                strcpy(w_ip,strtok(argv[4], ":"));
                w_port = atoi(strtok(NULL, ":"));
                printf("w_ip, w_port %s %d\n", w_ip, w_port);
                break;
                
        }
    }
    //get file name
    strcpy(file, argv[5]);
    //option getting end
    
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
            int ans=0;
            char * code;
            code = recev_code(new_socket,w_ip,w_port);//recev code
            for(int i=1;i<=4;i++){
                sleep(1);
                child_proc(new_socket,code,w_ip,w_port);//send code
                sleep(1);
                ans +=child_proc2(new_socket,w_ip,w_port,file,i);//send input
            }
            printf("end loop\n");
            child_proc3(new_socket,ans);//send back result to submitter
        }
        else {
            close(new_socket) ;
        }
    }
}



