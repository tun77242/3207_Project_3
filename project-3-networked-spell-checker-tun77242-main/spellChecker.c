#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define BUFFER_SIZE 256
#define DICTIONARY "dictionary.txt"
#define LOG_FILE "logFile.txt"
#define PORT_NUMBER 8765
#define DICTIONARY_LENGTH 99171
#define EXIT -1
#define MAX_SIZE 1000
#define NUM_WORKERS 5

char **dict_list;

//locks
pthread_mutex_t worker_queue_lock;
pthread_mutex_t log_queue_lock;
pthread_mutex_t logfile_lock;

//conditional variables
pthread_cond_t cond1;
pthread_cond_t cond2;
pthread_cond_t cond3;
pthread_cond_t cond4;

void *workerThread(void *params);
void *logThread(void *params);

struct Queue *worker_queue;
struct Queue *log_queue;

typedef struct Node
{
    struct sockaddr_in client;
    int client_socket;
    char *word;
    struct Node *next;
}Node;

typedef struct Queue
{
    Node *front;
    int queue_size;
}Queue;

Queue *createQueue();
Node *createNode(struct sockaddr_in, char *, int);
void enqueue(Queue *, struct sockaddr_in, char *, int);
Node *dequeue(Queue *);

char **open_dictionary(char *);     //reads dictionary
int open_listenfd(int);             //opens listener

int main(int argc, char **argv)
{
    int portNumber;
    char *dictionary;

    //checks if port and dictionary file are given
    if(argc == 1)
    {
        portNumber = PORT_NUMBER;
        dictionary = DICTIONARY;
    }
    else if(argc == 2)
    {
        portNumber = atoi(argv[1]);
        dictionary = DICTIONARY;
    }
    else
    {
        portNumber = atoi(argv[1]);
        dictionary = argv[2];
    }

        dict_list = open_dictionary(dictionary);
    
    //queues initialized for client buffers
    worker_queue = createQueue();
    log_queue = createQueue();

    pthread_mutex_init(&worker_queue_lock, NULL);
    pthread_mutex_init(&log_queue_lock, NULL);
    pthread_mutex_init(&logfile_lock, NULL);

    pthread_cond_init(&cond1, NULL);
    pthread_cond_init(&cond2, NULL);
    pthread_cond_init(&cond3, NULL);
    pthread_cond_init(&cond4, NULL);

    pthread_t workers[NUM_WORKERS];

    int i;
    for(i = 0; i < NUM_WORKERS; i++)
    {
        pthread_create(&workers[i], NULL, &workerThread, NULL);
    }

    pthread_t logger_thread;
    pthread_create(&logger_thread, NULL, &logThread, NULL);

    struct sockaddr_in client;      //socket
    socklen_t client_size = sizeof(struct sockaddr_in);
    int connection_socket = open_listenfd(portNumber);      //socket connection

    char *connect_done = "Successfully connected to the server!\n(Type -1 to exit)\n";
    char *buffer_full = "Jobs in worker buffer is full! Try again later.\n";

    while(1)            //checks if socket connects
    {
        int client_socket = accept(connection_socket, (struct sockaddr *)&client, &client_size);
      
        if(client_socket == -1)     //if not
        {
            printf("Failed to connect the socket %d!\n", client_socket);
            continue;
        }

        pthread_mutex_lock(&worker_queue_lock);
      
        if(worker_queue->queue_size >= MAX_SIZE)
        {
            send(client_socket, buffer_full, strlen(buffer_full), 0);
            pthread_cond_wait(&cond2, &worker_queue_lock);
        }

        printf("New client connection established! Client # %d\n", client_socket);
        send(client_socket, connect_done, strlen(connect_done), 0);

        enqueue(worker_queue, client, NULL, client_socket);     //pushed
        pthread_mutex_unlock(&worker_queue_lock);       //unlocked
        pthread_cond_signal(&cond1);        //signalled
    }
    
    return 0;
}

char **open_dictionary(char *filename)      //opens dictionary to compare
{
    FILE *fd;
    char **output = malloc(DICTIONARY_LENGTH *sizeof(char *) + 1);
    char line [BUFFER_SIZE];
    int index = 0;

    fd = fopen(filename, "r");
    if(fd == NULL)
    {
        printf("Failed to open the given dictionary file!\n");
        exit(1);
    }

    while((fgets(line, BUFFER_SIZE, fd)) != NULL)
    {
        output[index] = (char *) malloc(strlen(line) *sizeof(char *) + 1);
        int temp = strlen(line) - 2;
        line[temp] = '\0';
        strcpy(output[index], line);
        index++;
    }
    
    fclose(fd);
    return output;
}

