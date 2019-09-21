# Spell Checker
## Description

A spellchecking program on a server which multiple clients can connect to and send words to be spellchecked.
It utilizes multithreading, synchronization, and network programming. 

The main thread loads the dictionary into memory, creates a pool of worker threads, sets up a network connection, and waits for incoming client connections.

The worker thread acquires the mutex lock. The worker thread lets the socket establish a connection with the client, sends a greeting message to the client, and releases mutex lock. The worker thread waits for the client to send it words to be spellchecked and when it receives words, it checks to see if they're in the dictionary. The worker thread confirms if the word was in the dictionary or not and then prints a "correct" or "incorrect" message to the client. The worker thread continues to receive words until the client enters the 'ESCAPE' key ('^]').

The log thread takes in a word every time it is entered by a client and appends the word and the result of whether or not it's in the dictionary to the "log.txt" output file.  

## Compiling the Program

Compile the program by using the makefile with
```
make
```

or with

```
gcc -pthread -o spell spell.c
```

## Running the Server

Run the program with
```
./spell
```
to use the default port 9999 and default dictionary file "dictionary.txt"

If you use only a different port, then you will use the default dictionary file "dictionary.txt" 
```
./spell [PORT_NUMBER]
```

If you use only a different dictionary file, then you will use the default port 9999
```
./spell [DICTIONARY_FILE]
```

You can also use both a different port and a different dictionary file to not use the default port 9999 and default dictionary file "dictionary.txt"
```
./spell [PORT_NUMBER] [DICTIONARY_FILE]
```

***Examples:*** 
```
./spell 5000 
```
will use the new port 5000 and the default dictionary file "dictionary.txt"

```
./spell words.txt 
```
will use the new dictionary file "words.txt" and the default port 9999 

```
./spell 5000 words.txt
```
will use the new port 5000 and the new dictionary file "words.txt"

## Connecting the Clients

Open a new window after running the program and connect a client to the server with
```
telnet localhost [PORT_NUMBER]
```

You may connect as many clients in as many windows as you want with the command listed above. The port number needs to match the port number of the server - which will be listed on the server window once you run the program. 
The port number will be 9999 by default unless you specify otherwise when you run the server. 

The server window will display how many clients are simultaneously connected to the server. The server will continue accepting words from a client until the client presses the 'ESCAPE' key which will disconnect them from the server and they will not be able to send any more words to the server. After every word that a client sends to the server to be spellchecked, a "log.txt" output file is updated with the word and whether or not the word is correct.
