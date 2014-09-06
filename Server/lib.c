#include "lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#define DEBAG 1

#define IDLE 0
#define SENDING 1
#define RECEIVING 2

int status[5000];

// SAFER SEND AND RECV
void _send(int fd, const void* buffer) {
    // We will close the connection if the buffer received is lte 0
    if (status[fd] == SENDING){
        char trash[MAXBUF+1];
        if(DEBAG) printf("WAITNG TRASH\n");
        long char_read = recv(fd, trash, MAXBUF, 0);
        if(DEBAG) printf("TRASHED\n");
        if (char_read <= 0){
            status[fd] = IDLE;
            close(fd);
            if(DEBAG) printf("CLOSING\n");
            pthread_exit(0);
        }
    }
    if(DEBAG) printf("SENDING\n");
    if (strcmp(buffer, "") == 0){
        char string[] = "\nNone\n";
        if (send(fd, string, strlen((char*)string), 0) <= 0) {
            close(fd);
            if(DEBAG) printf("CLOSING\n");
            pthread_exit(0);
        };
    }
    else{
        if (send(fd, buffer, strlen((char*)buffer), 0) <= 0) {
            close(fd);
            if(DEBAG) printf("CLOSING\n");
            pthread_exit(0);
        };
    }
    if(DEBAG) printf("SENT\n");
    status[fd] = SENDING;
}

void flush_buffer(char buffer[]){
    int i;
    for (i=0; i<MAXBUF+1; i++){
        buffer[i] = '\0';
    }
    return;
}

void _recv(int fd, char buffer[], int be_string) {
    if (status[fd] == RECEIVING){
        if(DEBAG) printf("SENDING TRASH\n");
        send(fd, "\0", strlen((char*)"\0"), 0);
        if(DEBAG) printf("SENT\n");
    }
    //_send(fd, "TELLME");
    memset(buffer, 0, MAXBUF);
    flush_buffer(buffer);
    if(DEBAG) printf("WAITNG\n");
    long char_read = recv(fd, buffer, MAXBUF, 0);
    if(DEBAG) printf("GOT [%s]\n", buffer);
    if (char_read <= 0){
        status[fd] = IDLE;
        close(fd);
        pthread_exit(0);
    }
    
    // Use this only if you know that the message is not a string
    // AND you WANT a string
    if (be_string == 0) {
        ((char*)(buffer))[char_read] = '\0';
    }
    status[fd] = RECEIVING;
}

void manage_user(int fd, int registration) {
    status[fd] = IDLE;
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
        new_user->notif=NULL;
        pthread_mutex_init(&new_user->u_rate_mutex,NULL);
        pthread_mutex_init(&new_user->u_notif_mutex, NULL);


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
            _send(fd, " > Password: ");
            _recv(fd, buffer, 1);
        }while(user==NULL || strcmp(user->password, buffer)!=0);
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
        flush_buffer(buffer);
        _recv(user->fd, buffer, 1);
        printf("REC [%s]\n", buffer);
        while (atoi(buffer) <= 0 || atoi(buffer) > 4) {
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
                show_notifications(user);
                break;
            }
            case 4: {
                user->is_on=0;
                update_u_db();
                return;
                    }
            case 666: {
                user->is_on=0;
                _infoUser("User logged out.", user->username);
                return;
            }
            default:
                break;
        }
    } while (choice > 0 && choice < 5);
    user->is_on=0;
    _infoUser("User logged out.", user->username);
}

void update_u_db(){
    UserList u = Users;
    pthread_mutex_lock(&userdb_mutex);
    int userdb=open("userdb",O_RDWR|O_CREAT|O_APPEND|O_TRUNC, S_IRUSR | S_IWUSR);
    while(u!=NULL){
        write(userdb, u->user, sizeof(struct User));
        u=u->next;
    }
    close(userdb);
    pthread_mutex_unlock(&userdb_mutex);
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
    return NULL;
}

Film find_film(char *f_title){
    FilmList F=Films;
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
    new_film->f_avg=0;
    new_film->f_part_avg=0;
    new_film->f_avg_count=0;
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
    char aux[129];
    bzero(aux, 129);
    sprintf(aux, "\n## ONLINE USERS ##\n");
    strcat(buffer, aux);
    while(u != NULL){
        if(u->user->is_on == 1)
            sprintf(aux, " > %s Rating: [%.2f/5]\t[ONLINE]\n",
                    u->user->username,
                    u->user->u_average); 
        else sprintf(aux, " > %s Rating: [%.2f/5]\t[OFFLINE]\n",
                u->user->username,
                u->user->u_average);
        strcat(buffer, aux);       
        u=u->next;
    }
    _send(user->fd, buffer);
}

