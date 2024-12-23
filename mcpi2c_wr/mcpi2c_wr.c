
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmcp.h"

int main(int argc, char **argv) {

  argc--;
  argv++;
  if (!argc) return 0;
  unsigned char addr = strtoull(argv[0], NULL, 0);
  argc--;
  argv++;
  if (!argc) return 0;
  char *d = malloc(sizeof(unsigned char)*argc);
  for (int i=0;i<argc;i++) {
    d[i] = strtoull(argv[i], NULL, 0);
  }

  mcp2221_t *dev = mcp_ara_open();
  mcp_ara_i2c_write(dev,
		    addr,
		    d,
		    argc);
  mcp_ara_close(dev);
}
