#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define MAXPLAYER 100

int sockfd;
int fds[MAXPLAYER];
short PORT = 8800;
typedef struct sockaddr SA;
char *account[120];

void init()
{
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    printf("[System] Creating socket...\n");
    if(sockfd == -1) {
        perror("Create Socket Failed");
        exit(-1);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (SA *)&addr, sizeof(addr)) == -1){
		perror("Bind Failed");
		exit(-1);
	}
    if (listen(sockfd, MAXPLAYER) == -1){
		perror("Listen Failed");
		exit(-1);
	}

    for(int i=0; i<120; i++) {
        account[i] = malloc(sizeof(char) * 30);
    }
}

int authen(char *buf)
{
    FILE *fp;
    char tmp[100];
    fp = fopen("shadow", "r");
    while((fscanf(fp, "%s", tmp)) != EOF) {
        if(strcmp(buf, tmp) == 0) {
            return 1;
        }
    }
    return 0;
}

void chat(char *msg)
{
    for(int i=0; i<MAXPLAYER; i++) {
        if(fds[i] != 0) {
            printf("[System] Service send the message to fd: %d\n", fds[i]);
            send(fds[i], msg, strlen(msg), 0);
        }
    }
}

void getAlluser(int fd)
{
    char line[100] = "\n---------------------\n";
    printf("[Command] ls by fd: %d \n", fd);
    send(fd, line, strlen(line), 0);
    send(fd, "[Online User List]\n", strlen("[Online User List]\n"), 0);
    for(int i=0; i<MAXPLAYER; i++) {
        if(fds[i] != 0) {
            char buf[100];
            if(fds[i] != fd) {
                sprintf(buf, "[User] %s, [fd]: %d\n", account[fds[i]], fds[i]);
                send(fd, buf, strlen(buf), 0);
            }
        }
    }
    send(fd, line, strlen(line), 0);
    char buf[100] = {};
    strcpy(buf, "Please choose your opponent: (enter @fd)");
    send(fd, buf, strlen(buf), 0);
}

void *service_thread(void *p)
{
    int fd = *(int *)p, check;
    char *ptr, buf[100] = {};
    printf("pthread = %d\n", fd);

    /* Login */
    while(1) {
        send(fd, "authenticate", strlen("authenticate"), 0);
        recv(fd, buf, sizeof(buf), 0); // receive the username and password from client
        //printf("buf = %s\n", buf); // For debugging
        ptr = strstr(buf, ":");
        *ptr = '\0';
        strcpy(account[fd], buf);
        account[fd][strlen(buf)] = '\0';
        *ptr = ':';
        ptr = strstr(buf, "@");
        *ptr = '\0';
        
        if((check = authen(buf)) == 1) {
            printf("[System] New account: %s\n", account[fd]);
            break;
        }
        else {
            printf("[System] Login Failed...\n");
            pthread_kill(fd, SIGALRM);
        }
    }

    while(1) {
        char buf2[100] = {};

        /* 用戶退出房間 */
        if(recv(fd, buf2, sizeof(buf2), 0) <= 0) {
            int i;
            for(i=0; i<MAXPLAYER; i++) {
                if(fds[i] == fd) {
                    fds[i] = 0;
                    break;
                }
            }
            printf("fd: %d exit the room!\n", fd);
            pthread_exit((void *)i);
        }

        /* 指令 */
        int oppofd;
        char msg[1024], tmp[1024];
        if(strcmp(buf2, "ls") == 0) {
            getAlluser(fd);
        }
        else if(buf2[0] == '@') {
            oppofd = atoi(&buf2[1]);
            sprintf(msg, "CONNECT %s %d", account[fd], fd);
            send(oppofd, msg, strlen(msg), 0);
            sprintf(tmp, "Wait for your opponent's response...\n"); 
            send(fd, tmp, strlen(tmp), 0);
        }
        else if(strncmp(buf2, "AGREE ", 6) == 0) {
            oppofd = atoi(&buf2[6]);
            sprintf(msg, "AGREE %s %d", account[fd], fd);
            printf("msg: %s, length: %ld, oppofd: %d\n", msg, strlen(msg), oppofd);
            send(oppofd, msg, strlen(msg), 0);
        }
        else if(buf2[0] == '#') {
            int pos = atoi(&buf2[1]);
            char *ptr;
            ptr = strstr(buf2, " ");
            ptr++;
            oppofd = atoi(ptr);
            sprintf(tmp, "#%d", pos);
            printf("tmp: %s, length: %ld, oppofd: %d\n", tmp, strlen(tmp), oppofd);
            send(oppofd, tmp, strlen(tmp), 0);
        }
        else {
            chat(buf2);
        }
        memset(buf2, 0, sizeof(buf2));
    }
}


int main(int argc, char **argv) 
{
    init();
    printf("[System] Server Initialization Success...\n");
    while(1) {
        struct sockaddr_in fromaddr;
        socklen_t len = sizeof(fromaddr);
        int fd = accept(sockfd, (SA *)&fromaddr, &len);
        if(fd == -1) {
            printf("[System] Client Connection Failed...\n");
            continue;
        }

        int i = 0;
        for(i=0; i<MAXPLAYER; i++) {
            if(fds[i] == 0) {
                fds[i] = fd;
                pthread_t tid;
                pthread_create(&tid, 0, service_thread, &fd);
                break;
            }
        }
        if(i == MAXPLAYER) {
            char *str = "The room is full!!";
            send(fd, str, strlen(str), 0);
            close(fd);
        }
    }
}