//
//  library.c
//  ServerClient
//
//  Created by Vince on 8/29/13.
//  Copyright (c) 2013 Vince. All rights reserved.
//
#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

// SAFER SEND AND RECV
void _send(int fd, const void* buffer) {
    // We will close the connection if the buffer received is lte 0
    if (send(fd, buffer, strlen((char*)buffer), 0) <= 0) {
        close(fd);
        pthread_exit(0);
    }
}

void _recv(int fd, void* buffer, int be_string) {
    memset(buffer, 0, MAXBUF);
    long char_read = recv(fd, buffer, MAXBUF, 0);
    if (char_read <= 0){
        close(fd);
        pthread_exit(0);
    }
    
    // Use this only if you know that the message is not a string
    // AND you WANT a string
    if (be_string is 0) {
        ((char*)(buffer))[char_read] = '\0';
    }
}


// USER FUNCTIONS
// Manages the user interaction
void manage_user(int fd, int registration) {
    User user = None;
    char buffer[MAXBUF+1];
    if (registration == 1) {
        _info("Serving the registration form.");
        User new_user = (struct UserType*)malloc(sizeof(struct UserType));
        new_user->fd = fd;
        new_user->notifications = None;
        new_user->noti_count = 0;
        new_user->private_messages = None;
        
        do {
            _send(fd, " > Username: ");
            _recv(fd, buffer, 1);
            if (strcmp(buffer, ":q") == 0) {
                _error("Registration task was interrupted by user..");
                return;
            }
        } while (find_username(Users, buffer) is_not None);
        new_user->username = (char*)malloc(sizeof(MAXBUF));
        strcpy(new_user->username, buffer);
        
        _send(fd, " > Nome: ");
        _recv(fd, buffer, 1);
        if (strcmp(buffer, ":q") == 0) {
            _error("Registration task was interrupted by user..");
            return;
        }
        new_user->name = (char*)malloc(sizeof(MAXBUF));
        strcpy(new_user->name, buffer);
        
        _send(fd, " > Cognome: ");
        _recv(fd, buffer, 1);
        if (strcmp(buffer, ":q") == 0) {
            _error("Registration task was interrupted by user..");
            return;
        }
        new_user->surname = (char*)malloc(sizeof(MAXBUF));
        strcpy(new_user->surname, buffer);
        
        _send(fd, " > Posizione X (int): ");
        _recv(fd, buffer, 1);
        if (strcmp(buffer, ":q") == 0) {
            _error("Registration task was interrupted by user..");
            return;
        }
        new_user->x = atoi(buffer) % 256;
        
        _send(fd, " > Posizione Y (int): ");
        _recv(fd, buffer, 1);
        if (strcmp(buffer, ":q") == 0) {
            _error("Registration task was interrupted by user..");
            return;
        }
        new_user->y = atoi(buffer) % 256;
        
        _send(fd, " > Raggio d'Azione (int): ");
        _recv(fd, buffer, 1);
        if (strcmp(buffer, ":q") == 0) {
            _error("Registration task was interrupted by user..");
            return;
        }
        new_user->rad = atoi(buffer) % 256;
        user = new_user;
        pthread_mutex_lock(&users_mutex);
        if (Users is None) {
            Users = (UserList)malloc(sizeof(struct UserListType));
            Users->info = user;
            Users->next = NULL;
        } else {
            UserList N = (UserList)malloc(sizeof(struct UserListType));
            N->info = user;
            N->next = Users;
            Users = N;
        }
        pthread_mutex_unlock(&users_mutex);
        pthread_mutex_lock(&map_mutex);
        map_user(user);
        pthread_mutex_unlock(&map_mutex);
        _infoUser("New user created.", user->username);
    }
    assert(user is_not None);
    _infoUser("User logged in.", user->username);
    int choice;
    do {
        memset(buffer, 0, MAXBUF);
        char notif[MAXBUF];
        if (user->noti_count > 0) {
            sprintf(notif, "\n## MESSAGE SYSTEM MENU ## [Notifiche: %d]",
                    user->noti_count);
        } else {
            sprintf(notif, "\n## MESSAGE SYSTEM MENU ##");
        }
        strcat(notif, menu);
        _send(user->fd, notif);
        _recv(user->fd, buffer, 1);
        while (atoi(buffer) <= 0 or atoi(buffer) > 8) {
            _send(user->fd, " > ");
            _recv(user->fd, buffer, 1);
        }
        choice = atoi(buffer);
        switch (choice) {
            case 1: {
                show_users_available(user);
            }
                break;
            case 2: {
                send_public_message(user);
            }
                break;
            case 3: {
                send_private_message(user);
            }
                break;
            case 4: {
                read_public_messages(user);
                break;
            }
                break;
            case 5: {
                read_private_messages(user);
                break;
            }
            case 6: {
                move_user(user);
            }
                break;
            case 7: {
                read_notifications(user);
            }
                break;
            case 8: {
                return;
            }
            default:
                break;
        }
    } while (choice > 0 && choice < 8);
}

