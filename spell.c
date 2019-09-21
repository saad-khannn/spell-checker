#include "spell.h"

int main(int argc, char* argv[]) {

    port = DEFAULT_PORT;
    dictionary = DEFAULT_DICTIONARY;

    if(argc == 1){ //If no arguments for port or dictionary are entered
		port = DEFAULT_PORT; //Use default port
		dictionary = DEFAULT_DICTIONARY; //Use default dictionary
		}

    else if(argc == 2){ //If an argument for either port or dictionary is entered
		//Need to check to see if the argument is a port or a dictionary
        if(!strcmp(argv[1] + strlen(argv[1]) - 4, ".txt")){ //If the second argument ends in '.txt'
			dictionary = argv[1]; //Set dictionary to dictionary entered as argument
			port = DEFAULT_PORT; //Keep port as default port
		}
		else if(strcmp(argv[1] + strlen(argv[1]) - 4, ".txt")){ //If the second argument doesn't end in '.txt'
		    port = atoi(argv[1]); //Set port to port entered as argument
			dictionary = DEFAULT_DICTIONARY; // Keep dictionary as default dictionary
			}
        }
	else if(argc == 3){ //If two arguments for both port and dictionary are entered
		//Need to check to see which argument is port and which argument is dictionary
		if((!strcmp(argv[1] + strlen(argv[1]) - 4, ".txt")) && (strcmp(argv[2] + strlen(argv[2]) - 4, ".txt"))){ //If first argument ends in '.txt' and second argument doesn't end in '.txt'
            dictionary = argv[1]; //Set dictionary to dictionary entered as first argument
            port = atoi(argv[2]); //Set port to port entered as second argument
            }
		else if((strcmp(argv[1] + strlen(argv[1]) - 4, ".txt")) &&(!strcmp(argv[2] + strlen(argv[2]) - 4, ".txt"))){ //If first argument doesn't end in '.txt' and second argument does end in '.txt'
			port = atoi(argv[1]); //Set port to port entered as first argument
			dictionary = argv[2]; //Set dictionary to dictionary entered as second argument
		}
        else{ //If arguments are incorrect, print error message
			printf("Incorrect Inputs for PORT and DICTIONARY. Please try again.\n");
			return -1;
		}
	}
    else{ //If more than 3 arguments were entered, print error message
		printf("Please enter 3 or less arguments. Please try again.\n");
		return -1;
	}

	if(port < 1024 || port > 65535){
		printf("Port number is either too low(below 1024) or too high(above 65535). Please try again\n");
		return -1;
	}


    printf("Port = %d\n", port);
    printf("Dictionary = %s\n", dictionary);

    socket_queue = initialize_socket_queue(); //Initialize socket queue
    log_queue = initialize_log_queue(); //Initialize log queue

    //Load dictionary file into memory
    FILE *dict;
    dict = fopen(dictionary, "r");
    if(dict == NULL){
        printf("Dictionary could not be opened\n");
    }
    else{
        int i = 0;
        while(!feof(dict)){
            char buf[64];
            fgets(buf, 64, dict);
            char *word = (char*)malloc(sizeof(buf));
            strncpy(word, buf, strcspn(buf, "\r\n"));
            dictFile[i++] = word;
        }
    }
    fclose(dict);

    //Create a worker thread pool
    for(int i = 0; i < WORKER_THREADS; i++){
        int workers = pthread_create(&thread_pool[i], NULL, worker_thread, NULL);
        if(workers != 0){
            puts("Worker threads could not be created");
            exit(1);
            }
        }

    //Create a log thread
    pthread_t log_threads;
    int logs = pthread_create(&log_threads, NULL, log_thread, NULL);
    if (logs != 0){ //checking if error in thread creation
        puts("Log thread could not be created");
        exit(1);
        }

    //Set up a network connection
    int socketfd, new_socket, c;
    struct sockaddr_in server, client;
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0){
        puts("Socket could not be created");
        }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    //Bind socket
    if(bind(socketfd, (struct sockaddr*)&server, sizeof(server)) < 0){
        puts("Bind failed");
        exit(1);
        }
    puts("Bind completed");
    listen(socketfd, 5);

    puts("Waiting for incoming connections . . .");

    //Accept incoming connections from clients
    while(1){
        c = sizeof(struct sockaddr_in);
        new_socket = accept(socketfd, (struct sockaddr*)&client, (socklen_t*)&c);
        if (new_socket < 0) {
            puts("Connection failed");
            exit(1);
            }
        pthread_mutex_lock(&socket_mutex_lock); //Acquire lock to socket queue
        socket_queue_enqueue(socket_queue, new_socket); //Add new connection to socket queue
        pthread_cond_signal(&socket_cv_not_empty); //Signal workers
        pthread_mutex_unlock(&socket_mutex_lock); //Release socket queue lock
        puts("Connection established");
        numClients++;
        if(numClients == 1){
            printf("There is 1 client in the server\n");
            }
        else if(numClients == 0 || numClients > 1){
            printf("There are %d clients in the server\n", numClients);
            }
        }
    return 0;
}

