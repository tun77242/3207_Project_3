# Project-3-Fall21
Networked Spell Checker
# # CIS 3207 Assignment 3
# Networked Spell Checker
## Project Overview

- In the main function, firstly my program checks if any port number and dictionary file name are given. 
   If not given they will use the assigned default port number 8765 and default dictionary "dictionary.txt".
   Then it initializes all the queues, thread mutex locks, thread conditional variables, threads from workers, threads for log file.
   Then create socket and connection.

- A while loop is used for socket connection. 
   If connection is successful, it created a worker queue, and when it is not full, it adds the client, unlocks and signals unlock for next client.
   Else fails if socket connection is not established.

- char **open_dictionary(char *filename); -> returns a double pointer char to all the words in the file. 
It opens the given file or default file and copies the list of words in the file to compare.

- int open_listenfd(int port); -> This is obtained from the text and material. 
What it does it creates file descriptor for listener and opens the socket descriptor, also sets the socketopt by resetting the server address, setting all of it's bytes to zero and bind() is then called, associating the port number with the socket fd, prepares the socket to accept calls.

- Queue and node are used as structures just like in previous labs

- Enqueue and dequeue functions are used to push and pop off nodes onto and off of queues

- Worker Thread -> handles concurrent threads for each client that was connected to the server. 
   It also creates as many worker threads as specified, all while concurrently working with the server to fulfill each clients request.

   The first while loop in the worker thread loops files through all the user words, consistently checking each word from the dictionary and displays the correct message.
   It goes through the Queue, enqueueing and dequeueing each command as they are typed with the appropriate response or function.
   The next while loop in this thread, communicates the recieve and send for the client socket which will take in the word from the client and check it under the server and at the end, it pushses, unlocks and sends signals.

- Log Thread -> logs from the servers point of view everything every client enters and records all the words each user types, checks whether they are correctly spelled.
   The user see's them, the server records them.
   At the end it unlocks the logfile.

My programs works with multiple clients and identifies the words given by the users correctly. All the results are written into the lof file.

My run with 3 clients : (showing the result from server)

cis-lclient07:~>./spellChecker 7412 dictionary.txt
New client connection established! Client # 4
ca OK
cattt MISSPELLED
hamsa MISSPELLED
 OK
 OK
New client connection established! Client # 5
object OK
objectifying MISSPELLED
New client connection established! Client # 6
humus MISSPELLED
horse OK
cactus MISSPELLED
sand OK
127 MISSPELLED
? MISSPELLED


Client #1.........

cis-lclient07:~>nc 127.0.0.1 7412
Successfully connected to the server!
(Type -1 to exit)
Enter your word here to spellcheck? cat
ca OK
Enter your word here to spellcheck? cattt 
cattt MISSPELLED
Enter your word here to spellcheck? hamsa 
hamsa MISSPELLED
Enter your word here to spellcheck? 
 OK
Enter your word here to spellcheck?  
 OK
Enter your word here to spellcheck? cactus 
cactus MISSPELLED


Client #2...........

cis-lclient07:~>nc 127.0.0.1 7412
Successfully connected to the server!
(Type -1 to exit)
Enter your word here to spellcheck? object 
object OK
Enter your word here to spellcheck? objectifying 
objectifying MISSPELLED
Enter your word here to spellcheck? 1278
127 MISSPELLED
Enter your word here to spellcheck? ?   
? MISSPELLED


Client #3 .........

cis-lclient07:~>nc 127.0.0.1 7412
Successfully connected to the server!
(Type -1 to exit)
Enter your word here to spellcheck? humus 
humus MISSPELLED
Enter your word here to spellcheck? horse 
horse OK
Enter your word here to spellcheck? -1
Server connection closed!

LogFile :--------

ca OK
cattt MISSPELLED
hamsa MISSPELLED
 OK
 OK
object OK
objectifying MISSPELLED
humus MISSPELLED
horse OK
cactus MISSPELLED
sand OK
127 MISSPELLED
? MISSPELLED