int open_listenfd(int portNumber)       //listens, opens socket des
{
    int listenfd, optval = 1;
    struct sockaddr_in serverAddress;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return -1;
    }

    if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0)
    {
        return -1;
    }

    bzero((char *) &serverAddress, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons((unsigned short)portNumber);

    if(bind(listenfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
    {
        return -1;
    }

    if(listen(listenfd, 20) < 0)
    {
        return -1;
    }
    
    return listenfd;
}

Queue *createQueue()
{
    Queue *temp = (Queue *)malloc(sizeof(Queue));
    temp->front = NULL;
    temp->queue_size = 0;
    return temp;
}

Node *createNode(struct sockaddr_in client, char *word, int socket)
{
    Node *temp = (Node *)malloc(sizeof(Node));
    temp->client = client;
    
    if(word == NULL)
    {
        temp->word = word;
    }
    else
    {
        temp->word = malloc(sizeof(char *) *strlen(word) + 1);
      
        if(temp->word == NULL)
        {
            printf("Failed to allocate memory for Node!\n");
            exit(1);
        }
      
        strcpy(temp->word, word);
    }

    temp->next = NULL;
    temp->client_socket = socket;
    return temp;
}

void enqueue(Queue *queue, struct sockaddr_in client, char *word, int socket)       //pushed node onto queue
{
    Node *temp = createNode(client, word, socket);

    if (queue->queue_size == 0)
    {
        queue->front = temp;
    }
    else
    {
        Node *head = queue->front;
        
        while(head->next != NULL)
        {
            head = head->next;
        }
        
        head->next = temp;
    }

    queue->queue_size++;
    return;
}

Node *dequeue(Queue *queue)         //takes first node off of queue
{
    if(queue->front == NULL)
    {
        queue->queue_size = 0;
        return NULL;
    }

    Node *temp = queue->front;
    queue->front = queue->front->next;
    queue->queue_size--;
    
    free(queue->front);
    return temp;
}

void *workerThread(void *params)
{
    char *prompt_msg = "Enter your word here to spellcheck? ";
    char *error_msg = "Error! Enter your word again!\n";
    char *close_msg = "Server connection closed!\n";

    while(1)       //checks each word for all given words by user
    {
        pthread_mutex_lock(&worker_queue_lock);
        if (worker_queue->queue_size <= 0) 
        {
            pthread_cond_wait(&cond1, &worker_queue_lock);
        }

        Node *job = dequeue(worker_queue);
        pthread_mutex_unlock(&worker_queue_lock);
        pthread_cond_signal(&cond2);

        int client_socket = job->client_socket;     //gets next client socket

        while(1)       //takes word from client and check under server by receiving and sending to client socket
        {
            char receive_buff[BUFFER_SIZE] = "";
            send(client_socket, prompt_msg, strlen(prompt_msg), 0);
            int bytes_returned = recv(client_socket, receive_buff, BUFFER_SIZE, 0);

            if(bytes_returned <= -1)
            {
                send(client_socket, error_msg, strlen(error_msg), 0);
                continue;
            }
            else if (atoi(&receive_buff[0]) == EXIT) 
            {
                send(client_socket, close_msg, strlen(close_msg), 0);
                close(client_socket);
                break;
            }
            else
            {
                char *recv = receive_buff;
                recv[strlen(recv) - 1] = '\0';
                recv[bytes_returned - 2] = '\0';

                char *result = " MISSPELLED\n";
                int i;
                for (i = 0; i < DICTIONARY_LENGTH; i++)
                {
                    if (strcmp(recv, dict_list[i]) == 0)
                    {
                        result = " OK\n";
                        break;
                    }
                }

                char *p = recv;
                strcat(p, result);
                printf("%s", p);
                send(client_socket, p, strlen(p), 0);

                struct sockaddr_in client = job->client;

                if(log_queue->queue_size >= MAX_SIZE) 
                {
                    pthread_cond_wait(&cond4, &log_queue_lock);
                }

                enqueue(log_queue, client, p, client_socket);
                pthread_mutex_unlock(&log_queue_lock);
                pthread_cond_signal(&cond3);
            }
        }
    }
}

void *logThread(void *params)       //gets all words and record them from every client
{  
    while(1)
    {
        pthread_mutex_lock(&log_queue_lock);
        
        if (log_queue->queue_size <= 0)
        {
            pthread_cond_wait(&cond3, &log_queue_lock);
        }
    
        Node *node = dequeue(log_queue);
        char * word = node->word;

        pthread_mutex_unlock(&log_queue_lock);
        pthread_cond_signal(&cond4);

        if (word == NULL)
        {
            continue;
        }

        pthread_mutex_lock(&logfile_lock);          //locks
        FILE *log_file = fopen(LOG_FILE, "a");      //opens and writes to default log file
        fprintf(log_file, "%s", word);
        fclose(log_file);                           //closed

        pthread_mutex_unlock(&logfile_lock);
    }
}