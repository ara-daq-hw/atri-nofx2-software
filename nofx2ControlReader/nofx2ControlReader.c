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

#define LOG_PFX "nofx2ControlReader: "

// this program should be burnt and thrown into the pits of hell
// ... but failing that, you launch it just by
// nofx2ControlReader [verbosity#]
// e.g. nofx2ControlReader 0 (default) - quiet, don't say anything
//      nofx2ControlReader 1 (low) - print completed packets
//      nofx2ControlReader 2 (medium) - print fragmented read process
//      nofx2ControlReader 3 (high) - print entire goddamn read process

// oh fucking hell
// we need to create a goddamn reader process
// because Xillybus at this point had broken async I/O
// you've got to be fucking kidding me
int main(int argc, char **argv) {
  int retval;
  int wfd, rfd;
  unsigned int ndb;
  unsigned int nrdb;
  uint8_t buffer[256];
  uint8_t *dp;
  uint8_t *op;

  int verbosity = 0;

  argc--;
  argv++;
  if (argc) verbosity = strtoull(*argv, NULL, 0);

  retval = mkfifo("/tmp/atri_inpkts", 0666);
  if (verbosity > 1) printf(LOG_PFX "mkfifo returned retval %d\n", retval);
  // this will block until someone opens us
  wfd = open("/tmp/atri_inpkts", O_WRONLY);
  // ok we should be open now
  rfd = open("/dev/xillybus_pkt_out", O_RDONLY);
  if (wfd < 0 || rfd < 0) {
    fprintf(stderr, LOG_PFX "cannot start up, one of wfd/rfd open() failed\n");
    fprintf(stderr, LOG_PFX "open(/tmp/atri_inpkts): %d", wfd);
    fprintf(stderr, LOG_PFX "open(/dev/xillybus_pkt_out): %d", rfd);
    unlink("/tmp/atri_inpkts");
    exit(1);
  }
  while (1) {
    int nb;
    ndb = 4;
    dp = buffer;
    while (ndb) {
      retval = read(rfd, dp, ndb);
      if (retval < 0) {
	fprintf(stderr, LOG_PFX "header: got a read error %d\n", errno);
	goto myExit;
      }
      ndb -= retval;
      dp += retval;
      if (verbosity > 1) {
	printf(LOG_PFX "got %d bytes reading header, %d left\n", retval, ndb);
	if (verbosity > 2) {
	  printf(LOG_PFX "header currently: %2.2x %2.2x %2.2x %2.2x\n",
		 buffer[0],
		 buffer[1],
		 buffer[2],
		 buffer[3]);
	  printf(LOG_PFX "dp offset is %d\n", (dp-buffer));
	}
      }
    }
    ndb = buffer[3]+1;
    nrdb = ndb;
    dp = buffer + 4;
    if (verbosity > 1) {
      printf(LOG_PFX "got a header: %2.2x %2.2x %2.2x %2.2x\n",
	     buffer[0], buffer[1], buffer[2], buffer[3]);
      printf(LOG_PFX "waiting for %d bytes (dp offset %d)\n", ndb,
	     (dp-buffer));
    }
    while (ndb) {
      retval = read(rfd, dp, ndb);
      if (retval < 0) {
	fprintf(stderr, LOG_PFX "got a read error %d\n", errno);
	goto myExit;
      }
      ndb -= retval;
      op = dp;
      dp += retval;
      if (verbosity > 1) {
	printf(LOG_PFX "got %d bytes (dp offset now %d) remaining %d\n", retval, (dp-buffer), ndb);
	if (verbosity > 2) {
	  printf(LOG_PFX);
	  for (int i=0;i<(nrdb-ndb);i++) {
	    printf("%2.2x", op[i]);
	    if (!((i+1)%16)) {
	      if (i+1<(nrdb-ndb)) printf(LOG_PFX);
	      printf("\n");
	    }
	    else if (i+1<(nrdb-ndb)) printf(" ");
	    else printf("\n");
	  }
	}
      }
    }
    nb = buffer[3] + 5;
    if (verbosity > 0) {      
      printf(LOG_PFX "got a packet (%d bytes) - delivering\n", nb);
      if (verbosity > 1) {
	printf(LOG_PFX);
	for (int i=0;i<nb;i++) {
	  printf("%2.2x", buffer[i]);
	  if (!((i+1)%16)) {
	    if (i+1<nb) printf(LOG_PFX);
	    printf("\n");
	  }
	  else if (i+1<nb) printf(" ");
	  else printf("\n");
	}
      }
    }
    retval = write(wfd, buffer, nb);
    if (retval < 0) {
      fprintf(stderr, LOG_PFX "got a write error %d\n", retval);
      break;
    }
  }
 myExit:
  printf(LOG_PFX "Exiting\n");
  close(rfd);
  close(wfd);
  // destroy on exit
  unlink("/tmp/atri_inpkts");
}
