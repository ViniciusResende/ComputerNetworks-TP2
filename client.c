#include <netinet/in.h>
#include "commons.h"

/**
 * Prints the client menu options.
 */
void printMenu() {
  printf("\n\n");
  printf("0 - Sair\n");
  printf("1 - Senhor dos anéis\n");
  printf("2 - O Poderoso Chefão\n");
  printf("3 - Clube da Luta\n");
  printf("\n");
}

/**
 * @brief Handles the UDP server communication.
 * 
 * This function establishes a connection with the server using UDP protocol and sends the selected movie to the server.
 * It then receives and prints the messages sent by the server until the end of the movie transmission.
 * 
 * @param ipFamily The IP family to use (IPv4 or IPv6).
 * @param servPort The server port number.
 * @param address The server address.
 */
void handleUDPServer(int ipFamily, char *servPort, char *address) {
  // Tell the system what kind(s) of address info we want
  struct addrinfo addrCriteria; // Criteria for address
  memset(&addrCriteria, 0, sizeof(addrCriteria)); // Zero out structure
  addrCriteria.ai_family = (ipFamily == IPV4_CODE) ? AF_INET : AF_INET6;
  addrCriteria.ai_socktype = SOCK_DGRAM; // Only datagram socket
  addrCriteria.ai_protocol = IPPROTO_UDP; // Only UDP socket

  // Get address(es)
  struct addrinfo *servAddr; // Holder for returned list of server addrs
  int rtnVal = getaddrinfo(address, servPort, &addrCriteria, &servAddr);
  if (rtnVal != 0) {
    perror("getaddrinfo() failed");
  }

  int selectedMovie = -1;
  while (selectedMovie != 0) {
    // Prints client menu
    printMenu();
    // Reads the selected movie
    scanf("%d", &selectedMovie);

    while(selectedMovie < 0 || selectedMovie > 3) {
      printf("Opção inválida, selecione uma das listadas abaixo:\n");
      printMenu();
      scanf("%d", &selectedMovie);
    }

    if (selectedMovie == 0) break; // Finishes up the handling of the server

    // Create a datagram/UDP socket
    int clientSocket = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol); // Socket descriptor for client
    if (clientSocket < 0) exitWithSystemMessage("ERROR opening socket");

    // Send the selected movie to the server
    char selectedMovieStr[MESSAGE_SIZE];
    sprintf(selectedMovieStr, "%d", selectedMovie);
    ssize_t numBytesSent = sendto(
      clientSocket, 
      selectedMovieStr, 
      strlen(selectedMovieStr), 
      NO_FLAGS, 
      servAddr->ai_addr, 
      servAddr->ai_addrlen
    );
    if (numBytesSent < 0) {
      exitWithSystemMessage("ERROR sending message to server");
    } else if ((size_t)numBytesSent != strlen(selectedMovieStr)) {
      exitWithSystemMessage("ERROR sending unexpected number of bytes");
    }

    struct sockaddr_storage fromAddr; // Source address of server
    ssize_t numBytesRcvd = 0; // Bytes received in single recv()
    socklen_t fromAddrLen = sizeof(fromAddr);
    char buffer[MESSAGE_SIZE]; // I/O buffer
    while (1) {
      // Receive message from server
      memset(buffer, 0, MESSAGE_SIZE);
      numBytesRcvd = recvfrom(
        clientSocket, 
        buffer, 
        MESSAGE_SIZE, 
        NO_FLAGS, 
        (struct sockaddr *) &fromAddr, 
        &fromAddrLen
      );
      if (numBytesRcvd < 0) {
        exitWithSystemMessage("ERROR receiving message from server");
      }

      // Checks for end of movie transmission
      if (strcmp(buffer, MOVIE_END_MESSAGE) == 0) {
        close(clientSocket);
        break;
      } else {
        // Prints received message if not end of the movie
        printf("%s", buffer);
      }
    }    
  }
}

/**
 * @brief The main function of the client program.
 *
 * This function is the entry point of the client program. It takes command line arguments
 * for IP type, IP address, and port number. It then calls the handleUDPServer function to
 * establish a UDP connection with the server.
 *
 * @param argc The number of command line arguments.
 * @param argv An array of strings containing the command line arguments.
 * @return 0 on successful execution.
 */
int main(int argc, char *argv[]) {
  if (argc != 4)
    exitWithSystemMessage("Usage: ./client <ip-type> <server-ip> <server-port>");

  int ipType = (strcmp(argv[1], "ipv4") == 0) ? IPV4_CODE : IPV6_CODE;
  char *address = argv[2];
  char *servPort = argv[3];

  handleUDPServer(ipType, servPort, address);

  exit(0);
  return 0;
}