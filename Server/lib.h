#ifndef ServerClient_library_h
#define ServerClient_library_h
#include <pthread.h>
#define MAXBUF 1024


typedef struct UserList* UserList;
typedef struct User* User;
typedef struct FilmList* FilmList;
typedef struct Film* Film;
typedef struct F_ValutationList* F_ValutationList;
typedef struct F_Valutation* F_Valutation;
typedef struct C_Valutation* C_Valutation;
typedef struct C_ValutationList* C_ValutationList;



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
};



struct UserList{
    User user;
    struct UserList *next;
};

struct Film{
    char *title;
    char *year;
    char *genre;
    float f_average;
    F_ValutationList film_valutations;
};

struct FilmList{
    Film film;
    struct FilmList *next;
};

struct F_Valutation{
    char comment[121];
    User from;
    int F_score;
    float Comment_avg;
    C_ValutationList CommentScores;
};

struct F_ValutationList{
    F_Valutation f_valutation;
    struct F_ValutationList *next;
};

struct C_Valutation{
    int C_score;
    User from;
};

struct C_ValutationList{
    C_Valutation c_valutation;
    struct C_ValutationList *next;
};

extern UserList Users;
extern pthread_mutex_t users_mutex;
extern pthread_mutex_t userdb_mutex;

extern pthread_mutex_t film_mutex;
extern const char hello[MAXBUF];
extern const char menu[MAXBUF];
extern int logfile;
extern User SERVER;

void _send(int fd, const void* buffer);
void _recv(int fd, void* buffer, int be_string);
int _infoUser(char *buffer, char *user);
int _info(char *buffer);
int _error(char *buffer);
void manage_user(int fd, int registration);
User find_username(UserList U, char *new_user);
UserList add_to(UserList U, User new_user);
UserList remove_from(UserList U, User user);
void show_online_users(User user);


#endif
