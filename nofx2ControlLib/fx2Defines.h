/*! \file fx2Defines.h
    \brief Header file which specifies the FX2 packets definitions
    
    The header file which specifies the FX2 packet definitions as described in 
    ARA-doc-267-v1
    
    June 2011 rjn@hep.ucl.ac.uk

*/
/*
 *
 * Current CVS Tag:
 * $Header: $
 */

#ifndef FX2_DEFINES_H
#define FX2_DEFINES_H
#include <stdint.h>


//This is just an arbitrary number for now
#define MAX_FX2_BUFFER_SIZE 64


//!  The FX2 Vendor Request Options
/*!
  Here is a list of the enabled vendor requests
*/
enum {
  VR_ATRI_I2C = 0xb4,
  VR_LED_DEBUG = 0xb5,
  VR_VERSION = 0xb7,
  VR_ATRI_COMPONENT_ENABLE=0xb6,
  VR_ATRI_RESET = 0xBB
};
typedef uint8_t Fx2VendorRequest_t;




//!  The FX2 Vendor Request Directions
/*!
  Here is a list of the enabled vendor requests
*/
enum {
  VR_HOST_TO_DEVICE = 0x40,
  VR_OUT = 0x40,
  VR_DEVICE_TO_HOST = 0xc0,
  VR_IN = 0xc0
};
typedef uint8_t Fx2VendorRequestDirection_t;



//!  The FX2 Atri Component Mask
/*!
  The masks needed to enable or disable various components on the ATRI
*/
enum {
  FX2_ATRI_DDA1_MASK = 0x01,
  FX2_ATRI_DDA2_MASK = 0x02,
  FX2_ATRI_DDA3_MASK = 0x04,
  FX2_ATRI_DDA4_MASK = 0x08,
  FX2_ATRI_GPS_MASK = 0x20,
  FX2_ATRI_FPGA_OSC_MASK = 0x40
};
typedef uint8_t Fx2AtriComponentMask_t;



//!  The FX2 I2C Address
/*!
  The I2C Address
*/
enum {
  I2C_ATRI_HOT_SWAP_READ=0x91,
  I2C_ATRI_HOT_SWAP_WRITE=0x90,
  I2C_ATRI_CLOCK_THINGY_READ=0xd1,
  I2C_ATRI_CLOCK_THINGY_WRITE=0xd0
};
typedef uint8_t Fx2I2CAddress_t;



//!  The FX2 Control Packet 
/*!
  Here is the header for the FX2 control packets
*/
typedef struct {
  Fx2VendorRequestDirection_t bmRequestType;
  Fx2VendorRequest_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t dataLength;
  uint8_t data[MAX_FX2_BUFFER_SIZE]; 
} Fx2ControlPacket_t; ///<The Header associated with a packet



//!  The FX2 Response Packet 
/*!
  Here is the format for responses to FX2 Control Packets
*/
typedef struct {
  int16_t status;
  Fx2ControlPacket_t control;
} Fx2ResponsePacket_t; ///< The repsonse format






#endif //FX2_DEFINES_H
