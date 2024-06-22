#include <pthread.h>
#include "commons.h"

// Define the size of the movie array
#define MOVIE_SIZE 5

// Define the number of movie options
#define MOVIE_OPTIONS 3

// Structure to hold handler data
struct handlerData {
  int serverSocket; // The server socket file descriptor
  int selectedMovie; // The selected movie index
  struct sockaddr_storage sockaddr; // The socket address storage structure
};

// Global variable to keep track of the number of connected clients
int connectedClients = 0;
pthread_mutex_t mutex;

/**
 * Function to print the number of connected clients.
 * 
 * This function is executed in a separate thread and continuously prints the number of connected clients.
 * It takes a void pointer as an argument, which is casted to an int pointer to access the number of connected clients.
 * The function uses printf to print the number of connected clients and sleeps for 4 seconds before printing again.
 */
void* printClients() {
  while(1) {
    pthread_mutex_lock(&mutex);
    printf("Clientes: %d\n", connectedClients);
    pthread_mutex_unlock(&mutex);
    sleep(4);
  }
}

/**
 * Opens the movie file corresponding to the selected movie.
 *
 * @param selectedMovie The selected movie.
 * @return A pointer to the opened file, or NULL if the file could not be opened.
 */
FILE* openMovieFile(int selectedMovie) {
  switch (selectedMovie) {
    case LORD_OF_THE_RINGS:
      return fopen("movies/lordOfTheRings.txt", "r");
    case THE_GODFATHER:
      return fopen("movies/theGodfather.txt", "r");
    case FIGHT_CLUB:
      return fopen("movies/fightClub.txt", "r");
    default:
      return NULL;
  }
}

/**
 * Handles the UDP client connection.
 * 
 * @param arg The argument passed to the thread function.
 * @return Always returns 0.
 */
void* handleUDPClient(void *arg) {
  struct handlerData *connectionData = (struct handlerData *)arg;
  if (connectionData->selectedMovie <= 0 || connectionData->selectedMovie > MOVIE_OPTIONS) {
    printf("Invalid movie option\n");
    return 0;
  }

  FILE *file = openMovieFile(connectionData->selectedMovie);
  if (file == NULL) {
    perror("Failed to open file");
    return 0;
  }

  int phrasesRead = 0;

  /**
   * Sends phrases from a file to a client using a UDP socket.
   *
   * @param connectionData The connection data containing the server socket and client address.
   * @param file The file containing the phrases to be sent.
   * @param phrasesRead The number of phrases already read from the file.
   */
  char buffer[MESSAGE_SIZE];
  while (phrasesRead < MOVIE_SIZE) {
    if (fgets(buffer, MESSAGE_SIZE, file) == NULL) {
      perror("Failed to read line from file");
      fclose(file);
      return 0;
    }
    phrasesRead++;

    ssize_t numBytesSent = sendto(
      connectionData->serverSocket, 
      buffer, 
      strlen(buffer), 
      NO_FLAGS, 
      (struct sockaddr*) &(connectionData->sockaddr), 
      sizeof(connectionData->sockaddr)
    );
    if (numBytesSent < 0) perror("sendto() failed");

    sleep(3);
  }

  fclose(file);

  // Send the end of movie message
  ssize_t numBytesSent = sendto(
    connectionData->serverSocket, 
    MOVIE_END_MESSAGE, 
    strlen(MOVIE_END_MESSAGE), 
    NO_FLAGS, 
    (struct sockaddr*) &(connectionData->sockaddr), 
    sizeof(connectionData->sockaddr)
  );
  if (numBytesSent < 0) perror("sendto() failed");
  
  pthread_mutex_lock(&mutex);
  connectedClients--;
  pthread_mutex_unlock(&mutex);
  
  return 0;
}

/**
 * Main function of the server program.
 * 
 * @param argc The number of command-line arguments.
 * @param argv An array of strings containing the command-line arguments.
 *             The first argument should be either "ipv4" or "ipv6" to specify the IP type.
 *             The second argument should be the server port number.
 * @return Returns 0 on successful execution.
 */
int main(int argc, char *argv[]) {
  if (argc != 3)
    exitWithSystemMessage("Usage: ./server <ip-type> <port>");

  int ipType = (strcmp(argv[1], "ipv4") == 0) ? IPV4_CODE : IPV6_CODE;
  char* servPort = argv[2];

  // Construct the server address structure
  struct addrinfo addrCriteria; // Criteria for address
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = (ipType == IPV4_CODE) ? AF_INET : AF_INET6;
  addrCriteria.ai_flags = AI_PASSIVE; // Accept on any address/port
  addrCriteria.ai_socktype = SOCK_DGRAM; // Only datagram socket
  addrCriteria.ai_protocol = IPPROTO_UDP; // Only UDP socket

  struct addrinfo *serverAddr; // Holder for returned server addrs
  int rtnVal = getaddrinfo(NULL, servPort, &addrCriteria, &serverAddr);  
  if (rtnVal != 0)
    exitWithSystemMessage("getaddrinfo() failed");

  // Create socket for incoming connections
  int serverSock = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol);
  if (serverSock < 0) {
    exitWithSystemMessage("socket() failed");
  }

  // Bind to the local address
  if (bind(serverSock, serverAddr->ai_addr, serverAddr->ai_addrlen) < 0) {
    exitWithSystemMessage("bind() failed");
  }

  // Free address list allocated by getaddrinfo()
  freeaddrinfo(serverAddr);

  // Handles exhibition of connected clients  
  pthread_mutex_init(&mutex, NULL);

  pthread_t printThread;
  int ret = pthread_create(&printThread, NULL, printClients, NULL);
  if(ret != 0) perror("pthread_create() failed");
  
  while (1) {
    struct handlerData *connectionData = malloc(sizeof(struct handlerData));
    if (connectionData == NULL) {
      perror("ERROR: malloc() failed");
      continue;
    }
    connectionData->serverSocket = serverSock;

    // Set length of client address structure (in-out parameter)
    socklen_t clntAddrLen = sizeof(connectionData->sockaddr);

    char buffer[MESSAGE_SIZE]; // I/O buffer
    ssize_t numBytesRcvd = recvfrom(
      serverSock, 
      buffer, 
      MESSAGE_SIZE, 
      NO_FLAGS, 
      (struct sockaddr *) &(connectionData->sockaddr), 
      &clntAddrLen
    );
    if (numBytesRcvd < 0) {
      free(connectionData);
      perror("recvfrom() failed");
    }
    
    connectionData->selectedMovie = atoi(buffer);
    pthread_mutex_lock(&mutex);
    connectedClients++;
    pthread_mutex_unlock(&mutex);

    pthread_t thread;
    int ret = pthread_create(&thread, NULL, handleUDPClient, (void *)connectionData);
    if (ret != 0) {
      perror("pthread_create() failed");
      free(connectionData);
      continue;
    }
  }

  pthread_mutex_destroy(&mutex);
  exit(0);
  return 0;
}