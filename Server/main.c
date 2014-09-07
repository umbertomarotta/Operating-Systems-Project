#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include "lib.h"

// define externed vars
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t userdb_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t filmdb_mutex = PTHREAD_MUTEX_INITIALIZER;
UserList Users = NULL;
FilmList Films = NULL;
int logfile;
User SERVER;

// Static strings
const char menu[MAXBUF] = "\n 1. Visualizza utenti online \n 2. Visualizza elenco film \n 3. Visualizza Notifiche \n 4. Esci \n > ";
const char hello[MAXBUF] = "\n## MOVIE RATING SYSTEM ## \n 1. Registrazione \n 2. Login \n 3. Esci \n > ";

void votedb_init(C_ValutationList *Head, char* title, int id){
    struct C_Valutation vot;
    char* buffer = (char*)malloc(sizeof(char)*MAXBUF+1);
    sprintf(buffer, "%s.%d.rec_vote", title, id);
    int vot_db=open(buffer, O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
    C_ValutationList ptr = NULL;
    while(read(vot_db, &vot, sizeof(struct C_Valutation))){
        if(*Head==NULL){
            *Head=(C_ValutationList)malloc(sizeof(struct C_ValutationList));
            (*Head)->c_valutation=(C_Valutation)malloc(sizeof(struct C_Valutation));
            memcpy((*Head)->c_valutation, &vot, sizeof(struct C_Valutation));
            (*Head)->next=NULL;
        }else{
            ptr=(C_ValutationList)malloc(sizeof(struct C_ValutationList));
            ptr->c_valutation=(C_Valutation)malloc(sizeof(struct C_Valutation));
            memcpy(ptr->c_valutation, &vot, sizeof(struct C_Valutation));
            ptr->next=*Head;
            *Head=ptr;
        }
    }
    close(vot_db);
}

void commentdb_init(F_ValutationList *Head, F_Valutation val, char *title){
    if(*Head==NULL){
        *Head=(F_ValutationList)malloc(sizeof(struct F_ValutationList));
        (*Head)->f_valutation=val;
        (*Head)->next=NULL;
        (*Head)->f_valutation->from=find_username(Users, (*Head)->f_valutation->user);
    }else{
        F_ValutationList ptr=(F_ValutationList)malloc(sizeof(struct F_ValutationList));
        ptr->f_valutation=val;
        ptr->f_valutation->from=find_username(Users, val->user);
        ptr->next=*Head;
        *Head=ptr;
    }
    votedb_init(&val->CommentScores, title, val->id);
}

void filmdb_init(){
    struct Film f;
    //FilmList ptr = Films;
    int filmfile = open("filmdb", O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
    struct F_Valutation v;
    char* buffer = (char*)malloc(sizeof(char)*MAXBUF+1);
    int filmcomments;    
    F_Valutation val=NULL;
    while(read(filmfile, &f, sizeof(struct Film))){
        Film new_film = (Film)malloc(sizeof(struct Film));
        memcpy(new_film, &f, sizeof(struct Film));
        F_add_to(new_film);
        sprintf(buffer, "%s.film_comments", new_film->title);
        filmcomments=open(buffer, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
        while(read(filmcomments, &v, sizeof(struct F_Valutation))){
            val=(F_Valutation)malloc(sizeof(struct F_Valutation));
            memcpy(val, &v, sizeof(struct F_Valutation));
            new_film->f_avg_count++;
            new_film->f_part_avg += val->F_score;
            commentdb_init(&(new_film->film_valutations), val, new_film->title);
        }
        new_film->f_avg= (float) new_film->f_part_avg / new_film->f_avg_count;
        close(filmcomments);
    }
    close(filmfile);
}

void userdb_init(){
    struct User u;
    int userfile = open("userdb", O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR);
    //UserList ptr = NULL;
    while(read(userfile, &u, sizeof(struct User))){
        if(Users == NULL) {
            Users = (UserList)malloc(sizeof(struct UserList));
            Users->user = (User)malloc(sizeof(struct User));
            memcpy(Users->user, &u, sizeof(struct User));
            Users->next=NULL;
        } else {
            UserList N = (UserList)malloc(sizeof(struct UserList));
            N->user = (User)malloc(sizeof(struct User));
            memcpy(N->user, &u, sizeof(struct User));
            N->next=Users;
            Users=N;
        }
    }
    close(userfile);
}


// Thread for first user interaction: Registration - Login or Exits
void *manage(void *arg) {
    int fd = *(int*) arg;
    _info("New thread created");
    char* buffer = (char*)malloc(sizeof(char)*MAXBUF+1);
    int choice = 0;
    do {
        // Sending welcome message
        strcpy(buffer, hello);
        _send(fd, buffer);

        // Give the user a choice
        _recv(fd, buffer, 1);
        while ((atoi(buffer) <= 0 || atoi(buffer) > 3)) {
            _send(fd, " > ");
            _recv(fd, buffer, 1);
        }
        choice = atoi(buffer);

        switch (choice) {
            case 1: {
                manage_user(fd, 1);
            }
                break;
            case 2: 
                {
                manage_user(fd, 0);
                }
                break;
            case 3:
                //if(buffer) free(buffer);
                close(fd);
                pthread_exit(0);
            default:
                break;
        }
    } while (choice > 0 && choice < 4);
    //if(buffer) free(buffer);
    close(fd);
    pthread_exit(0);
}


// Main Function
//int main(int argc, const char * argv[]) {
int main() {

    //Hardcoded parameters
    int argc = 2;
    char* argv[4];
    argv[3] = "logfile.txt";
    argv[1] = "4005";
    // Declaring some variables
    int server_socket,
        port_no,
        client_addr_len,
        retcode,
        fd;
    userdb_init();
    filmdb_init();
    pthread_t tid;
    struct sockaddr_in server_addr, client;
    if (argc < 2) {
        printf("Usage:\n%s port [file_logging]\n", argv[0]);
        _error("Server stared with wrong arguments, terminating.");
        return 0;
    } else if (argc == 3) {
        logfile = open(argv[3],O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR);
    } else {
        logfile = open("log.txt",O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR);
    }

    //printf("Server: initialization \n");
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Opening server socket: ");
        _error("Server error opening socket.");
        return -1;
    }
    _info("Server initalized.");
    // Get the port number from argv
    port_no = atoi(argv[1]);

    // Setting up the server
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_no);

    retcode = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));

    if (retcode == -1) {
        perror("Bind error: ");
        return -1;
    }

    listen(server_socket, 1);
    _info("Server listening and waiting connections.");
    //printf("Server: waiting connection \n");

    SERVER = (User)malloc(sizeof(struct User));
    strcpy(SERVER->username,"SERVER");

    // Ignore SIGPIPE is always good
    signal(SIGPIPE, SIG_IGN);

    client_addr_len = sizeof(client);


    // Socket loop
    while (1) {
        fd = accept(server_socket, (struct sockaddr*)&client, (socklen_t *)&client_addr_len);
        pthread_create(&tid, NULL, manage, (void*)&fd);
        pthread_detach(tid);
    }

    close(server_socket);
    _info("Socket closed.");
    _info("Server shut down");
    return 0;
}

