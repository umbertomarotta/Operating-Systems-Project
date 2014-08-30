#ifndef ServerClient_library_h
#define ServerClient_library_h
#include <pthread.h>
#define MAXBUF 1024
#define MAX_PAGE_BUFF 5


typedef struct UserList* UserList;
typedef struct User* User;
typedef struct FilmList* FilmList;
typedef struct Film* Film;
typedef struct F_ValutationList* F_ValutationList;
typedef struct F_Valutation* F_Valutation;
typedef struct C_Valutation* C_Valutation;
typedef struct C_ValutationList* C_ValutationList;
typedef struct notification* notifications;



struct User{
    char username[65];
    char name[65];
    char surname[65];
    char password[65];
    int fd;
    int noti_count;
    int avg_count;
    int part_avg;
    float u_average;
    int is_on;
    notifications notif;
    pthread_mutex_t u_rate_mutex;
    pthread_mutex_t u_notif_mutex;
};



struct UserList{
    User user;
    struct UserList *next;
};

struct Film{
    char title[33];
    char year[17];
    char genre[17];
    float f_avg;
    int f_part_avg;
    int f_avg_count;
    F_ValutationList film_valutations;
    pthread_mutex_t val_mutex;
};

struct FilmList{
    Film film;
    struct FilmList *next;
};

struct F_Valutation{
    int id;
    char comment[121];
    User from;
    char user[65];
    char date[65];
    int F_score;
    float Comment_avg;
    C_ValutationList CommentScores;
    pthread_mutex_t comm_vote_mutex;
};

struct F_ValutationList{
    F_Valutation f_valutation;
    struct F_ValutationList *next;
};

struct C_Valutation{
    int C_score;
    char user[65];
    time_t tim;
};

struct C_ValutationList{
    C_Valutation c_valutation;
    struct C_ValutationList *next;
};

struct notification{
    int id;
    F_Valutation rec;
    char title[33];
    struct notification* next;
};

extern UserList Users;
extern FilmList Films;
extern pthread_mutex_t users_mutex;
extern pthread_mutex_t userdb_mutex;
extern pthread_mutex_t filmdb_mutex;
extern const char hello[MAXBUF];
extern const char menu[MAXBUF];
extern int logfile;
extern User SERVER;

void _send(int fd, const void* buffer);
void _recv(int fd, void* buffer, int be_string);
int _infoUser(char *buffer, char *user);
int _info(char *buffer);
int _error(char *buffer);
void update_u_db();
void manage_user(int fd, int registration);
User find_username(UserList U, char *new_user);
UserList U_add_to(UserList U, User new_user);
UserList remove_from(UserList U, User user);
void show_online_users(User user);
void F_add_to(Film new_film);
void show_film(User user);
void add_film(User user);
Film find_film(char *f_title);
int show_film_valutation(User u, Film f);
void show_f_val(User u);
void add_val(User u, Film f);
void Val_add_to(F_ValutationList *LVal, char* title, F_Valutation new_val);
F_Valutation find_valutation(F_ValutationList, int);
void vote_comment(User u, F_Valutation f, char *title);
void C_Val_add_to(C_ValutationList *CVal, C_Valutation new_vote, int id, char *title);
C_Valutation find_vote(C_ValutationList, User u);
int check_ten_minutes(time_t now);
UserList create_ulist_unique(F_ValutationList Head);
UserList u_enqueue(UserList Head, User u);
void notify_users(User u, Film f, F_Valutation val);
void add_notif_to(notifications *Head, char* title, F_Valutation val);
void free_u_list(UserList *u);
int show_notifications(User u);
void free_notif_list(notifications *notif);

#endif
