#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 100
#define MAX_GROUPS 50
#define BUF_SIZE 2048

typedef struct {
    int socket;
    char username[50];
    int logged;
} Client;

typedef struct {
    char name[50];
    char members[10][50];
    int count;
} Group;

Client clients[MAX_CLIENTS];
Group groups[MAX_GROUPS];
int client_count = 0;
int group_count = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void send_msg(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

Client* get_client_by_name(char *name) {
    for(int i=0;i<client_count;i++)
        if(clients[i].logged && strcmp(clients[i].username, name)==0)
            return &clients[i];
    return NULL;
}

int user_exists(const char *login) {
    FILE *f = fopen("users.txt", "r");
    if(!f) return 0;
    char line[100], u[50], p[50];
    while(fgets(line, sizeof(line), f)) {
        sscanf(line, "%[^:]:%s", u, p);
        if(strcmp(u, login)==0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int check_login(const char *login, const char *pass) {
    FILE *f = fopen("users.txt", "r");
    if(!f) return 0;
    char line[100], u[50], p[50];
    while(fgets(line, sizeof(line), f)) {
        sscanf(line, "%[^:]:%s", u, p);
        if(strcmp(u, login)==0 && strcmp(p, pass)==0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void register_user(const char *login, const char *pass) {
    FILE *f = fopen("users.txt", "a");
    fprintf(f, "%s:%s\n", login, pass);
    fclose(f);
}

void *socketThread(void *arg) {
    int sock = *(int*)arg;
    char buffer[BUF_SIZE];
    Client *me;

    pthread_mutex_lock(&lock);
    me = &clients[client_count++];
    me->socket = sock;
    me->logged = 0;
    pthread_mutex_unlock(&lock);

    send_msg(sock, "Witaj! REGISTER user pass | LOGIN user pass\n");

    while(1) {
        memset(buffer, 0, BUF_SIZE);
        int n = recv(sock, buffer, BUF_SIZE, 0);
        if(n <= 0) break;

        char cmd[50], a[50], b[200];
        sscanf(buffer, "%s %s %[^\n]", cmd, a, b);

        if(strcmp(cmd,"REGISTER")==0) {
            if(user_exists(a)) send_msg(sock,"Uzytkownik istnieje\n");
            else {
                register_user(a,b);
                send_msg(sock,"Zarejestrowano\n");
            }
        }

        else if(strcmp(cmd,"LOGIN")==0) {
            if(check_login(a,b)) {
                strcpy(me->username,a);
                me->logged = 1;
                send_msg(sock,"Zalogowano\n");
            } else send_msg(sock,"Bledne dane\n");
        }

        else if(strcmp(cmd,"MSG")==0) {
            Client *dest = get_client_by_name(a);
            if(dest) {
                char msg[512];
                snprintf(msg, sizeof(msg), "[PM od %s]: %s\n", me->username, b);
                send_msg(dest->socket, msg);
            } else send_msg(sock,"Uzytkownik offline\n");
        }

        else if(strcmp(cmd,"CREATE_GROUP")==0) {
            strcpy(groups[group_count].name,a);
            groups[group_count].count=1;
            strcpy(groups[group_count].members[0], me->username);
            group_count++;
            send_msg(sock,"Utworzono grupe\n");
        }

        else if(strcmp(cmd,"JOIN_GROUP")==0) {
            for(int i=0;i<group_count;i++){
                if(strcmp(groups[i].name,a)==0){
                    strcpy(groups[i].members[groups[i].count++], me->username);
                    send_msg(sock,"Dolaczono do grupy\n");
                }
            }
        }

        else if(strcmp(cmd,"GROUP_MSG")==0) {
            for(int i=0;i<group_count;i++){
                if(strcmp(groups[i].name,a)==0){
                    for(int j=0;j<groups[i].count;j++){
                        Client *c = get_client_by_name(groups[i].members[j]);
                        if(c){
                            char msg[512];
                            snprintf(msg, sizeof(msg),
                                     "[Grupa %s | %s]: %s\n",
                                     a, me->username, b);
                            send_msg(c->socket, msg);
                        }
                    }
                }
            }
        }
    }

    close(sock);
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Uzycie: %s <adres_ip> <port>\n", argv[0]);
        exit(1);
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    pthread_t tid;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip);

    bind(serverSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr));
    listen(serverSocket,50);

    printf("Serwer dziala na %s:%d\n", ip, port);

    while(1){
        addr_size = sizeof(serverAddr);
        newSocket = accept(serverSocket,(struct sockaddr*)&serverAddr,&addr_size);
        pthread_create(&tid,NULL,socketThread,&newSocket);
        pthread_detach(tid);
    }
}
