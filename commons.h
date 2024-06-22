// C Standard Libraries used
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// Socket related libraries
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/**
 * @file commons.h
 * @brief This file contains common definitions and structures used in the project.
 */

// The code representing an IPv4 address.
#define IPV4_CODE 1001

// The code representing an IPv6 address.
#define IPV6_CODE 1002

#define NO_FLAGS 0

// The maximum size of a message.
#define MESSAGE_SIZE 128

// Delimiter used to indicate the end of a Movie transmission.
#define MOVIE_END_MESSAGE "MOVIE_END"

#define LORD_OF_THE_RINGS 1
#define THE_GODFATHER 2
#define FIGHT_CLUB 3

/**
 * @brief Prints an error message to stderr and exits the program.
 *
 * This function prints the given error message and detail to the standard error stream (stderr),
 * followed by a newline character, and then terminates the program with an exit status of 1.
 *
 * @param msg The error message to be printed.
 * @param detail The additional detail or explanation of the error.
 */
void exitWithUserMessage(const char *msg, const char *detail) {
  fputs(msg, stderr);
  fputs(": ", stderr);
  fputs(detail, stderr);
  fputc('\n', stderr);
  exit(1);
}

/**
 * Prints an error message corresponding to the current value of errno,
 * followed by the provided message, and exits the program with a status of 1.
 *
 * @param msg The error message to be printed.
 */
void exitWithSystemMessage(const char *msg) {
  perror(msg);
  exit(1);
}