void *worker_thread(){
    while(1){
    pthread_mutex_lock(&socket_mutex_lock); //Acquire lock
    while(socket_queue_size(socket_queue) == 0){ //If socket queue is empty
        pthread_cond_wait(&socket_cv_not_empty, &socket_mutex_lock); //Wait until a connection is made
        }
    int socket = socket_queue_dequeue(socket_queue); //Let socket establish connection with client
    printf("New client!\n");
    send(socket, msgGreeting, strlen(msgGreeting), 0); //Send greeting message
    pthread_mutex_unlock(&socket_mutex_lock); //Release socket queue lock

    char buf[256];
    memset(&buf, 0, sizeof(buf));
    while(recv(socket, buf, 256, 0)){ //Receive word from client
        for(int i = 0; i < strlen(buf); i++){ //
            if(buf[i] == '\r'){
                buf[i] = '\0';
                break;
            }
        }
        char *result;
        int check = search_dictionary(0, 99171, dictFile, buf); //Check if word is in dictionary
        if(check < 0){ //If word not in dictionary
            char msg[] = " is not correct!\n";
            result = malloc(strlen(buf) + strlen(msg) + 1); //Allocate memory for entire string
            strcpy(result, buf); //Copy word received from client into allocated string
            strcat(result, msg); //Concatenate "is not correct" message to word
            }
        else{ //Else, if word is in dictionary
            char msg[] = " is correct!\n";
            result = malloc(strlen(buf) + strlen(msg) + 1); //Allocate memory for entire string
            strcpy(result, buf); //Copy word received from client into allocated string
            strcat(result, msg); //Concatenate "is correct" message to word
            }
        if(buf[0] == 27){ //If ESCAPE key is entered
            numClients--;
            send(socket, msgClose, strlen(msgClose), 0); //Send farewell message
            printf("Client connection closed\n");
            if(numClients == 1){
                printf("There is 1 client in the server\n");
                }
            else if(numClients == 0 || numClients > 1){
                printf("There are %d clients in the server\n", numClients);
                }
            close(socket); //Close client socket
            break;
            }
        write(socket, result, strlen(result));
        pthread_mutex_lock(&log_mutex_lock); //Acquire log queue lock
        log_queue_enqueue(log_queue, result); //Add to log queue
        pthread_cond_signal(&log_cv_not_empty); //Signal
        pthread_mutex_unlock(&log_mutex_lock); //Release lock
        }
    }
}

void *log_thread(){
    while(1){
        FILE *log;
        pthread_mutex_lock(&log_mutex_lock); //Acquire log queue lock
        while(log_queue_size(log_queue) == 0){ //If log queue is empty
            pthread_cond_wait(&log_cv_not_empty, &log_mutex_lock); //Wait until it's not empty
            }
        printf("Client sent word! Log file updated!\n"); //Print every time a word is entered in client
        log = fopen("log.txt", "a"); //Open file in append mode (write mode erases file with every new word)
        if(log == NULL){ //If log file could not be opened
            printf("Log file could not be opened\n"); //Print error message
			exit(1);
        }
        while(log_queue_size(log_queue) != 0){ //While log queue is not empty
            fputs(log_queue_dequeue(log_queue), log); //Write to file
            fputs("\r\n", log); //New line in log file (Everything got printed on same line without this)
            }
        fclose(log); //Close file
        pthread_mutex_unlock(&log_mutex_lock); //Release
        pthread_cond_signal(&log_cv_not_full); //Signal
        }
}

int search_dictionary(int first, int last, char *dict[], char *word){
    int mid = (first + last) / 2;
    if(first > last){
        return -1;
        }
    if(strcmp(dict[mid], word) < 0){ //If word is at an index greater than mid (in the second half of the dictionary)
        return search_dictionary(mid + 1, last, dict, word);
        }
    else if(strcmp(dict[mid], word) > 0){//If word is at an index less than mid (in the first half of dictionary)
        return search_dictionary(first, mid-1, dict, word);
        }
    else{ //Else if word is exactly in the middle
        return mid; //Return middle word
        }
}

int socket_queue_size(struct socket_queue *queue){
    return queue->size;
}

int log_queue_size(struct log_queue *queue){
    return queue->size;
}

void socket_queue_enqueue(struct socket_queue *queue, int socket_descriptor){ //Add to socket queue
    struct socket_node *temp = (struct socket_node*)malloc(sizeof(struct socket_node));
    temp->socket_descriptor = socket_descriptor;
    if (socket_queue_size(queue) == 0){ //If nothing in socket queue (empty)
        queue->head = temp; //Head and
        queue->tail = temp; //Tail will be the same since there's only 1 item in queue
        queue->tail->next = NULL;
        }
    else{ //If something in socket queue
        queue->tail->next = temp; //Add node
        queue->tail = temp; //After tail
        queue->tail->next = NULL; //Of queue
        }
    queue->size++;
}

void log_queue_enqueue(struct log_queue *queue, char *str){ //Add to log queue
    struct node *temp = (struct node*)malloc(sizeof(struct node));
    temp->word = str;
    if(log_queue_size(queue) == 0){ //If nothing in log queue (empty)
        queue->head = temp; //Head and
        queue->tail = temp; //Tail will be the same since there's only 1 item in queue
        queue->tail->next = NULL;
        }
    else{ //If something in log queue
        queue->tail->next = temp; // Add node
        queue->tail = temp; //After tail
        queue->tail->next = NULL; //Of queue
        }
    queue->size++;
}

int socket_queue_dequeue(struct socket_queue *queue){ //Dequeue socket queue
    struct socket_node *temp = (struct socket_node*)malloc(sizeof(struct socket_node));
    temp = queue->head; //Removes head
    queue->head = queue->head->next; //of queue
    queue->size--; //and decrements size of queue
    return temp->socket_descriptor;
}

char* log_queue_dequeue(struct log_queue *queue){ //Dequeue log queue
    struct node *temp = (struct node*)malloc(sizeof(struct node));
    temp = queue->head; //Removes head
    queue->head = queue->head->next; //of queue
    queue->size--; //and decrements size of queue
    return temp->word;
}

