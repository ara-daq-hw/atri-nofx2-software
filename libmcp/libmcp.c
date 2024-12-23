#include "libmcp.h"

#define VR_HOST_TO_DEVICE 0x40
#define VR_DEVICE_TO_HOST 0xC0
#define VR_ATRI_I2C 0xB4
#define VR_ATRI_COMPONENT_ENABLE 0xB6
#define VR_ATRI_RESET 0xBB
#define VR_VERSION 0xB7

/* these are the maps from GPIO to ENs */
/* they DO NOT MATCH THE SCHEMATIC IGNORE THAT */
/* GP0 -> EN3 */
/* GP1 -> EN4 */
/* GP2 -> EN1 */
/* GP3 -> EN2 */
/* this was done for routability */
// component enable is done via VR_ATRI_COMPONENT_ENABLE
// lower 8 bits are components to change
// top 8 bits are what to do with them (1 to turn on 0 to turn off)
// EX1_mask = 0x01
// EX2_mask = 0x02
// EX3_mask = 0x04
// EX4_mask = 0x08

mcp2221_t *globalDevice = NULL;

// internally these are called with like "initFx2ControlSocket"
// stuff and "closeFx2Control" stuff.
void mcpComLib_init() {
  globalDevice = mcp_ara_open();
}

void mcpComLib_finish() {
  if (globalDevice != NULL)
    mcp_ara_close(globalDevice);
}

// this only gets used with mcpComLib stuff
#define NOFX2_VERSION_MAJOR 2
#define NOFX2_VERSION_MINOR 0

int sendVendorRequestMcp(uint8_t bmRequestType,
		      uint8_t bRequest,
		      uint16_t wValue,
		      uint16_t wIndex,
		      unsigned char *data,
		      uint16_t wLength) {
  if (bRequest == VR_ATRI_RESET) {
    // whatever
    return 0;
  }
  if (bRequest == VR_VERSION) {
    data[0] = NOFX2_VERSION_MAJOR;
    data[1] = NOFX2_VERSION_MINOR;
    return 0;
  }
  if (bRequest == VR_ATRI_COMPONENT_ENABLE) {
    uint8_t gpioToChange = (wValue & 0x0F);
    uint8_t highOrLow = ((wValue >> 8) & 0x0F);
    if (gpioToChange & 0x1) {
      // EX1 -> GP2
      if (highOrLow & 0x1)
	mcp2221_setGPIO(globalDevice, MCP2221_GPIO2, MCP2221_GPIO_VALUE_HIGH);
      else
	mcp2221_setGPIO(globalDevice, MCP2221_GPIO2, MCP2221_GPIO_VALUE_LOW);
    } else if (gpioToChange & 0x2) {
      // EX2 -> GP3
      if (highOrLow & 0x2)
	mcp2221_setGPIO(globalDevice, MCP2221_GPIO3, MCP2221_GPIO_VALUE_HIGH);
      else
	mcp2221_setGPIO(globalDevice, MCP2221_GPIO3, MCP2221_GPIO_VALUE_LOW);
    } else if (gpioToChange & 0x4) {
      // EX3 -> GP0
      if (highOrLow & 0x4)
	mcp2221_setGPIO(globalDevice, MCP2221_GPIO0, MCP2221_GPIO_VALUE_HIGH);
      else
	mcp2221_setGPIO(globalDevice, MCP2221_GPIO0, MCP2221_GPIO_VALUE_LOW);
    } else if (gpioToChange & 0x8) {
      // EX4 -> GP1
      if (highOrLow & 0x8)
	mcp2221_setGPIO(globalDevice, MCP2221_GPIO1, MCP2221_GPIO_VALUE_HIGH);
      else
	mcp2221_setGPIO(globalDevice, MCP2221_GPIO1, MCP2221_GPIO_VALUE_LOW);
    }
    return 0;
  }
  if (bRequest != VR_ATRI_I2C) return -1;
  // NOTE: the addr that we get is the dumbcrap 8-bit address.
  // so just freaking drop it.
  if (bmRequestType == VR_HOST_TO_DEVICE) {
    uint8_t addr = wValue >> 1;
    mcp_ara_i2c_write(globalDevice,
		      addr,
		      data,
		      wLength);
  } else if (bmRequestType == VR_DEVICE_TO_HOST) {
    uint8_t addr = wValue >> 1;
    mcp_ara_i2c_read(globalDevice,
		     addr,
		     data,
		     wLength);
  }
  return 0;
}

