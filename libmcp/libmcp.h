#ifndef MCP_H_
#define MCP_H_

#include <stdio.h>
#include <stdlib.h>
#include "libmcp2221/libmcp2221.h"
#include "libmcp2221/hidapi.h"

mcp2221_t *mcp_ara_open();
void mcp_ara_close(mcp2221_t *myDev);
int mcp_ara_i2c_write(mcp2221_t *myDev,
		      uint8_t addr,
		      uint8_t *data,
		      int length);
int mcp_ara_i2c_read(mcp2221_t *myDev,
		     uint8_t addr,
		     uint8_t *data,
		     int length);

// ultrasilliness
void mcpComLib_init();
void mcpComLib_finish();
int sendVendorRequestMcp(uint8_t bmRequestType,
		      uint8_t bRequest,
		      uint16_t wValue,
		      uint16_t wIndex,
		      unsigned char *data,
		      uint16_t wLength);

#endif
