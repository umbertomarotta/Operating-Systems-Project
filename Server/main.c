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
#include "library.h"

// define externed vars
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;
UserList Users;
MapList map[MAXMAP][MAXMAP];
MessageList checkingMessages;
int logfile;
User SERVER;

// Static strings
const char menu[MAXBUF] = "\n 1. Visualizza utenti \n 2. Invia msg pubblico \n 3. Invia msg privato \n 4. Leggi msg pubblici \n 5. Leggi msg privati \n 6. Muovi \n 7. Leggi notifiche \n 8. Esci \n > ";
const char hello[MAXBUF] = "\n## MESSAGE SYSTEM ## \n 1. Registrazione \n 2. Esci \n > ";


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
int main(int argc, const char * argv[]) {
    
    // Declaring some variables
    int server_socket,
        port_no,
        client_addr_len,
        retcode,
        fd;
    
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
    
    printf("Server: initialization \n");
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
    printf("Server: waiting connection \n");
    
    SERVER = (User)malloc(sizeof(struct UserType));
    SERVER->username = "SERVER";
    
    // Ignore SIGPIPE is always good
    signal(SIGPIPE, SIG_IGN);
    
    client_addr_len = sizeof(client);
    
    int x = 0, y = 0;
    for (x = 0; x < 256; x++) {
        for (y = 0; y < 256; y++) {
            map[x][y] = (MapList)malloc(sizeof(struct MapListType));
            map[x][y]->messages = None;
            map[x][y]->users = None;
        }
    }
    
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