void move_user(User user) {
    char buffer[MAXBUF+1];
    int choice = 0;
    int well = 0;
    do {
        if (well == 1) {
            _send(user->fd, "\n## MUOVI ## \n 1. Nord \n 2. Sud \n 3. Est \n 4. Ovest \n 5. Esci \n > Operazione effettuata. \n > ");
        } else {
            _send(user->fd, "\n## MUOVI ## \n 1. Nord \n 2. Sud \n 3. Est \n 4. Ovest \n 5. Esci \n > ");
        }
        _recv(user->fd, buffer, 0);
        while (atoi(buffer) <= 0 or atoi(buffer) > 6) {
            _send(user->fd, " > ");
            _recv(user->fd, buffer, 1);
        }
        choice = atoi(buffer);
        switch (choice) {
            case 1:
            {
                remap_user(user, user->x, user->y+4);
                well = 1;
            }
                break;
            case 2:
            {
                remap_user(user, user->x, user->y-4);
                well = 1;
            }
                break;
            case 3:
            {
                remap_user(user, user->x+4, user->y);
                well = 1;
            }
                break;
            case 4:
            {
                remap_user(user, user->x-4, user->y);
                well = 1;
            }
                break;
            default: {
                well = 0;
            }
                break;
        }
    } while (choice > 0 && choice < 5);
    
}

// Checks if a username is available
User find_username(UserList U, char *new_user) {
    if (U is_not None) {
        while (U != NULL && U->info != NULL) {
            if (strcmp(U->info->username, new_user) == 0){
                return U->info;
            }
            U = U->next;
        }
    }
    return None;
}


// Adds a user to the given UserList U, if any
// Returns a UserList
UserList add_to(UserList U, User new_user) {
    if (U is None) {
        U = (UserList)malloc(sizeof(struct UserListType));
        U->info = new_user;
        U->next = None;
        return U;
    } else {
        UserList N = (UserList)malloc(sizeof(struct UserListType));
        N->info = new_user;
        N->next = U;
        return N;
    }
}

// Removes a user from the given UserList U, if any
// Returns a UserList
UserList remove_from(UserList U, User user) {
    if (U is_not None) {
        if (U->info == user) {
            return U->next;  // Will not free the user on purpose
        }
        U->next = remove_from(U->next, user);
        return U;
    }
    return None;
}

// Sets the user position into the global map
void map_user(User user) {
    assert(user is_not None);
    if (user->x < 0) {
        user->x = 0;
    }
    if (user->y < 0) {
        user->y = 0;
    }
    if (user->x >= 256) {
        user->x = 255;
    }
    if (user->y >= 256) {
        user->y = 255;
    }
    map[user->x][user->y]->users = add_to(map[user->x][user->y]->users, user);
    _infoUser("User is now mapped.", user->username);
}

void remap_user(User user, int x, int y) {
    assert(user is_not None);
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (x >= 256) {
        x = 255;
    }
    if (y >= 256) {
        y = 255;
    }
    pthread_mutex_lock(&map_mutex);
    map[user->x][user->y]->users = remove_from(map[user->x][user->y]->users, user);
    map[x][y]->users = add_to(map[x][y]->users, user);
    user->x = x;
    user->y = y;
    pthread_mutex_unlock(&map_mutex);
    _infoUser("User moved.", user->username);
}

