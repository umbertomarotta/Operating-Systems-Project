#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#define N 1024

#define DEBAG 0

#if defined POSIX
#define CLEARSCR system ( "clear" )
#elif defined MSDOS || defined _WIN32
#define CLEARSCR system ( "cls" )
#else
#define CLEARSCR system ( "clear" )
#endif

void clear_screen(){
    CLEARSCR;
}

void flush_buffer(char buffer[]){
    int i;
    for (i=0; i<N; i++){
        buffer[i] = '\0';
    }
    return;
}

void aspetta(double secondi){
    time_t tim = time(NULL);
    while(difftime(time(NULL),tim) < secondi);
}

int c_fd; // Socket global variable

void sig_func(int sig) {
    printf("\n Caught signal %d \n Closing the connection... \n Bye. \n", sig);
    close(c_fd);
    exit(0);
}

void ffflush(){
    while (getchar() != '\n');
    return;
}

//int main(int argc, const char * argv[])
int main()
{
    //Hardcoded parameters
    int argc = 3;
    char* argv[3];
    argv[2] = "4005";
    argv[1] = "localhost";

    long num;
    int portno;
    struct sockaddr_in sin;
    struct hostent *hp;
    char buffer[N];

    if (argc != 3) {
        printf("Usage: HOST PORT");
        exit(1);
    }

    if ((hp = gethostbyname(argv[1])) == 0) {
        perror("Error: ");
        exit(1);
    }

    c_fd = socket(AF_INET, SOCK_STREAM, 0);
    portno = atoi(argv[2]);
    sin.sin_family = AF_INET;
    memcpy((char *)&sin.sin_addr, (char *)hp->h_addr, hp->h_length);
    sin.sin_port = htons(portno);

    if (connect(c_fd, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
        printf("Connection failed \n");
        return 1;
    }

    // Register the signal handler
    signal(SIGINT, sig_func);

    // Socket read/write loop interaction
    while (1) {
        if(DEBAG) printf("WAITNG\n");
        num = read(c_fd, buffer, sizeof(buffer));
        if (num <=0) {
            printf("Connection closed.");
            break;
        }
        buffer[num] = '\0';
        
        printf("%s", buffer);
        
        bzero(buffer, N);
        fgets(buffer, N, stdin);
        size_t len = strlen(buffer);
//        while (len == 1 && buffer[0] == '\n') {
//            printf(" > ");
//            bzero(buffer, N);
//            fgets(buffer, N, stdin);
//            len = strlen(buffer);
//        }
        if (buffer[0] == '\n') buffer[0] = '0';
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        if(DEBAG) printf("SENDING [%s]\n", buffer);
        if((send(c_fd, buffer, strlen(buffer), 0)) == -1) {
            printf("Failure sending message \n");
            close(c_fd);
            exit(1);
        }
        if(DEBAG) printf("SENT [%s]\n", buffer);
        flush_buffer(buffer);
        //aspetta(2);
    }
    close(c_fd);
    return 0;
}

