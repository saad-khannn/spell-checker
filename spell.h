#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>


#define DEFAULT_DICTIONARY "dictionary.txt"
#define DEFAULT_PORT 9999
#define WORKER_THREADS 5

//Structs
struct node{
    char *word;
    int socket_descriptor;
    struct node *next;
};
struct socket_node{
    int socket_descriptor;
    struct socket_node *next;
};
 struct socket_queue{
    struct socket_node *head;
    struct socket_node *tail;
    int size;
};
struct log_queue{
    struct node *head;
    struct node *tail;
    int size;
};
struct socket_queue* initialize_socket_queue(){
    struct socket_queue* Queue = (struct socket_queue*)malloc(sizeof(struct socket_queue));
    Queue->head = NULL;
    Queue->tail = NULL;
    Queue->size = 0;
    return Queue;
}
struct log_queue* initialize_log_queue(){
    struct log_queue* Queue = (struct log_queue*)malloc(sizeof(struct log_queue));
    Queue->head = NULL;
    Queue->tail = NULL;
    Queue->size = 0;
    return Queue;
}
struct socket_queue *socket_queue;
struct log_queue *log_queue;

//Global Variables
int port; //Global port variable
char *dictionary; //Global dictionary file string
char *dictFile[99171]; //99171 words in the dictionary file
int numClients = 0; //Global variable to keep track of clients in server
char *msgGreeting = "Welcome to the Networked Spell Checker! Enter a word to see if it is spelled correctly\n";
char *msgClose = "Thank you for using the Networked Spell Checker! Goodbye!\n";
pthread_cond_t socket_cv_not_empty, socket_cv_not_full, log_cv_not_empty, log_cv_not_full; //Global condition variables
pthread_mutex_t socket_mutex_lock, log_mutex_lock; //Global socket lock and log lock
pthread_t thread_pool[WORKER_THREADS]; //Global array of worker threads

//Prototype Functions
void *main_thread();
void *worker_thread();
void *log_thread();
int search_dictionary(int first, int last, char *dict[], char *word);
struct socket_queue* initialize_socket_queue();
struct log_queue* initialize_log_queue();
int socket_queue_size(struct socket_queue *queue);
int log_queue_size(struct log_queue *queue);
void socket_queue_enqueue(struct socket_queue *queue, int socket_descriptor);
void log_queue_enqueue(struct log_queue *queue, char *str);
int socket_queue_dequeue(struct socket_queue *queue);
char* log_queue_dequeue(struct log_queue *queue);