void show_film(User u){
    const char f_menu[] = "\n 1. Mostra Commenti \n 2. Aggiungi Film \n 3. Esci \n > ";
    FilmList f = Films;
    char buffer[MAXBUF];
    char aux[129];
    memset(buffer, '\0', MAXBUF);
    sprintf(buffer, "\n ## ELENCO FILM ##\n");
    //_send(u->fd, buffer);
    while(f != NULL){
        sprintf(aux, " > [%s] %s (%s) Rating: [%.2f/5]\n", 
                f->film->year, 
                f->film->title, 
                f->film->genre,
                f->film->f_avg);
        //_send(u->fd, buffer);
        strcat(buffer, aux);
        f=f->next;
    }
    _send(u->fd, buffer);
    int choice;
    do{
        memset(buffer, '\0', MAXBUF);
        sprintf(buffer, "\n## RATING SYSTEM MENU > FILM ##\n");
        strcat(buffer, f_menu);
        _send(u->fd, buffer);
        _recv(u->fd, buffer, 1);
        if(DEBAG) printf("REC [%s]\n", buffer);
        
        while (atoi(buffer) <= 0 || atoi(buffer) > 3) {
            _send(u->fd, " > ");
            _recv(u->fd, buffer, 1);
        }
        choice = atoi(buffer);
        switch (choice) {
            case 1: {
                show_f_val(u);
                }
                break;
            case 2: {       
                add_film(u);
                }
                break;
            case 3: 
                return;
                
            default:
                break;
        }
    }while(choice>0 && choice <3);
}

F_Valutation find_valutation(F_ValutationList Head, int id){
    F_ValutationList Lval = Head;
    while(Lval!=NULL){
        if(Lval->f_valutation->id == id)
            return Lval->f_valutation;
        Lval=Lval->next;
    }
    return NULL;
}

void vote_comment(User u, F_Valutation Valutation, char *title){
    C_Valutation c = find_vote(Valutation->CommentScores, u);
    if(c!=NULL)
        if(difftime(time(NULL), c->tim)<600){
            _send(u->fd," > You must wait 10 minutes to rate this comment. ");
            return;
        }
    char buffer[MAXBUF];
    int score;
    memset(buffer, '\0', MAXBUF);   
    C_Valutation new_vote = (C_Valutation)malloc(sizeof(struct C_Valutation));
    do{
        _send(u->fd, " > Insert Score [0,5]: ");
        _recv(u->fd, buffer, 1);
        score=atoi(buffer);
    }while(score<0 || score >5);
    new_vote->C_score=score;
    strcpy(new_vote->user,u->username);
    new_vote->tim=time(NULL);
    pthread_mutex_lock(&Valutation->comm_vote_mutex);
    pthread_mutex_lock(&Valutation->from->u_rate_mutex);
    Valutation->from->avg_count++;
    Valutation->from->part_avg += score;
    Valutation->from->u_average = (float)Valutation->from->part_avg/Valutation->from->avg_count;
    pthread_mutex_unlock(&Valutation->from->u_rate_mutex);
    C_Val_add_to(&(Valutation->CommentScores), new_vote, Valutation->id, title);
    pthread_mutex_unlock(&Valutation->comm_vote_mutex);
    _infoUser("User voted a comment", u->username);
    update_u_db();
}

int show_film_valutation(User u, Film f){
    char buffer[4096];
    char b_aux[MAXBUF];
    const char c_menu[]="\n 1. Visualizza altri \n 2. Vota Recensione \n 3. Esci \n > ";
    int i=0;
    F_Valutation choice=NULL;
    bzero(b_aux, MAXBUF);
    bzero(buffer, 4096);
    F_ValutationList f_val = f->film_valutations;
    sprintf(b_aux, "\n ## COMMENTI DEL FILM %s ##\n", f->title);
    strcat(buffer, b_aux);
    while(f_val != NULL && f_val->f_valutation != NULL){
        sprintf(b_aux, "\n %02d [%s] [%s] Ha commentato:\n\t%s [%d]\n",
                f_val->f_valutation->id,
                f_val->f_valutation->date,
                f_val->f_valutation->user,
                f_val->f_valutation->comment, 
                f_val->f_valutation->F_score);
        strcat(buffer, b_aux);
        ++i; 
        if(i % MAX_PAGE_BUFF == 0){
            sprintf(b_aux, c_menu);
            strcat(buffer, b_aux);
            _send(u->fd, buffer);
            _recv(u->fd, buffer, 1);
            if(atoi(buffer)==1){
                bzero(buffer, 4096);
                bzero(b_aux, MAXBUF);
                continue;
            }
            else if(atoi(buffer)==2){
                do{
                    _send(u->fd, "Enter 'back' to stop voting or Select comment ID: ");
                    _recv(u->fd, buffer, 1);
                    if(strcmp(buffer, "back")==0) break;
                    choice=find_valutation(f->film_valutations, atoi(buffer));
                    if(choice) vote_comment(u, choice, f->title);
                }while(1);
                continue;
            }
            else return 1;
        }
        f_val=f_val->next;
    }
    strcat(buffer, "\n 1. Vota Recensione \n 2. Esci \n > ");
    _send(u->fd, buffer);
    _recv(u->fd, buffer, 1);
    if(atoi(buffer)==1)
        do{
            _send(u->fd, "Enter 'back' to stop voting or Select comment ID: ");
            _recv(u->fd, buffer, 1);
            if(strcmp(buffer, "back")==0) break;
            choice=find_valutation(f->film_valutations, atoi(buffer));
            if(choice) vote_comment(u, choice, f->title);
        }while(1);
    return 1;

}



