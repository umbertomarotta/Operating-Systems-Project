//
//  main.c
//  ServerClient
//
//  Created by Vince on 8/19/13.
//  Copyright (c) 2013 Vince. All rights reserved.
//
// Remember: Comments don't compile

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
UserList Users = NULL;
UserList Users;
int logfile;
User SERVER;

// Static strings
const char menu[MAXBUF] = "\n 1. Visualizza utenti online \n 2. Invia msg pubblico \n 3. Invia msg privato \n 4. Leggi msg pubblici \n 5. Leggi msg privati \n 6. Muovi \n 7. Leggi notifiche \n 8. Esci \n > ";
const char hello[MAXBUF] = "\n## MESSAGE SYSTEM ## \n 1. Registrazione \n 2. Esci \n > ";

void userdb_init(){
    struct User u;
    int userfile = open("userdb", O_RDWR|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR);
    UserList ptr = NULL;
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
    char buffer[MAXBUF+1];
    int choice = 0;
    do {
        // Sending welcome message
        _send(fd, hello);

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
                close(fd);
                pthread_exit(0);
            default:
                break;
        }
    } while (choice > 0 && choice < 4);
    close(fd);
    pthread_exit(0);
}


// Main Function
//int main(int argc, const char * argv[]) {
int main() {

    //Hardcoded parameters
    int argc = 2;
    char* argv[3];
    argv[3] = "logfile.txt";
    argv[1] = "4004";

    // Declaring some variables
    int server_socket,
        port_no,
        client_addr_len,
        retcode,
        fd;
    userdb_init();
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
    //_info("Server listening and waiting connections.");
    //printf("Server: waiting connection \n");

    SERVER = (User)malloc(sizeof(struct User));
    strcpy(SERVER->username,"SERVER");

    // Ignore SIGPIPE is always good
    signal(SIGPIPE, SIG_IGN);

    client_addr_len = sizeof(client);


    // Socket loop
    while (1) {
        fd = accept(server_socket, (struct sockaddr*)&client, &client_addr_len);
        pthread_create(&tid, NULL, manage, (void*)&fd);
        pthread_detach(tid);
    }

    close(server_socket);
    _info("Socket closed.");
    _info("Server shut down");
    return 0;
}

