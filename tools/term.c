/*
 * term.c - A simple terminal program for talking to USB serial devices
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define BUFSIZE 1000

int main (int argc, char *argv[]) {

  fd_set fds;
  char buf[BUFSIZE];
  int n;

  if (argc != 2) {
    fprintf(stderr, "Usage: term [path]\n");
    exit(-1);
  }

  int fd = open(argv[1], O_RDWR);

  if (fd == -1) {
    perror("term");
    exit(-1);
  }
  
  fprintf (stderr, "Connected.\n");

  while (1) {
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    FD_SET(fd, &fds);

    select(fd+1, &fds, 0, 0, 0);

    if (FD_ISSET(fd, &fds)) {
      if ((n = read(fd, buf, BUFSIZE)) > 0) {
        write(1, buf, n);
      } else {
	fprintf(stderr, "EOF\n");
	exit(0);
      }
    }

    if (FD_ISSET(0, &fds)) {
      if ((n = read(0, buf, BUFSIZE)) > 0) {
        write(fd, buf, n);
      } else {
	close(fd);
	exit(0);
      }
    }
  }
}
