#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>

// this program should also be burnt and thrown into the pits of hell
// ... but failing that, you launch it just by
// nofx2EventReader [verbosity#]
// e.g. nofx2EventReader 0 (default) - quiet, don't say anything
//      nofx2EventReader 1 (low) - print completed frames
//      nofx2EventReader 2 (medium) - print fragmented read process
//      nofx2EventReader 3 (high) - print entire goddamn read process

// oh fucking hell
// we need to create a goddamn reader process
// because Xillybus at this point had broken async I/O
// you've got to be fucking kidding me

// max frame size is 65537 words or 32768.5 uint32s
#define BUFFER_SIZE 32769

int main(int argc, char **argv) {
  int retval;
  int wfd, rfd;
  unsigned int ndb;
  unsigned int nrdb;
  uint32_t *buffer;
  uint8_t *dp;
  uint8_t *op;
  int verbosity = 0;

  argc--;
  argv++;
  if (argc) verbosity = strtoull(*argv, NULL, 0);

  retval = mkfifo("/tmp/atri_inframes", 0666);
  if (verbosity > 1) printf("mkfifo returned retval %d\n", retval);
  // this will block until someone opens us
  wfd = open("/tmp/atri_inframes", O_WRONLY);
  // ok we should be open now
  // n.b. we probably need to catch a signal or something, who knows
  rfd = open("/dev/xillybus_ev_out", O_RDONLY);
  buffer = (uint32_t *) malloc(sizeof(uint32_t)*BUFFER_SIZE);
  if (wfd < 0 || rfd < 0 || !buffer) {
    fprintf(stderr, "cannot start up, one of wfd/rfd/malloc open() failed\n");
    fprintf(stderr, "open(/tmp/atri_inframes): %d", wfd);
    fprintf(stderr, "open(/dev/xillybus_ev_out): %d", rfd);
    fprintf(stderr, "malloc(buffer): %llx", (unsigned long long) buffer);
    unlink("/tmp/atri_inframes");
    exit(1);
  }
  while (1) {
    int nb;
    ndb = 4;
    dp = (uint8_t *) buffer;
    // step 1: read the header.
    // a header consists of a single 32-bit word (4 bytes)
    retval = read(rfd, dp, ndb);
    if (retval < 0) {
      fprintf(stderr, "header: got a read error %d\n", errno);
      goto myExit;
    } else if (retval != 4) {
      fprintf(stderr, "header: didn't get 4 bytes returned: %d\n", retval);
      goto myExit;
    }
    // value is in words
    ndb = ((buffer[0] >> 16) & 0xFFFF);
    // if it's odd, pad it
    if (ndb & 0x1) ndb = ndb + 1;
    // convert it to bytes
    ndb = ndb * 2;
    // store total to read
    nrdb = ndb;
    // calculate data pointer
    dp = (uint8_t *) (&buffer[1]);
    if (verbosity > 1) {
      printf("got a header: %2.2x %2.2x\n",
	     buffer[0] & 0xFFFF,
	     (buffer[0] >> 16) & 0xFFFF);
      printf("waiting for %d bytes (dp offset %d)\n", ndb,
	     (dp-(uint8_t *)buffer));
    }
    // while there are bytes to read, read
    while (ndb) {
      // try full read
      retval = read(rfd, dp, ndb);
      // on less than 0, panic
      // on a read of non-multiple of 4, panic
      // neither should happen
      if (retval < 0) {
	fprintf(stderr, "body: got a read error %d\n", errno);
	goto myExit;
      } else if (retval % 4) {
	fprintf(stderr, "body: didn't get multiple of 4 bytes: %d\n", retval);
	goto myExit;
      }
      // decrease number of bytes to read
      ndb -= retval;
      // store original data pointer
      op = dp;
      // move data pointer forward
      dp += (retval>>2);
      if (verbosity > 1) {
	printf("got %d bytes (dp offset now %d) remaining %d\n", retval, (dp-(uint8_t *)buffer), ndb);
	if (verbosity > 2) {
	  for (int i=0;i<((nrdb-ndb)>>2);i++) {
	    printf("%4.4x %4.4x", op[i] & 0xFFFF, (op[i] >> 16) & 0xFFFF);
	    if (!((i+1)%8)) printf("\n");
	    else if (i+1<(nrdb-ndb)) printf(" ");
	    else printf("\n");
	  }
	}
      }
    }
    // calculate number of bytes to write (data plus 4)
    nb = nrdb + 4;
    if (verbosity > 0) {
      printf("got a frame (%d bytes) - delivering\n", nb);
      for (int i=0;i<(nb>>2);i++) {
	printf("%4.4x %4.4x", buffer[i] & 0xFFFF, (buffer[i]>>16) & 0xFFFF);
	if (!((i+1)%8)) printf("\n");
	else if (i+1<nb) printf(" ");
	else printf("\n");
      }
    }
    retval = write(wfd, buffer, nb);
    if (retval < 0) {
      fprintf(stderr, "got a write error %d\n", retval);
      break;
    }
  }
 myExit:
  printf("Exiting\n");
  close(rfd);
  close(wfd);
  // destroy on exit
  unlink("/tmp/atri_inframes");
}