mcp2221_t *mcp_ara_open() {
  mcp2221_init();
  int count = mcp2221_find(MCP2221_DEFAULT_VID,
			   MCP2221_DEFAULT_PID,
			   NULL, NULL, NULL);
  if (!count) {
    fprintf(stderr, "fatal error: could not find MCP2221!");
    mcp2221_exit();
    return NULL;
  }
  mcp2221_t *myDev = mcp2221_open();
  mcp2221_gpioconfset_t gpioConf = mcp2221_GPIOConfInit();

  gpioConf.conf[0].gpios          = MCP2221_GPIO0 | MCP2221_GPIO1 | MCP2221_GPIO2 | MCP2221_GPIO3;
  gpioConf.conf[0].mode           = MCP2221_GPIO_MODE_GPIO;
  gpioConf.conf[0].direction      = MCP2221_GPIO_DIR_OUTPUT;
  gpioConf.conf[0].value          = MCP2221_GPIO_VALUE_LOW;

  mcp2221_setGPIOConf(myDev, &gpioConf);

  mcp2221_i2c_state_t state = MCP2221_I2C_IDLE;
  mcp2221_i2cState(myDev, &state);
  if (state != MCP2221_I2C_IDLE)
    mcp2221_i2cCancel(myDev);
  mcp2221_i2cDivider(myDev, 260);

  return myDev;
}

void mcp_ara_close(mcp2221_t *myDev) {
  mcp2221_close(myDev);
  mcp2221_exit();
}

// we need exactly 2 functions
// read bytes from I2C
// write bytes to I2C
// return 0 if OK -1 if nak
int mcp_ara_i2c_write(mcp2221_t *myDev,
		       uint8_t addr,
		       uint8_t *data,
		       int length) {
  mcp2221_i2c_state_t state;
  mcp2221_i2c_status_t status;
  
  mcp2221_i2cWrite(myDev, addr, data, length, MCP2221_I2CRW_NORMAL);
  while (1) {
    mcp2221_i2cFullStatus(myDev, &state, &status);
    if (state == MCP2221_I2C_IDLE || status != MCP2221_I2C_OK)
      break;
  }
  if (status != MCP2221_I2C_OK)
    return -1;
  return 0;
}

int mcp_ara_i2c_read(mcp2221_t *myDev,
		     uint8_t addr,
		     uint8_t *data,
		     int length) {
  mcp2221_i2c_state_t state;
  mcp2221_i2c_status_t status;

  mcp2221_i2cRead(myDev, addr, length, MCP2221_I2CRW_NORMAL);
  while (1) {
    mcp2221_i2cState(myDev, &state);
    if (state == MCP2221_I2C_DATAREADY || state == MCP2221_I2C_ADDRNOTFOUND)
      break;
  }
  if (state == MCP2221_I2C_DATAREADY) {
    mcp2221_i2cGet(myDev, data, length);
    return 0;
  }
  return -1;
}

// we are trying to replace atriControlLib:
// we sleazeball this by just replacing
// sendVendorRequest.
// bmRequestType = VR_HOST_TO_DEVICE (0x40) or VR_DEVICE_TO_HOST (0xC0)
// bRequest = VR_ATRI_I2C (0xb4)
// wValue = i2cAddress
// wIndex = 0
// dataLength = length
// read is exactly the same except bmRequestType is VR_DEVICE_TO_HOST