// Show available users
void show_users_available(User user) {
    assert(user is_not None);
    int start_x = user->x - user->rad;
    int start_y = user->y - user->rad;
    int final_x = user->x + user->rad;
    int final_y = user->y + user->rad;
    if (start_x < 0) {
        start_x = 0;
    }
    if (start_y < 0) {
        start_y = 0;
    }
    if (final_x >= 256) {
        final_x = 255;
    }
    if (final_y >= 256) {
        final_y = 255;
    }
    int x = 0;
    int y = 0;
    UserList resL = NULL;
    for (x = start_x ; x < final_x; x++) {
        for (y = start_y; y < final_y; y++) {
            if (map[x][y]->users is_not None) {
                pthread_mutex_lock(&map_mutex);
                UserList U = map[x][y]->users;
                while (U is_not None) {
                    resL = add_to(resL, U->info);
                    U = U->next;
                }
                pthread_mutex_unlock(&map_mutex);
            }
        }
    }
    char buffer[MAXBUF] = "\n## Utenti disponibili nella tua area \n ";
    int length = (int)strlen(buffer);
    int counter = 0;
    UserList M = resL;
    while (M is_not None) {
        length += sprintf(buffer+length, " (%d) %s \n ", counter, M->info->username);
        counter += 1;
        M = M->next;
    }
    strcat(buffer, "\n Premi 0 per continuare... ");
    _send(user->fd, buffer);
    _recv(user->fd, buffer, 0);
    _infoUser("Asked for available users.", user->username);
}


// MESSAGE FUNCTIONS
// Read messages and cleans up the oldies
MessageList readMessages(MessageList M, char* buffer, int length) {
    if (M is_not None && (difftime(time(NULL), M->timer) < 600)) {
        char date[80];
        struct tm ts = *localtime(&M->timer);
        strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
        length += sprintf(buffer+length, " - [ FROM %s @ %s ] \n   -> %s \n", M->from->username, date, M->message);
        M->next = readMessages(M->next, buffer, length);
        return M;
    } else if (M is_not None && (difftime(time(NULL), M->timer) >= 600)){
        MessageList tmp = M->next;
        free(M);
        return readMessages(tmp, buffer, length);
    }
    return None;
}

MessageList add_message_to(MessageList M, char *message, User from, int private) {
    if (M is None) {
        M = (MessageList)malloc(sizeof(struct MessageListType));
        M->from = from;
        strcpy(M->message, message);
        M->next = None;
        M->is_private = private;
        M->read = 0;
        M->timer = time(NULL);
        return M;
    } else {
        MessageList N = (MessageList)malloc(sizeof(struct MessageListType));
        N->from = from;
        strcpy(N->message, message);
        N->next = M;
        N->is_private = private;
        N->read = 0;
        N->timer = time(NULL);
        return N;
    }
}

// Send a public message
void send_public_message(User user) {
    char message[MAXBUF+1];
    _send(user->fd, " > Message: ");
    _recv(user->fd, message, 1);
    pthread_mutex_lock(&map_mutex);
    map[user->x][user->y]->messages = add_message_to(map[user->x][user->y]->messages, message, user, 0);
    pthread_mutex_unlock(&map_mutex);
    _infoUser("Sent a new public message.", user->username);
}

void *checkMessages(void *arg) {
    sleep(600);
    MessageList M = *(MessageList*)arg;
    if (M->read == 0) {
        char notif[MAXBUF+1];
        sprintf(notif, "Il messaggio \n -> %s \n e' scaduto.", M->message);
        pthread_mutex_lock(&users_mutex);
        M->from->notifications = add_message_to(M->from->notifications, notif, SERVER, 1);
        M->from->noti_count += 1;
        pthread_mutex_unlock(&users_mutex);
    }
    pthread_exit(0);
}

// Send a private message
int send_private_message(User user) {
    int tries = 0;
    char send_to[MAXBUF+1];
    char message[MAXBUF+1];
    User rcpt = None;
    do {
        if (tries is 0) {
            _send(user->fd, "\n # Send a private message # \n > To: ");
            _recv(user->fd, send_to, 1);
        } else {
            _send(user->fd, "\n (SERVER): Username not found > To: ");
            _recv(user->fd, send_to, 1);
        }
        if (strcmp(send_to, ":q") == 0) {
            return 0;
        }
        tries += 1;
        rcpt = find_username(Users, send_to);
    } while (rcpt is None);
    
    _send(user->fd, " > Message: ");
    _recv(user->fd, message, 1);
    if (strcmp(message, ":q") == 0) {
        return 0;
    }
    pthread_mutex_lock(&users_mutex);
    rcpt->private_messages = add_message_to(rcpt->private_messages, message, user, 1);
    char notif[MAXBUF+1];
    sprintf(notif, "Hai un messaggio privato da %s", user->username);
    rcpt->notifications = add_message_to(rcpt->notifications, notif, SERVER, 1);
    rcpt->noti_count += 1;
    pthread_t tid;
    pthread_create(&tid, NULL, checkMessages, (void*)&rcpt->private_messages);
    pthread_detach(tid);
    pthread_mutex_unlock(&users_mutex);
    _infoUser("Sent a new private message.", user->username);
    return 1;
}

