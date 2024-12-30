/*! \file atriDefines.h
    \brief Header file which specifies the ATRI packets definitions
    
    The header file which specifies the ATRI packet definitions as described in 
    ARA-doc-267-v1
    
    June 2011 rjn@hep.ucl.ac.uk

*/
/*
 *
 * Current CVS Tag:
 * $Header: $
 */

#ifndef ATRI_DEFINES_H
#define ATRI_DEFINES_H
#include <stdint.h>

#define ATRI_CONTROL_FRAME_START 0x3c
#define ATRI_CONTROL_FRAME_END 0x3e

#define MAX_ATRI_PACKET_SIZE 64 //Is this sufficent?

///Do all the int8_t need to be uint8_t?


//!  The ATRI packet location (destination or source)
/*!
  When messages are sent to/from the firmware they are to/from one of these locations
*/
enum {
  ATRI_LOC_PACKET_CONTROLLER=0, ///< The ATRI packet controller
  ATRI_LOC_WISHBONE=1, ///< The WISHBONE bus
  ATRI_LOC_DB_STATUS=2, ///< The daughter board status modules
  ATRI_LOC_I2C_DB1=3, ///< The I2C controller for daughterboard 1
  ATRI_LOC_I2C_DB2=4, ///< The I2C controller for daughterboard 2
  ATRI_LOC_I2C_DB3=5, ///< The I2C controller for daughterboard 3
  ATRI_LOC_I2C_DB4=6 ///< The I2C controller for daughterboard 4
} ;
typedef uint8_t AtriControlPacketLocation_t;  ///<Should only ever be 8 bytes long



//!  The ATRI Control Packet Header
/*!
  Here is the four byte header that starts every control packet. After an optional payload of packetLength bytes the control packet ends with a single byte '0x3e'
*/
typedef struct {
  uint8_t frameStart; ///< 0x3c ('<')
  AtriControlPacketLocation_t packetLocation; ///< Destination or source
  uint8_t packetNumber; ///< Unique packet number
  uint8_t packetLength; ///< Number of optional bytes in the payload
} AtriControlPacketHeader_t; ///<The Header associated with a packet


//!  The ATRI Control Packet
/*!
  Here is the atri control packet object
*/
typedef struct {
  AtriControlPacketHeader_t header;
  uint8_t data[MAX_ATRI_PACKET_SIZE]; ///the last valid data entry must be 0x3e
} AtriControlPacket_t; ///<The container classs for Atri Control Packets... not a fixed size




///Here are some packet length definitions that may or may not be useful
#define ATRI_APC_INFO_PACKET_LENGTH 1
#define ATRI_DBSTAT_INFO_PACKET_LENGTH 4 ///<For the four daughterboard statuses

//Here are the DBSTAT bit definitions
#define ATRIDBSTAT_DDA_PRESENT 0x1
#define ATRIDBSTAT_DDA_ON 0x2
#define ATRIDBSTAT_TDA_PRESENT 0x4
#define ATRIDBSTAT_TDA_ON 0x8




//!  The ATRI control packets from atri_packet_controller
/*!
  Here is the current single byte error enumerations from the atri_packet_controller
*/
enum {
  APC_HARDWARE_RESET=0, ///< A hardware reset has occured
  APC_BAD_PACKET_FORMAT=-1, ///< Malformed packet
  APC_BAD_DESTINATION=-2, ///< Bad destination for packet
  APC_I2C_CONTROLLER_FIFO_FULL=-3 ///< An unidentified packet has been rejected since the destination I2C controller FIFO is full
} ;
typedef int8_t AtriPacketControllerErrorCode_t;





#endif //ATRI_DEFINES_H
