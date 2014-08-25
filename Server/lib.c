#include "lib.h"
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
    if (be_string == 0) {
        ((char*)(buffer))[char_read] = '\0';
    }
}

void manage_user(int fd, int registration) {
    User user = NULL;
    char buffer[MAXBUF+1];
    if (registration == 1) {
        _info("Serving the registration form.");
        User new_user = (User)malloc(sizeof(struct User));
        new_user->fd = fd;
        new_user->noti_count = 0;
        new_user->avg_count = 0;
        new_user->part_avg = 0;
        new_user->u_average = 0;
        

        
        _send(fd, " > Nome: ");
        _recv(fd, buffer, 1);
        if (strcmp(buffer, ":q") == 0) {
            _error("Registration task was interrupted by user..");
            return;
        }
        //new_user->name = (char*)malloc(sizeof(char)*strlen(buffer));
        strcpy(new_user->name, buffer);
        
        _send(fd, " > Cognome: ");
        _recv(fd, buffer, 1);
        if (strcmp(buffer, ":q") == 0) {
            _error("Registration task was interrupted by user..");
            return;
        }
        //new_user->surname = (char*)malloc(strlen(buffer)*sizeof(char));
        strcpy(new_user->surname, buffer);
        
        do{
            _send(fd, " > Username: ");
            _recv(fd, buffer, 1);
            if (strcmp(buffer, ":q") == 0) {
                _error("Registration task was interrupted by user..");
                return;
            }
            //new_user->username = (char*)malloc(sizeof(char)*strlen(buffer));
            strcpy(new_user->username, buffer);
        } while (find_username(Users, buffer) != NULL);


        _send(fd, " > Password: ");
        _recv(fd, buffer, 1);
        if (strcmp(buffer, ":q") == 0) {
            _error("Registration task was interrupted by user..");
            return;
        }
        //new_user->password = (char*)malloc(sizeof(char)*strlen(buffer));
        strcpy(new_user->password, buffer);

        

        user = new_user;
        pthread_mutex_lock(&users_mutex);

         if(Users == NULL) {
            Users = (UserList)malloc(sizeof(struct UserList));
            Users->user = user;
            Users->next = NULL;
        } else {
            UserList N = (UserList)malloc(sizeof(struct UserList));
            N->user = user;
            N->next = Users;
            Users = N;
        }
        pthread_mutex_unlock(&users_mutex);
        _infoUser("New user created.", user->username);
    }
    assert(user != NULL);
     int userfile = open("userdb", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
    user->is_on=1;
    pthread_mutex_lock(&userdb_mutex);
    write(userfile, user, sizeof(struct User));
    pthread_mutex_unlock(&userdb_mutex);
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
        while (atoi(buffer) <= 0 || atoi(buffer) > 8) {
            _send(user->fd, " > ");
            _recv(user->fd, buffer, 1);
        }
        choice = atoi(buffer);
        switch (choice) {
            case 1: {
                show_online_users(user);
            }
                break;
            /*case 2: {
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
            }*/
            default:
                break;
        }
    } while (choice > 0 && choice < 8);
    user->is_on=0;
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

User find_username(UserList U, char *new_user){
    if(U != NULL){
        while(U!=NULL && U->user != NULL){
            if(strcmp(U->user->username, new_user)==0)
                return U->user;
            U=U->next;

        }
    }
}

UserList add_to(UserList U, User new_user) {
    if (U == NULL) {
        U = (UserList)malloc(sizeof(struct UserList));
        U->user = new_user;
        U->next = NULL;
        return U;
    } else {
        UserList N = (UserList)malloc(sizeof(struct UserList));
        N->user = new_user;
        N->next = U;
        return N;
    }
}

UserList remove_from(UserList U, User user) {
    if (U != NULL) {
        if (U->user == user) {
            return U->next;  // Will not free the user on purpose
        }
        U->next = remove_from(U->next, user);
        return U;
    }
    return NULL;
}

void show_online_users(User user){
    UserList u = Users;
    char buffer[MAXBUF];
    char aux[121];
    memset(buffer, '\0', MAXBUF);
    sprintf(buffer, "\n## ONLINE USERS ##\n");
    while(u != NULL){
        if(u->user->is_on == 1){
            sprintf(aux, " > %s\n", u->user->username);
            strcat(buffer, aux);
            
        }
        u=u->next;
    }
    _send(user->fd, buffer);
}