void show_f_val(User u){
    int fd = u->fd;
    char buffer[MAXBUF];
    Film f=NULL;
    const char v_menu[]= "\n 1. Commenta Film \n 2. Esci \n > ";
    memset(buffer, '\0', MAXBUF);
    do{
        _send(fd, " > Select Title: ");
        _recv(fd, buffer, 1);
        f=find_film(buffer);
    }while(f==NULL && strcmp(buffer,":quit")); 
    int choice;
    show_film_valutation(u, f);
    do{
        memset(buffer, '\0', MAXBUF);
        sprintf(buffer, "\n## RATING SYSTEM MENU > FILM > COMMENTI [%s] ##", f->title);
        strcat(buffer, v_menu);
        _send(fd, buffer);
        _recv(fd, buffer, 1);
        while (atoi(buffer) <= 0 || atoi(buffer) > 2) {
            _send(fd, " > ");
            _recv(fd, buffer, 1);
        }
        choice = atoi(buffer);
        switch (choice) {
            case 1: {
                add_val(u, f);
                }
                break;
            case 2: {        
                return;
                }
                
            default:
                break;
        }
    }while(choice<0 || choice >2);
}

void add_val(User u, Film f){
    char buffer[MAXBUF];
    char date[80];
    time_t now = time(NULL);
    struct tm ts = *localtime(&now);
    strftime(date, sizeof(date), "%Y-%m-%d %H:%M", &ts);
    int score;
    memset(buffer, '\0', MAXBUF);   
    F_Valutation new_val = (F_Valutation)malloc(sizeof(struct F_Valutation));
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
    pthread_mutex_init(&new_val->comm_vote_mutex, NULL);
    strcpy(new_val->date, date);
    new_val->CommentScores=NULL;
    strcpy(new_val->user, u->username);
    pthread_mutex_lock(&f->val_mutex);
    f->f_avg_count++;
    f->f_part_avg += score;
    f->f_avg = (float) f->f_part_avg / f->f_avg_count;
    Val_add_to(&(f->film_valutations), f->title, new_val);
    pthread_mutex_unlock(&f->val_mutex);
    _infoUser("User submitted a new comment", u->username);
    _info("New comment added to database.");
    notify_users(u, f, new_val);
}

