#ifndef NOFX2CONTROLLIB_H
#define NOFX2CONTROLLIB_H

#include <pthread.h>
#include <stdint.h>

// such garbage
#include "fx2Defines.h"
#include "atriDefines.h"

// STUPID NAMING MUST BE MAINTAINED
// DUPLICATES OF ATRICONTROLLIB
// THIS LETS US OVERRIDE THIS LIBRARY WITH LD_PRELOAD

//Here is the socket part of the library
#define MAX_ATRI_CONNECTIONS 128
#define MAX_FX2_CONNECTIONS 128
#define MAX_ATRI_PACKETS 256   //Arbitrary numbers may change to something more meaningful

#define NUM_ASYNC_BUFFERS 480
#define NUM_ASYNC_REQUESTS 10

// Now here are some simple global variables for handling the atri control socket
pthread_mutex_t async_buffer_mutex;
pthread_mutex_t atri_socket_list_mutex;
pthread_mutex_t atri_packet_list_mutex;
pthread_mutex_t atri_packet_queue_mutex;
int fAtriControlSocket;
int fNumAtriSockets;

// Now here are some simple global variables for handling the fx2 control socket
pthread_mutex_t fx2_socket_list_mutex;
int fFx2ControlSocket;
int fNumFx2Sockets;

#define READ_USB_TIMEOUT 1
#define WRITE_USB_TIMEOUT 10
// Lastly the mutex for libusb
pthread_mutex_t libusb_command_mutex;


//Here is the socket list
struct socket_list {
  int socketFd;
  struct socket_list *next;
} ;
typedef struct socket_list AtriSocketLinkedList_t;
typedef struct socket_list Fx2SocketLinkedList_t;
AtriSocketLinkedList_t *fAtriControlSocketList;
Fx2SocketLinkedList_t *fFx2ControlSocketList;

//Here is the packet list
struct packet_list {
  uint8_t atriPacketNumber;
  int socketFd;
  struct packet_list *next;
} ;
typedef struct packet_list AtriPacketLinkedList_t;
AtriPacketLinkedList_t *fPacketList;

struct packet_queue {
  AtriControlPacket_t controlPacket;
  struct packet_queue *next;
} ;
typedef struct packet_queue AtriPacketQueueFifo_t;
AtriPacketQueueFifo_t *fAtriPacketFifo;
uint8_t fAtriPacketNumber;



//Need to add documentation for all of this
//Now for the worker functions
void initAtriControlSocket(char *socketPath);
int checkForNewAtriControlConnections();
int serviceOpenAtriControlConnections();
int addToAtriControlSocketList(int socketFd);
int removeFromAtriControlSocketList(int socketFd);
void addControlPacketToQueue(AtriControlPacket_t *packetPtr);
int getControlPacketFromQueue(AtriControlPacket_t *packetPtr);
int addToAtriPacketList(int socketFd, uint8_t packetNumber);
int removeFromAtriPacketList(uint8_t packetNumber);
void sendAtriControlPacketToSocket(AtriControlPacket_t *packetPtr);

void closeAtriControl(char *socketPath);


void initFx2ControlSocket(char *socketPath);
int checkForNewFx2ControlConnections();
int serviceOpenFx2ControlConnections();
int addToFx2ControlSocketList(int socketFd);
int removeFromFx2ControlSocketList(int socketFd);
void closeFx2Control(char *socketPath);

// IMMENSE BULLCRAP

#define INVALID_HANDLE_VALUE NULL
#define USB_TOUT_MS 50  // 50 ms
#define ATRI_EVENT_EP_WRITE    0x04 //USBFX2 end point address for bulk write
#define ATRI_EVENT_EP_READ     0x86 //USBFX2 end point address for bulk read
#define ATRI_CONTROL_EP_WRITE  0x02 
#define ATRI_CONTROL_EP_READ   0x84 
#define USBFX2_INTFNO 0 //USBFX2 interface number
#define USBFX2_CNFNO 1 //USBFX2 configuration number
#define USBFX2_ALT_SETTING 0 //USBFX2 alternative setting

/* USBFX2 device descriptions */
static const unsigned int USBFX2_ATRI_VENDOR_ID = 0x04b4; //0x090c;
//static const unsigned int USBFX2_ATRI_PRODUCT_ID = 0x1111; //0x1000;
static const unsigned int USBFX2_ATRI_PRODUCT_ID = 0x9999; //0x1000;


// PCIe endpoint device name and file handle
#define ATRI_PCIE_DEV "/dev/atri-pcie"
int usePcieReadout = 0;
int pcieHandle = -1;

// PCIe IOCTL commands
enum {
    XPCIE_IOCTL_INIT,
    XPCIE_IOCTL_FLUSH
};

//struct usb_device *findEzUsbAtriBoard();
void enablePcieEndPoint();
void disablePcieEndPoint();
int openPcieDevice();
int closePcieDevice();

int openFx2Device();
int flushControlEndPoint();
int flushEventEndPoint();
int closeFx2Device();
int sendVendorRequest(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength);
int sendVendorRequestStruct(Fx2ControlPacket_t *controlPacket);
int readControlEndPoint(unsigned char* buffer, int numBytes, int *numBytesRead);
int writeControlEndPoint(unsigned char* buffer, int numBytes, int *numBytesSent);
int readEventEndPoint(unsigned char* buffer, int numBytes, int *numBytesRead);
int writeEventEndPoint(unsigned char* buffer, int numBytes, int *numBytesSent);

char *getLibUsbErrorAsString(int error);

#endif
