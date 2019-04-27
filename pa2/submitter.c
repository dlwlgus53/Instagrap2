#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>//for getopt
#include  <ctype.h>
int
main (int argc, char **argv)
{
    struct sockaddr_in serv_addr;
    int sock_fd ;
    int s, len ;
    char buffer[1024] = {0};
    char * data ;
    char c;
    
    char ip[126] = {0};
    int port=0;
    char id[10] = {0};
    char password[10] = {0};
    char file[126] = {0};
    //option getting start
    while ((c = getopt (argc, argv, "n:u:k:")) != -1){
        switch (c)
        {
            case 'n'://get port and ip
                strcpy(ip,strtok(argv[2], ":"));
                port = atoi(strtok(NULL, ":"));
                break;
            case 'u'://get user id
                strcpy(id, argv[4]);
                break;
            case 'k'://get password
                strcpy(password, argv[6]);
                break;
                
        }
    }
    //get file name
    strcpy(file, argv[7]);
    //option getting end
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
    if (sock_fd <= 0) {
        perror("socket failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip , &serv_addr.sin_addr) <= 0) {
        perror("inet_pton failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed : ") ;
        exit(EXIT_FAILURE) ;
    }
    
    //scanf("%s", buffer) ;
    FILE *fp = fopen(file, "r");
    
    if (fp == NULL){
        printf("Error opening file\n");
        exit(1);
    }
    
    while( fgets(buffer, 1024, fp) != NULL) {
        data = buffer ;
        len = strlen(buffer) ;
        s = 0 ;
        while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
            data += s ;
            len -= s ;
        }
        
    }
    
    fclose(fp);
    
    shutdown(sock_fd, SHUT_WR) ;
    
    char buf[1024] ;
    data = 0x0 ;
    len = 0 ;
    while ( (s = recv(sock_fd, buf, 1023, 0)) > 0 ) {
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
    printf(">%s\n", data);
    
}