void Val_add_to(F_ValutationList *LVal, char *title,  F_Valutation new_val){
    char buffer[MAXBUF];
    sprintf(buffer, "%s.film_comments", title);
    int filmcomments=open(buffer, O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    if(*LVal==NULL){
        *LVal = (F_ValutationList)malloc(sizeof(struct F_ValutationList));
        (*LVal)->f_valutation = new_val;
        (*LVal)->next=NULL;
        (*LVal)->f_valutation->id=0;
    }else{
        F_ValutationList node = (F_ValutationList)malloc(sizeof(struct F_ValutationList));
        node->f_valutation=new_val;
        node->next=*LVal;
        *LVal=node;
        (*LVal)->f_valutation->id=(*LVal)->next->f_valutation->id+1;
    }
    write(filmcomments, (*LVal)->f_valutation, sizeof(struct F_Valutation));
    close(filmcomments);
}

void C_Val_add_to(C_ValutationList *CVal, C_Valutation new_vote, int id, char *title){
    char buffer[MAXBUF];
    sprintf(buffer, "%s.%d.rec_vote", title, id);
    int rec_votations=open(buffer, O_RDWR|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    if(*CVal==NULL){
        *CVal=(C_ValutationList)malloc(sizeof(struct C_ValutationList));
        (*CVal)->c_valutation = new_vote;
        (*CVal)->next=NULL;
    }else{
        C_ValutationList node=(C_ValutationList)malloc(sizeof(struct C_ValutationList));
        node->c_valutation=new_vote;
        node->next=*CVal;
        *CVal=node;
    }    
    write(rec_votations, new_vote, sizeof(struct C_Valutation));
    close(rec_votations);
}

C_Valutation find_vote(C_ValutationList CVal, User u){
    C_ValutationList cval=CVal;
    while(cval!=NULL){
        if(strcmp(cval->c_valutation->user, u->username)==0)
            return cval->c_valutation;
        cval=cval->next;
    }
    return NULL;
}

int check_ten_minutes(time_t t_val){
    time_t now = time(NULL);
    if(difftime(t_val, now)>600)
        return 1;
    else return 0;
}

UserList create_ulist_unique(F_ValutationList Head){
    F_ValutationList LVal = Head;
    UserList u_unique=NULL;
    while(LVal!=NULL && LVal->f_valutation!=NULL){
        u_unique=u_enqueue(u_unique, LVal->f_valutation->from);
        LVal=LVal->next;
        fprintf(stderr, "create_ulist\n");
    }
    return u_unique;
}

UserList u_enqueue(UserList Head, User u){
    UserList ptr=Head;
    if(ptr==NULL){
        ptr=(UserList)malloc(sizeof(struct UserList));
        ptr->user=u;
        return ptr;
    }else{
        if(strcmp(u->username, ptr->user->username)==0)
            return ptr;
        else{
            ptr->next=u_enqueue(ptr->next, u);
        }
        return ptr;
    }
}


void add_notif_to(notifications *Head, char *title, F_Valutation val){
    if(*Head==NULL){
        *Head=(notifications)malloc(sizeof(struct notification));
        (*Head)->rec=val;
        (*Head)->next=NULL;
        (*Head)->id=0;
        strcpy((*Head)->title, title);
    }else{
        notifications ptr=(notifications)malloc(sizeof(struct notification));
        ptr->rec=val;
        strcpy(ptr->title, title);
        ptr->next=*Head;
        *Head=ptr;
        (*Head)->id=(*Head)->next->id+1;
    }
}

void notify_users(User u, Film f, F_Valutation val){
    fprintf(stderr, "riesco a creare sta lista?\n");
    UserList unique_user_list=create_ulist_unique(f->film_valutations);
    UserList u_ptr=unique_user_list;
    fprintf(stderr, "bo\n");
    while(u_ptr!=NULL){
        if(strcmp(u->username, u_ptr->user->username)!=0){
            pthread_mutex_lock(&u_ptr->user->u_notif_mutex);
            add_notif_to(&u_ptr->user->notif, f->title, val);
            u_ptr->user->noti_count++;
            pthread_mutex_unlock(&u_ptr->user->u_notif_mutex);
        }
        u_ptr=u_ptr->next;
    }
    fprintf(stderr, "gnac\n");
   free_u_list(&unique_user_list);
   fprintf(stderr, "ci arrivo qua?\n");
}

void free_u_list(UserList *u){
    if(*u!=NULL){
        free_u_list(&(*u)->next);
        (*u)->next=NULL;
        free(*u);
    }
}

F_Valutation find_notif(notifications Head, int id){
    notifications LNotif = Head;
    while(LNotif){
        if(LNotif->id==id)
            return LNotif->rec;
        LNotif=LNotif->next;
    }
    return NULL;
}

int show_notifications(User u){
    if (u->noti_count <=0) return 1;
    //else if(u->notif=NULL) { u->noti_count = 0; return; }
    char buffer[MAXBUF];
    char b_aux[129];
    const char c_menu[]="\n 1. Visualizza altri \n 2. Vota Recensione \n 3. Esci \n > ";
    //F_Valutation choice = NULL;
    bzero(buffer, MAXBUF);
    bzero(b_aux, 129);
    notifications LNotif = u->notif;
    sprintf(b_aux, "\n ## NOTIFICHE ##\n");
    strcat(buffer, b_aux);
    while(LNotif != NULL && LNotif->rec != NULL){
        sprintf(b_aux, "\n %02d [%s] [%s] Ha commentato il film [%s]:\n\t%s [%d]\n",
                LNotif->id,
                LNotif->rec->date,
                LNotif->rec->user,
                LNotif->title,
                LNotif->rec->comment, 
                LNotif->rec->F_score);
        strcat(buffer, b_aux);
        strcat(buffer, c_menu);
        _send(u->fd, buffer);
        _recv(u->fd, buffer, 1);
        if(atoi(buffer)==2){
            vote_comment(u, LNotif->rec, LNotif->title);
        }else if(atoi(buffer)==3) break;
        LNotif=LNotif->next;
        bzero(buffer, MAXBUF);
    }
    pthread_mutex_lock(&u->u_notif_mutex);
    u->noti_count=0;
    if(u->notif!=NULL) free_notif_list(&u->notif);
    u->notif=NULL;
    pthread_mutex_unlock(&u->u_notif_mutex);
    return 1;
}

void free_notif_list(notifications *notif){
    if(*notif!=NULL){
        free_notif_list(&(*notif)->next);
        (*notif)->next=NULL;
        free(*notif);
    }
}