// Read public messages
void read_public_messages(User user) {
    assert(user is_not None);
    int start_x = user->x - user->rad;
    int start_y = user->y - user->rad;
    int final_x = user->x + user->rad;
    int final_y = user->y + user->rad;
    if (start_x < 0) {
        start_x = 0;
    }
    if (start_y < 0) {
        start_y = 0;
    }
    if (final_x >= 256) {
        final_x = 255;
    }
    if (final_y >= 256) {
        final_y = 255;
    }
    int x = 0;
    int y = 0;
    char buffer[MAXBUF] = "\n## Messaggi pubblici \n ";
    for (x = start_x ; x < final_x; x++) {
        for (y = start_y; y < final_y; y++) {
            if (map[x][y]->messages is_not None) {
                pthread_mutex_lock(&map_mutex);
                map[x][y]->messages = readMessages(map[x][y]->messages, buffer, (int)strlen(buffer));
                pthread_mutex_unlock(&map_mutex);
            }
        }
    }
    strcat(buffer, "\n Premi 0 per uscire... ");
    _send(user->fd, buffer);
    _recv(user->fd, buffer, 0);
    _infoUser("Asked for public messages.", user->username);
}

// Read private messages
void read_private_messages(User user) {
    char buffer[MAXBUF] = "\n## MESSAGGI PRIVATI ## \n ";
    pthread_mutex_lock(&users_mutex);
    MessageList M = user->private_messages;
    pthread_mutex_unlock(&users_mutex);
    while (M is_not None) {
        sprintf(buffer, "\n Accetti il messaggio da %s (s/n)? ", M->from->username);
        _send(user->fd, buffer);
        _recv(user->fd, buffer, 0);
        if (strcmp(buffer, "s") == 0) {
            char msg[MAXBUF+1];
            sprintf(msg, " Messaggio: %s \n Premi 0 per continuare ", M->message);
            _send(user->fd, msg);
            _recv(user->fd, buffer, 0);
        } else {
            char notif[MAXBUF+1];
            sprintf(notif, "L'utente %s ha rifiutato la lettura del messaggio.", user->username);
            pthread_mutex_lock(&users_mutex);
            M->from->notifications = add_message_to(M->from->notifications, notif, SERVER, 1);
            M->from->noti_count += 1;
            pthread_mutex_unlock(&users_mutex);
        }
        MessageList tmp = M->next;
        free(M);
        M = tmp;
    }
}

// Read notifications
void read_notifications(User user) {
    char buffer[MAXBUF] = "\n## NOTIFICHE ## \n ";
    pthread_mutex_lock(&users_mutex);
    readMessages(user->notifications, buffer, (int)strlen(buffer));
    user->noti_count = 0;
    user->notifications = None;
    pthread_mutex_unlock(&users_mutex);
    strcat(buffer, "\n Premi 0 per continuare...");
    _send(user->fd, buffer);
    _recv(user->fd, buffer, 0);
    _infoUser("Asked for notifications.", user->username);
}

// Logging functions
int _info(char* buffer) {
    char date[80];
    char str[MAXBUF];
    time_t now = time(NULL);
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    sprintf(str, "[ INFO @ %s ]: %s \n", date, buffer);
    if (write(logfile, str, strlen(str)) == -1) {
        return 0;
    }
    return 1;
}

int _error(char* buffer) {
    char date[80];
    char str[MAXBUF];
    time_t now = time(NULL);
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    sprintf(str, "[ ERROR @ %s ]: %s \n", date, buffer);
    if (write(logfile, str, strlen(str)) == -1) {
        return 0;
    }
    return 1;
}

int _infoUser(char* buffer, char* user) {
    char date[80];
    char str[MAXBUF];
    time_t now = time(NULL);
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    sprintf(str, "[ INFO @ %s - User: %s ]: %s \n", date, user, buffer);
    if (write(logfile, str, strlen(str)) == -1) {
        return 0;
    }
    return 1;
}

int _errorUser(char* buffer, char* user) {
    char date[80];
    char str[MAXBUF];
    time_t now = time(NULL);
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    sprintf(str, "[ ERROR @ %s - User: %s ]: %s \n", date, user, buffer);
    if (write(logfile, str, strlen(str)) == -1) {
        return 0;
    }
    return 1;
}