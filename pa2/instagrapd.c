#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

void
child_proc(int conn)
{
    struct sockaddr_in serv_addr;
    int sock_fd ;
    char buffer[1024] ;
    char * data = 0x0, * code = 0x0 ;
    int len = 0 ;
    int s ;
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
    if (sock_fd <= 0) {
        perror("socket failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8564);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed : ") ;
        exit(EXIT_FAILURE) ;
    }
   
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
    while ( (s = recv(sock_fd, buffer, 1023, 0)) > 0 ) {
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

void
child_proc2(int conn)
{
    struct sockaddr_in serv_addr;
    int sock_fd ;
    char buffer[1024] ;
    char * data = 0x0, * code = 0x0 ;
    int len = 0 ;
    int s ;
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
    if (sock_fd <= 0) {
        perror("socket failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8564);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    //input file open
    FILE *fp = fopen("1.in", "r");
    
    if (fp == NULL){
        printf("Error opening file\n");
        exit(1);
    }
    
    //send input and wait output
    printf("send input and wait output start\n");
    while( fgets(buffer, 1024, fp) != NULL) {
        data = buffer ;
        len = strlen(buffer) ;
        s = 0 ;
    }
    
    //input value with code
    strcat(data, "1");
    printf("input :\n %s", data);
    len++;
    while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
        data += s ;
        len -= s ;
    }
    shutdown(sock_fd, SHUT_WR) ;
    fclose(fp);
    data = 0x0; len=0;
    while ( (s = recv(sock_fd, buffer, 1023, 0)) > 0 ) {
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
    printf("send input and wait output end\n");
    //send input and get output end
    
    //output
    printf(">%s\n", data) ;
    
    //start send to submitter
    printf("send output start\n");
    while (len > 0 && (s = send(conn, data, len, 0)) > 0) {
        data += s ;
        len -= s ;
    }
    shutdown(conn, SHUT_WR) ;
    printf("send output end\n");
    //end send to submitter
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
    address.sin_port = htons(8765);
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
            child_proc2(new_socket) ;
        }
        else {
            close(new_socket) ;
        }
    }
}


