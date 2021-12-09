#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

int sockfd;
short PORT = 8800;
typedef struct sockaddr SA;
char account[1024], name[100], passwd[100], board[9], opponame[100];
char lable1='O', lable2='X';
int oppofd, isMe=0, game=0;

void init()
{
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (connect(sockfd, (SA *)&addr, sizeof(addr)) == -1){
		perror("Connect Failed");
		exit(-1);
	}
	printf("[System] Client start Successfully !\n");
}

int iswin(char lable)
{
    if(board[0]==lable && board[1]==lable && board[2]==lable)   return 1;
	if(board[3]==lable && board[4]==lable && board[5]==lable)   return 1;
	if(board[6]==lable && board[7]==lable && board[8]==lable)   return 1;
	if(board[0]==lable && board[3]==lable && board[6]==lable)   return 1;
	if(board[1]==lable && board[4]==lable && board[7]==lable)   return 1;
	if(board[2]==lable && board[5]==lable && board[8]==lable)   return 1;
	if(board[0]==lable && board[4]==lable && board[8]==lable)   return 1;
	if(board[2]==lable && board[4]==lable && board[6]==lable)   return 1;
	return 0;
}

int isfair()
{
	for(int i=0; i<9; i++)
	{
		if(board[i] == ' ') {
            return 0;
        }
	}
	return 1;
}

void print()
{
	printf("%c|%c|%c\n",board[0], board[1], board[2]);
	printf("------\n");
	printf("%c|%c|%c\n",board[3], board[4], board[5]);
	printf("------\n");
	printf("%c|%c|%c\n",board[6], board[7], board[8]);
}

void start()
{
    char buf2[100] = {};
    /* 帳密驗證 */
	while(1) {
		recv(sockfd, buf2, sizeof(buf2), 0);
		if (strcmp(buf2, "authenticate") == 0){
			send(sockfd, account, strlen(account), 0);
			break;
		}
	}
    pthread_t id;
    void *recv_thread(void *);
    pthread_create(&id, 0, recv_thread, 0);
    int first = 1; // 遊戲剛開始
    while(1) {
        char buf[100];
        fgets(buf, sizeof(buf), stdin);
        if(game==1 && isMe==1 && first==1) {
            print();
			printf("Please enter #(0-8)\n");
			first = 0;
        }
        if(game == 1)   first=0;
        char *ptr = strstr(buf, "\n");
        *ptr = '\0';
        char msg[256];
        if(strcmp(buf, "bye") == 0) {
            sprintf(msg, "[System] %s exit the room", name);
            send(sockfd, msg, strlen(msg), 0);
            break;
        }
        else if(strcmp(buf, "ls") == 0) {
            sprintf(msg, "ls");
            send(sockfd, msg, strlen(msg), 0);
        }
        else if(buf[0] == '@') {
            sprintf(msg, "%s", buf);
            send(sockfd, msg, strlen(msg), 0);
        }
        else if(buf[0] == '#') {
            if(game == 0)   printf("The game ends or doesn't start!\n");
            else if(isMe == 0)   printf("Please wait for your opponent!\n");
            else {
                int num = atoi(&buf[1]);
                if(board[num]!=' ' || num>9 || num<0) {
                    printf("Please choose another number. #(0~8)\n");
                }
                else {
                    board[num] = lable1;
                    printf("----------\n");
					print();
                    if(iswin(lable1)) {
                        printf("[System] You Win!!\n");
                        game = 0;
                    }
                    else if(isfair()) {
                        printf("[System] 3Fair!\n");
                        game = 0;
                    }
                    else {
                        printf("\nWait for your opponent!\n");
                        isMe = 0;
                        sprintf(msg, "#%d %d", num, oppofd);
                        send(sockfd, msg, strlen(msg), 0);
                    }
                }
            }
        }
        else if(strcmp(buf, "yes") == 0) {
            sprintf(msg, "AGREE %d", oppofd);
            send(sockfd, msg, strlen(msg), 0);
            printf("[System] Game Start!\n");
            for(int i=0; i<9; i++) {
                board[i] = ' ';
            }
            game = 1;
            for(int i=0; i<9; i++) {
                board[i] = ' ';
            }
            lable1 = 'O', lable2 = 'X';
            if(lable1 == 'O') {
                isMe = 1;
                printf("Press Enter to start the game...\n");
            }
            else {
                isMe = 0;
                printf("Wait for your opponent...\n");
            }
        }
        else if(buf[0] == '#') {
            board[atoi(&buf[1])] = lable2;
            if(iswin(lable2)) {
                printf("[System] You Lose!!\n");
                game = 0;
            }
            else if(isfair()) {
                printf("[System] Fair!!\n");
                game = 0;
            }
            isMe = 1;
        }
        else if(buf[0] != '\0'){
			sprintf(msg, "[%s] : %s", name, buf);
			send(sockfd, msg, strlen(msg), 0);
		}
    }
    close(sockfd);
}

void *recv_thread(void *p)
{
    char *ptr, *qtr;
    while(1) {
        char buf[100] = {};
        if(recv(sockfd, buf, sizeof(buf), 0) <= 0) {
            return;
        }
        if (strcmp(buf,"authenticate") == 0){
			send(sockfd,name,strlen(name),0);
		}
        else if(strncmp(buf, "CONNECT ", 8) == 0) {
            ptr = strstr(buf, " ");
            ptr++;
            qtr = strstr(ptr, " ");
            *qtr = '\0';
            strcpy(opponame, ptr);
            qtr++;
            oppofd = atoi(qtr);
            printf("[System] Do you agree to play a game with %s ?\n", opponame);
        }
        else if(strncmp(buf, "AGREE ", 6) == 0) {
            ptr = strstr(buf, " ");
            ptr++;
            qtr = strstr(ptr, " ");
            *qtr = '\0';
            strcpy(opponame, ptr);
            qtr++;
            oppofd = atoi(qtr);
            printf("[System] %s agree with you!\n", opponame);
            printf("[System] Game Start!\n");
            for(int i=0; i<9; i++) {
                board[i] = ' ';
            }
            game = 1;
            lable1 = 'X', lable2 = 'O';
            if(lable1 == 'O') {
                isMe = 1;
                printf("Press Enter to start the game...\n");
            }
            else {
                isMe = 0;
                printf("Wait for your opponent...\n");
            }
        }
        else if(buf[0] == '#') {
            board[atoi(&buf[1])] = lable2;
            if(iswin(lable2)) {
                printf("[System] You Lose!!\n");
                game = 0;
            }
            else if(isfair()) {
                printf("[System] 2Fair!!\n");
                game = 0;
            }
			else{
				isMe=1;
				print();
				printf("Please enter #(0-8)\n");
			}
        }
        else {
            printf("%s\n", buf);
        }
    }
}

int main(int argc, char **argv)
{
    init();
    printf("[System] 請輸入您的帳號 : ");
    scanf("%s", name);
    printf("[System] 請輸入您的密碼 : ");
    scanf("%s", passwd);
    sprintf(account, "%s:%s@", name, passwd);
	start();
	return 0;
}