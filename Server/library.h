//
//  library.h
//  ServerClient
//
//  Created by Vince on 8/29/13.
//  Copyright (c) 2013 Vince. All rights reserved.
//

#ifndef ServerClient_library_h
#define ServerClient_library_h
#include <pthread.h>
#define MAXBUF 1024
#define MAXMAP 256
#define is ==
#define not !
#define and &&
#define or ||
#define None NULL
#define is_not !=

// Structures

typedef struct UserType* User;
typedef struct MessageListType* MessageList;
typedef struct UserListType* UserList;
typedef struct MapListType* MapList;

struct MessageListType {
    char message[MAXBUF];
    User from;
    time_t timer;
    int is_private;
    int read;
    struct MessageListType* next;
};

struct UserType {
    char* username;
    char* name;
    char* surname;
    int x;
    int y;
    int rad;
    int fd;
    int noti_count;
    MessageList private_messages;
    MessageList notifications;
};

struct UserListType {
    User info;
    struct UserListType* next;
};

struct MapListType {
    UserList users;
    MessageList messages;
};

// Declaring the global Users list and the map
extern UserList Users;
extern pthread_mutex_t users_mutex;

extern MapList map[MAXMAP][MAXMAP];
extern pthread_mutex_t map_mutex;
extern const char hello[MAXBUF];
extern const char menu[MAXBUF];
extern int logfile;
extern User SERVER;

// Prototypes
void _send(int fd, const void* buffer);
void _recv(int fd, void* buffer, int be_string);
void manage_user(int fd, int registration);
User find_username(UserList U, char *new_user);
UserList add_to(UserList U, User new_user);
void move_user(User user);
void map_user(User user);
void remap_user(User user, int x, int y);
void show_users_available(User user);

// Message Functions
MessageList readMessages(MessageList M, char* buffer, int length);
MessageList add_message_to(MessageList M, char *message, User from, int private);
void send_public_message(User user);
int send_private_message(User user);
void read_public_messages(User user);
void read_private_messages(User user);
void read_notifications(User user);


// Logging functions
int _info(char* buffer);
int _error(char* buffer);
int _infoUser(char* buffer, char* user);
int _errorUser(char* buffer, char* user);

#endif
