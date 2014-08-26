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
        int userfile = open("userdb", O_RDWR | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
        pthread_mutex_lock(&userdb_mutex);
        write(userfile, user, sizeof(struct User));
        close(userfile);
        pthread_mutex_unlock(&userdb_mutex);
    }
    //Login BLOCK
    else if(registration==0){

        do{
            _send(fd, " > Username: ");
            _recv(fd, buffer, 1);
            user=find_username(Users,buffer);
            if(!user) _error("User doesn't exist");
            _send(fd, " > Password: ");
            _recv(fd, buffer, 1);
        }while(user!=NULL && strcmp(user->password, buffer)!=0);
    }
    assert(user != NULL);
    _infoUser("User logged in.", user->username);
    user->is_on=1;
    user->fd=fd;
    int choice;
    do {
        memset(buffer, '\0', MAXBUF);
        char notif[MAXBUF];
        if (user->noti_count > 0) {
            sprintf(notif, "\n## MOVIE RATING SYSTEM MENU ## [Notifiche: %d]",
                    user->noti_count);
        } else {
            sprintf(notif, "\n## RATING SYSTEM MENU ##");
        }
        strcat(notif, menu);
        _send(user->fd, notif);
        _recv(user->fd, buffer, 1);
        while (atoi(buffer) <= 0 || atoi(buffer) > 6) {
            _send(user->fd, " > ");
            _recv(user->fd, buffer, 1);
        }
        choice = atoi(buffer);
        switch (choice) {
            case 1: {
                show_online_users(user);
            }
                break;
            case 2: {
                show_film(user);
            }
                break;
            case 3: {
                add_film(user);
            }
                break;
            case 4: {
                show_f_val(user);
            }
                break;
            case 5: {
                add_val(user);
                break;
            }
           /*case 6: {
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
    } while (choice > 0 && choice < 6);
    user->is_on=0;
    _infoUser("User logged out.", user->username);
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

Film find_film(FilmList F, char *f_title){
    if(F != NULL){
        while(F != NULL && F->film !=NULL){
            if(strcmp(F->film->title, f_title)==0)
                return F->film;
            F=F->next;
        }
    }
    return NULL;
}

UserList U_add_to(UserList U, User new_user) {
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

void add_film(User u){
    char buffer[MAXBUF];
    Film new_film = (Film)malloc(sizeof(struct Film));
    _send(u->fd, " > Insert title: ");
    _recv(u->fd, buffer, 1);
    strcpy(new_film->title, buffer);
    _send(u->fd, " > Insert year: ");
    _recv(u->fd, buffer, 1);
    strcpy(new_film->year, buffer);
    _send(u->fd, " > Insert genre: ");
    _recv(u->fd, buffer, 1);
    strcpy(new_film->genre, buffer);
    new_film->f_average=0;
    new_film->film_valutations=NULL;
    pthread_mutex_init(&new_film->val_mutex, NULL); 
    F_add_to(new_film);
    pthread_mutex_lock(&filmdb_mutex);
    int filmfile=open("filmdb", O_RDWR|O_CREAT|O_APPEND|O_TRUNC,S_IRUSR,S_IWUSR);
    FilmList ptr=Films;
    while(ptr!=NULL){
        write(filmfile, ptr->film, sizeof(struct Film));
        ptr=ptr->next;
    }
    close(filmfile);
    pthread_mutex_unlock(&filmdb_mutex);
    _info("New film added to database.");
}

void F_add_to(Film new_film){
    FilmList F=Films;
    if(F==NULL || strcmp(F->film->title,new_film->title)>0){
        F = (FilmList)malloc(sizeof(struct FilmList));
        F->film=new_film;
        F->next=Films;
        Films=F;
    }else{
        FilmList pred=F;
        while(pred->next!=NULL && strcmp(pred->next->film->title,new_film->title)<0)
            pred=pred->next;
        FilmList aux = (FilmList)malloc(sizeof(struct FilmList));
        aux->film=new_film;
        aux->next=pred->next;
        pred->next=aux;
    }
    return;
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
    memset(buffer, '\0', MAXBUF);
    sprintf(buffer, "\n## ONLINE USERS ##\n");
    _send(user->fd, buffer);
    while(u != NULL){
        if(u->user->is_on == 1){
            sprintf(buffer, " > %s\n", u->user->username);
            _send(user->fd, buffer);
            
        }
        u=u->next;
    }
}

void show_film(User u){
    FilmList f = Films;
    char buffer[MAXBUF];
    memset(buffer, '\0', MAXBUF);
    sprintf(buffer, "\n ## ELENCO FILM ##\n");
    _send(u->fd, buffer);
    while(f != NULL){
        sprintf(buffer, " > [%s] %s (%s)\n", f->film->year, f->film->title, f->film->genre);
        _send(u->fd, buffer);
        f=f->next;
    }
}

int show_film_valutation(User u, Film f){
    char buffer[MAXBUF];
    if(f!=NULL){
        F_ValutationList f_val = f->film_valutations;
        memset(buffer, '\0', MAXBUF);
        sprintf(buffer, "\n ## VALUTAZIONI DEL FILM %s ##\n", f->title);
        _send(u->fd, buffer);
        while(f_val != NULL){
            sprintf(buffer, " > [%s] Ha commentato:\n\t%s [%d]\n", 
                    f_val->f_valutation->from,
                    f_val->f_valutation->comment, f_val->f_valutation->F_score);
            _send(u->fd, buffer);
            f_val=f_val->next;
        }
        return 1;
    }else{
        sprintf(buffer, "\n ## FILM NON TROVATO ##\n");
        _send(u->fd, buffer);
        return 0;
    }
}

void show_f_val(User u){
    char buffer[MAXBUF];
    memset(buffer, '\0', MAXBUF);
    do{
        _send(u->fd, " > Select Title: ");
        _recv(u->fd, buffer, 1);
    }while(!show_film_valutation(u, find_film(Films,buffer)) && strcmp(buffer,":quit"));
}

void add_val(User u){
    char buffer[MAXBUF];
    int score;
    Film f;
    memset(buffer, '\0', MAXBUF);   
    F_Valutation new_val = (F_Valutation)malloc(sizeof(struct F_Valutation));
    do{
        _send(u->fd, " > Select Film: ");
        _recv(u->fd, buffer, 1);
        f=find_film(Films, buffer);
    }while(!f);
    _send(u->fd, " > Insert Comment: ");
    _recv(u->fd, buffer, 1);
    strcpy(new_val->comment, buffer);
    do{
        _send(u->fd, " > Insert Score [1,5]: ");
        _recv(u->fd, buffer, 1);
        score=atoi(buffer);
    }while(score<1 || score >5);
    new_val->F_score=score;
    new_val->from=u;
    new_val->Comment_avg=0;
    new_val->CommentScores=NULL;
    strcpy(new_val->user, u->username);
    pthread_mutex_lock(&f->val_mutex);
    f->film_valutations=Val_add_to(f->film_valutations, f->title, new_val);
    pthread_mutex_unlock(&f->val_mutex);
    /*pthread_mutex_lock(&filmdb_mutex);
    int filmfile=open("filmdb", O_RDWR|O_CREAT|O_APPEND|O_TRUNC,S_IRUSR,S_IWUSR);
    FilmList ptr=Films;
    while(ptr!=NULL){
        write(filmfile, ptr->film, sizeof(struct Film));
        ptr=ptr->next;
    }
    close(filmfile);
    pthread_mutex_unlock(&filmdb_mutex);
    _info("New film added to database.");*/
}

F_ValutationList Val_add_to(F_ValutationList LVal, char *title,  F_Valutation new_val){
    char buffer[MAXBUF];
    sprintf(buffer, "%s.film_comments", title);
    int filmcomments=open(buffer, O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    if(LVal==NULL){
        LVal = (F_ValutationList)malloc(sizeof(struct F_ValutationList));
        LVal->f_valutation = new_val;
        LVal->next=NULL;
    }else{
        F_ValutationList node = (F_ValutationList)malloc(sizeof(struct F_ValutationList));
        node->f_valutation=new_val;
        node->next=LVal;
        LVal=node;
    }
    write(filmcomments, LVal->f_valutation, sizeof(struct F_Valutation));
    close(filmcomments);
    return LVal;
}

/*void convertSHA1BinaryToCharStr(unsigned char *hash, char *hashstr) {
    int i=0;
    for (;i< sizeof(hash)/sizeof(hash[0]);++i)
        strcat(hashtr,"%02x \n", hash[i]);

}*/
