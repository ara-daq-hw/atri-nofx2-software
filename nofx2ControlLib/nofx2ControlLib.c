// HAVE TO MAINTAIN DUMBASS NAMING
#include "nofx2ControlLib.h"
#include "libmcp.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <poll.h>
#include <fcntl.h>
#include "araSoft.h"
#include <unistd.h>
#include <string.h>

static const int SUCCEED = 1;
static const int FAILED = 0;

static int controlReadFd = -1;
static int controlWriteFd = -1;

///////////////////////////////////////////////////////////////////////////
//                       RYAN'S CRAP                                     //
///////////////////////////////////////////////////////////////////////////

//Here be some magic

int fNumAsyncRequests;
int fNumAsyncBuffers;
unsigned char asyncBuffer[NUM_ASYNC_BUFFERS][512];
unsigned char asyncBufferSoftware[NUM_ASYNC_BUFFERS][512];
int asyncBufferSize[NUM_ASYNC_BUFFERS];
int asyncBufferStatus[NUM_ASYNC_BUFFERS];

//Now for the worker functions
void initAtriControlSocket(char *socketPath)
{
  struct sockaddr_un u_addr; // unix domain addr
  int len;
  //Create socket
  if ((fAtriControlSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s -- socket -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }
  u_addr.sun_family = AF_UNIX;
  strcpy(u_addr.sun_path, socketPath);
  unlink(u_addr.sun_path);
  len = strlen(u_addr.sun_path) + sizeof(u_addr.sun_family);
  //Bind address
  if (bind(fAtriControlSocket, (struct sockaddr *)&u_addr, len) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s -- bind -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }     

  //Listen and have a queue of up to 20 connections waiting
  if (listen(fAtriControlSocket, 20)) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s -- listen -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }

  //Now set up the mutexs
  pthread_mutex_init(&atri_socket_list_mutex, NULL);    
  pthread_mutex_init(&atri_packet_list_mutex,NULL);
  pthread_mutex_init(&atri_packet_queue_mutex,NULL);

  pthread_mutex_lock(&atri_socket_list_mutex);
  fNumAtriSockets=0;
  fAtriControlSocketList = NULL;
  pthread_mutex_unlock(&atri_socket_list_mutex);

  pthread_mutex_lock(&atri_packet_list_mutex);
  fPacketList = NULL;
  pthread_mutex_unlock(&atri_packet_list_mutex);

  pthread_mutex_lock(&atri_packet_queue_mutex);
  fAtriPacketNumber=0;
  fAtriPacketFifo=NULL;
  pthread_mutex_unlock(&atri_packet_queue_mutex);

}

int checkForNewAtriControlConnections()
{
  // This is meant to be a simple function that just checks to see if we have an incoming
  // connection.
  struct pollfd fds[1];
  int s_dat = 0; 
  struct sockaddr_un u_inaddr; // incoming
  unsigned int u_len = sizeof(u_inaddr);
  int nfds=1;
  int pollVal;

  fds[0].fd = fAtriControlSocket;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  
  pollVal=poll(fds,nfds,1); //non-blocking, for now at least may change at some point if I put this in a thread
  if(pollVal==-1) {
    //At some point will improve the error handling
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Error calling poll: %s",__FUNCTION__,strerror(errno));
    close(fAtriControlSocket);
    return -1;
  }
  if(pollVal) {    
    if (fds[0].revents & POLLIN) {
      // incoming connection
      if ((s_dat = accept(fAtriControlSocket, (struct sockaddr *) &u_inaddr, &u_len)) == -1) {
        ARA_LOG_MESSAGE(LOG_ERR,"%s: accept -- %s\n",__FUNCTION__,strerror(errno));
        close(fAtriControlSocket);
        return -1;
      }
      ARA_LOG_MESSAGE(LOG_DEBUG,"%s: incoming connection\n",__FUNCTION__);
      addToAtriControlSocketList(s_dat);
    }
  }  
  return fNumAtriSockets;
}

int addToAtriPacketList(int socketFd, uint8_t packetNumber)
{
  //  fprintf(stderr,"addToAtriPacketList packetNumber=%u socketFd=%d fPacketList=%d\n",packetNumber,socketFd,(int)fPacketList);
  AtriPacketLinkedList_t *tempList=NULL;
  pthread_mutex_lock(&atri_packet_list_mutex);
  tempList=(AtriPacketLinkedList_t*)malloc(sizeof(AtriPacketLinkedList_t));
  tempList->socketFd=socketFd;
  tempList->atriPacketNumber=packetNumber;
  tempList->next=fPacketList;
  fPacketList=tempList;
  pthread_mutex_unlock(&atri_packet_list_mutex);
  //  ARA_LOG_MESSAGE(LOG_DEBUG,"addToAtriPacketList end packetNumber=%u fPacketList=%d\n",packetNumber,(int)fPacketList);
  return 0;

}

int removeFromAtriPacketList(uint8_t packetNumber)
{
  AtriPacketLinkedList_t *tempPacketList=NULL;
  //  fprintf(stderr,"removeFromAtriPacketList: packetNumber=%u tempPacketList=%d\n",packetNumber,(int)tempPacketList);
  int count=0;
  int socketFd=0;
  pthread_mutex_lock(&atri_packet_list_mutex);
  tempPacketList=fPacketList;;
  //Search through list and remove the item with socketFd
  while(tempPacketList) {
    //    ARA_LOG_MESSAGE(LOG_DEBUG,"Packet List: %d -- %d -- %d\n",count,tempPacketList->atriPacketNumber,packetNumber);
    if(tempPacketList->atriPacketNumber==packetNumber) {
      //Found our item in the list
      socketFd=tempPacketList->socketFd;
      //Only need to reassign fPacketList if we are deleting the first entry
      if(tempPacketList==fPacketList)
        fPacketList=tempPacketList->next;
      free(tempPacketList);
      pthread_mutex_unlock(&atri_packet_list_mutex);
      return socketFd;
      break;
    }
    tempPacketList=tempPacketList->next;
    count++;
  }
  pthread_mutex_unlock(&atri_packet_list_mutex);
  return 0;
}

void sendAtriControlPacketToSocket(AtriControlPacket_t *packetPtr)
{
  int socketFd=removeFromAtriPacketList(packetPtr->header.packetNumber);
  if(socketFd==0) {
    ARA_LOG_MESSAGE(LOG_ERR,"Error finding socket to return packet %d\n",packetPtr->header.packetNumber);
    return;
  }
  ARA_LOG_MESSAGE(LOG_DEBUG,"%s sending packet=%d to socketFd=%d\n",__FUNCTION__,packetPtr->header.packetNumber,socketFd);
  if (send(socketFd, (char*)packetPtr, sizeof(AtriControlPacket_t), 0) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: send -- %s\n",__FUNCTION__,strerror(errno));
    exit(1);
  }
 
}


int addToAtriControlSocketList(int socketFd)
{
  AtriSocketLinkedList_t *tempSocketList=NULL;
  pthread_mutex_lock(&atri_socket_list_mutex);
  // Create new item in the list at the head of the list
  tempSocketList=(AtriSocketLinkedList_t*)malloc(sizeof(AtriSocketLinkedList_t));
  tempSocketList->socketFd=socketFd;
  tempSocketList->next=fAtriControlSocketList;
  fAtriControlSocketList=tempSocketList;
  fNumAtriSockets++;
  pthread_mutex_unlock(&atri_socket_list_mutex);
  return fNumAtriSockets;
}

int removeFromAtriControlSocketList(int socketFd)
{
  pthread_mutex_lock(&atri_socket_list_mutex);
  AtriSocketLinkedList_t *lastSocketLink=NULL;
  AtriSocketLinkedList_t *tempSocketList=fAtriControlSocketList;
  int count=0;
  //Search through list and remove the item with socketFd
  while(tempSocketList) {
    //    ARA_LOG_MESSAGE(LOG_DEBUG,"%d -- %d -- %d\n",count,tempSocketList->socketFd,socketFd);
    if(tempSocketList->socketFd==socketFd) {
      //Found our item in the list
      if(lastSocketLink) {
        lastSocketLink->next=tempSocketList->next;
      }
      else {
        fAtriControlSocketList=tempSocketList->next;
      }
      free(tempSocketList);
      fNumAtriSockets--;
      break;
    }
    lastSocketLink=tempSocketList;
    tempSocketList=tempSocketList->next;
    count++;
  }
  pthread_mutex_unlock(&atri_socket_list_mutex);
  return fNumAtriSockets;
}


void closeAtriControl(char *socketPath) 
{
  
  AtriSocketLinkedList_t *tempSocketList=fAtriControlSocketList;
  while(tempSocketList) {
    close(tempSocketList->socketFd);
    tempSocketList=tempSocketList->next;
  }
  close(fAtriControlSocket);
  unlink(socketPath);
}

int serviceOpenAtriControlConnections()
{
  //Okay so this essentially checks the open connections and services them in some way
  //Could be combined with the check for new connections
  struct pollfd fds[MAX_ATRI_CONNECTIONS];

  AtriControlPacket_t controlPacket;
  int nbytes;
  int nfds=0;
  int tempInd=0;
  int pollVal;

  memset(&controlPacket,0,sizeof(AtriControlPacket_t));

  if(fNumAtriSockets==0) return 0;  
  pthread_mutex_lock(&atri_socket_list_mutex);
  AtriSocketLinkedList_t *tempSocketList=fAtriControlSocketList;
  //Loop through the socket list to fill the pollfd array
  while(tempSocketList) {
    fds[nfds].fd = tempSocketList->socketFd;
    fds[nfds].events = POLLIN;
    fds[nfds].revents = 0;
    nfds++;
    tempSocketList=tempSocketList->next;
  }
  pthread_mutex_unlock(&atri_socket_list_mutex);
  if(nfds==0) return 0;

  //  ARA_LOG_MESSAGE(LOG_DEBUG,"nfds = %d\n",nfds);

  pollVal=poll(fds, nfds, 1);
  if ( pollVal == -1) {
    //At some point will improve the error handling
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Error calling poll: %s",__FUNCTION__,strerror(errno));
    close(fAtriControlSocket);
    return -1;
  }
  
  if(pollVal==0) { 
    //Nothing has happened
    return 0;
  }
  //If we are here then something has happened
  //Now loop through connections and see what has happened
  for(tempInd=0;tempInd<nfds;tempInd++) {
    int thisFd=fds[tempInd].fd;
    if (fds[tempInd].revents & POLLHUP) {
      // done
      ARA_LOG_MESSAGE(LOG_INFO,"%s: Connection closed\n",__FUNCTION__);
      close(thisFd);
      removeFromAtriControlSocketList(thisFd);
      thisFd = 0;
    } else if (fds[tempInd].revents & POLLIN) {
      // data on the pipe
      nbytes = recv(thisFd, &controlPacket, sizeof(AtriControlPacket_t), 0); 
      if (nbytes) {
        ARA_LOG_MESSAGE(LOG_DEBUG,"%s: %d bytes from control pipe\n", __FUNCTION__,nbytes);
        
        ///Now add it to the queue and add it to the packet list
        addControlPacketToQueue(&controlPacket);
        addToAtriPacketList(thisFd,controlPacket.header.packetNumber);
        //      fprintf(stderr,"Got packet %d from thisFd %d\n",controlPacket.header.packetNumber,thisFd);
      } else {
        ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Connection closed\n",__FUNCTION__);
        close(thisFd);
        removeFromAtriControlSocketList(thisFd);
        thisFd = 0;
      }
    }  
  }
         
  return 1;
}

void addControlPacketToQueue(AtriControlPacket_t *packetPtr)
{
  //should add a check packet link here
  ///but won't for now
  pthread_mutex_lock(&atri_packet_queue_mutex);
  AtriPacketQueueFifo_t *tempQueue=(AtriPacketQueueFifo_t*)malloc(sizeof(AtriPacketQueueFifo_t));
  AtriPacketQueueFifo_t *nextQueue;
  fAtriPacketNumber++;
  
  //The software breaks if it tries packet number 0
  if(fAtriPacketNumber==0) fAtriPacketNumber++;
  packetPtr->header.packetNumber=fAtriPacketNumber;
  memcpy(&(tempQueue->controlPacket),packetPtr,sizeof(AtriControlPacket_t));
  tempQueue->next=NULL;

  if(fAtriPacketFifo==NULL) {
    fAtriPacketFifo=tempQueue;
  }
  else {
    nextQueue=fAtriPacketFifo;
    while(nextQueue->next) {
      nextQueue=nextQueue->next;
    }
    nextQueue->next=tempQueue;
  }
  pthread_mutex_unlock(&atri_packet_queue_mutex);  

  ARA_LOG_MESSAGE(LOG_DEBUG,"%s: packetNumber=%d\n",__FUNCTION__,packetPtr->header.packetNumber);

}

int getControlPacketFromQueue(AtriControlPacket_t *packetPtr)
{
  pthread_mutex_lock(&atri_packet_queue_mutex);
  AtriPacketQueueFifo_t *tempQueue=fAtriPacketFifo;
  //  ARA_LOG_MESSAGE(LOG_DEBUG,"getControlPacketFromQueue tempQueue=%d\n",tempQueue);
  if(tempQueue==NULL) {
      pthread_mutex_unlock(&atri_packet_queue_mutex);  
      return 0;
  }
  AtriPacketQueueFifo_t *nextInQueue=fAtriPacketFifo->next;  
  memcpy(packetPtr,&(tempQueue->controlPacket),sizeof(AtriControlPacket_t));
  
  free(fAtriPacketFifo);
  fAtriPacketFifo=nextInQueue;     
  pthread_mutex_unlock(&atri_packet_queue_mutex);  
  ARA_LOG_MESSAGE(LOG_DEBUG,"Got packetNumber=%d from queue\n",packetPtr->header.packetNumber);
  return 1;

}

//Now for the worker functions
void initFx2ControlSocket(char *socketPath)
{
  struct sockaddr_un u_addr; // unix domain addr
  int len;
  //Create socket
  if ((fFx2ControlSocket = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: socket -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }
  u_addr.sun_family = AF_UNIX;
  strcpy(u_addr.sun_path, socketPath);
  unlink(u_addr.sun_path);
  len = strlen(u_addr.sun_path) + sizeof(u_addr.sun_family);
  //Bind address
  if (bind(fFx2ControlSocket, (struct sockaddr *)&u_addr, len) == -1) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: bind -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }     

  //Listen and have a queue of up to 10 connections waiting
  if (listen(fFx2ControlSocket, 10)) {
    ARA_LOG_MESSAGE(LOG_ERR,"%s: listen -- %s\n",__FUNCTION__,strerror(errno));
    return;
  }

  //Now set up the lists
  pthread_mutex_init(&fx2_socket_list_mutex, NULL);    

  pthread_mutex_lock(&fx2_socket_list_mutex);
  fNumFx2Sockets=0;
  fFx2ControlSocketList = NULL;
  pthread_mutex_unlock(&fx2_socket_list_mutex);

}

int checkForNewFx2ControlConnections()
{
  // This is meant to be a simple function that just checks to see if we have an incoming
  // connection.
  struct pollfd fds[1];
  int s_dat = 0; 
  struct sockaddr_un u_inaddr; // incoming
  unsigned int u_len=sizeof(u_inaddr);
  int nfds=1;
  int pollVal;

  fds[0].fd = fFx2ControlSocket;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  
  pollVal=poll(fds,nfds,1); //non-blocking, for now at least may change at some point if I put this in a thread
  if(pollVal==-1) {
    //At some point will improve the error handling
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Error calling poll: %s",__FUNCTION__,strerror(errno));
    close(fFx2ControlSocket);
    return -1;
  }
  if(pollVal) {    
    if (fds[0].revents & POLLIN) {
      // incoming connection
      if ((s_dat = accept(fFx2ControlSocket, (struct sockaddr *) &u_inaddr, &u_len)) == -1) {
        ARA_LOG_MESSAGE(LOG_ERR,"%s: accept -- %s\n",__FUNCTION__,strerror(errno));
        close(fFx2ControlSocket);
        return -1;
      }
      ARA_LOG_MESSAGE(LOG_DEBUG,"%s: incoming connection\n",__FUNCTION__);
      addToFx2ControlSocketList(s_dat);
    }
  }  
  return fNumFx2Sockets;
}

int addToFx2ControlSocketList(int socketFd)
{
  Fx2SocketLinkedList_t *tempSocketList=NULL;
  pthread_mutex_lock(&fx2_socket_list_mutex);
  // Create new item in the list at the head of the list
  tempSocketList=(Fx2SocketLinkedList_t*)malloc(sizeof(Fx2SocketLinkedList_t));
  tempSocketList->socketFd=socketFd;
  tempSocketList->next=fFx2ControlSocketList;
  fFx2ControlSocketList=tempSocketList;
  fNumFx2Sockets++;
  pthread_mutex_unlock(&fx2_socket_list_mutex);
  return fNumFx2Sockets;
}


int removeFromFx2ControlSocketList(int socketFd)
{
  pthread_mutex_lock(&fx2_socket_list_mutex);
  Fx2SocketLinkedList_t *lastSocketLink=NULL;
  Fx2SocketLinkedList_t *tempSocketList=fFx2ControlSocketList;
  int count=0;
  //Search through list and remove the item with socketFd
  while(tempSocketList) {
    ARA_LOG_MESSAGE(LOG_DEBUG,"%s: %d -- %d -- %d\n",__FUNCTION__,count,tempSocketList->socketFd,socketFd);
    if(tempSocketList->socketFd==socketFd) {
      //Found our item in the list
      if(lastSocketLink) {
        lastSocketLink->next=tempSocketList->next;
      }
      else {
        fFx2ControlSocketList=tempSocketList->next;
      }
      free(tempSocketList);
      fNumFx2Sockets--;
      break;
    }
    lastSocketLink=tempSocketList;
    tempSocketList=tempSocketList->next;
    count++;
  }
  pthread_mutex_unlock(&fx2_socket_list_mutex);
  return fNumFx2Sockets;
}

void closeFx2Control(char *socketPath) 
{
  
  Fx2SocketLinkedList_t *tempSocketList=fFx2ControlSocketList;
  while(tempSocketList) {
    close(tempSocketList->socketFd);
    tempSocketList=tempSocketList->next;
  }
  close(fFx2ControlSocket);
  unlink(socketPath);
}

int serviceOpenFx2ControlConnections()
{
  //Okay so this essentially checks the open connections and services them in some way
  //Could be combined with the check for new connections
  struct pollfd fds[MAX_FX2_CONNECTIONS];

  Fx2ControlPacket_t tempPacket;
  Fx2ResponsePacket_t responsePacket;
  int nbytes;
  int nfds=0;
  int tempInd=0;
  int pollVal;
  int retVal;

  if(fNumFx2Sockets==0) return 0;  
  pthread_mutex_lock(&fx2_socket_list_mutex);
  Fx2SocketLinkedList_t *tempSocketList=fFx2ControlSocketList;
  //Loop through the socket list to fill the pollfd array
  while(tempSocketList) {
    fds[nfds].fd = tempSocketList->socketFd;
    fds[nfds].events = POLLIN;
    fds[nfds].revents = 0;
    nfds++;
    tempSocketList=tempSocketList->next;
  }
  pthread_mutex_unlock(&fx2_socket_list_mutex);
  if(nfds==0) return 0;

  //  ARA_LOG_MESSAGE(LOG_DEBUG,"nfds = %d\n",nfds);

  pollVal=poll(fds, nfds, 1);
  if ( pollVal == -1) {
    //At some point will improve the error handling
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Error calling poll: %s",__FUNCTION__,strerror(errno));
    close(fFx2ControlSocket);
    return -1;
  }
  
  if(pollVal==0) { 
    //Nothing has happened
    return 0;
  }

  //If we are here then something has happened
  //Now loop through connections and see what has happened
  for(tempInd=0;tempInd<nfds;tempInd++) {
    int thisFd=fds[tempInd].fd;
    if (fds[tempInd].revents & POLLHUP) {
      // done
      ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Connection closed\n",__FUNCTION__);
      close(thisFd);
      removeFromFx2ControlSocketList(thisFd);
      thisFd = 0;
    } else if (fds[tempInd].revents & POLLIN) {
      // data on the pipe
      nbytes = recv(thisFd, (char*)&tempPacket, sizeof(Fx2ControlPacket_t), 0); 
      if (nbytes) {
        ARA_LOG_MESSAGE(LOG_DEBUG,"%s: %d bytes from control pipe\n",__FUNCTION__, nbytes);
        retVal=sendVendorRequestStruct(&tempPacket);
        memcpy(&(responsePacket.control),&tempPacket,sizeof(Fx2ControlPacket_t));
        responsePacket.status=retVal;           
        
        if (send(thisFd, (char*)&responsePacket, sizeof(Fx2ResponsePacket_t), 0) == -1) {
          ARA_LOG_MESSAGE(LOG_ERR,"%s: send -- %s\n",__FUNCTION__,strerror(errno));
          exit(1);
        }        
        
      } else {
        ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Connection closed\n",__FUNCTION__);
        close(thisFd);
        removeFromFx2ControlSocketList(thisFd);
        thisFd = 0;
      }
    }    
  }
  return 1;

}

///////////////////////////////////////////////////////////////////////////

//////////////////////
// THIS IS OUR CRAP //
//////////////////////

void enablePcieEndPoint() {
  return;
}

void disablePcieEndPoint() {
  return;
}

int openPcieDevice() {
  return 0;
}

int closePcieDevice() {
  return 0;
}

int closeFx2Device() {
  ARA_LOG_MESSAGE(LOG_DEBUG,"closeFx2Device with NO FX2 LIBRARY\n");
  mcpComLib_finish();
  return SUCCEED;
}

int openFx2Device() {
  int i;
  ARA_LOG_MESSAGE(LOG_DEBUG,"openFx2Device with NO FX2 LIBRARY\n");

  // THIS IS RYAN SILLINESS EXCEPT COMMENTED CURRENT HANDLE 
  //  pthread_mutex_init(&async_buffer_mutex,NULL);
  fNumAsyncBuffers=0;
  fNumAsyncRequests=0;

  pthread_mutex_init(&libusb_command_mutex, NULL);  
  pthread_mutex_lock(&libusb_command_mutex);
  //  struct libusb_context *contextPtr=&fUsbContext;
  //  currentHandle=INVALID_HANDLE_VALUE;
  for(i=0;i<NUM_ASYNC_BUFFERS;i++) asyncBufferStatus[i]=0;
  // END RYAN SILLINESS
  
  // OUR SILLINESS
  mcpComLib_init();
  controlReadFd = open("/tmp/atri_inpkts", O_RDONLY);
  controlWriteFd = open("/dev/xillybus_pkt_in", O_WRONLY);
  if (controlReadFd < 0 || controlWriteFd < 0) {
    return -1;
  }
  pthread_mutex_unlock(&libusb_command_mutex);		       
}

int flushControlEndPoint() {
  return 0;
}

int flushEventEndPoint() {
  return 0;
}

// suuuuch silly pointlessness
int sendVendorRequestStruct(Fx2ControlPacket_t *controlPacket)
{
  return sendVendorRequest(controlPacket->bmRequestType,
                           controlPacket->bRequest,
                           controlPacket->wValue,
                           controlPacket->wIndex,
                           controlPacket->data,
                           controlPacket->dataLength);
}
// even mooooore silliness
// this is direct Ryan silliness copy except we cleanup a bit of garbage
int sendVendorRequest(uint8_t bmRequestType, uint8_t bRequest, uint16_t wValue, uint16_t wIndex, unsigned char *data, uint16_t wLength)
{
  int ind=0;
  pthread_mutex_lock(&libusb_command_mutex);

  ARA_LOG_MESSAGE(LOG_DEBUG,"%s: Got vendor request: bmRequestType=%#x bRequest=%#x wValue=%#x wIndex=%#x wLength=%d data[0]=%#x data[1]=%#x\n",__FUNCTION__,bmRequestType,bRequest,wValue,wIndex,wLength,data[0],data[1]);

  if(bmRequestType&0x80) {
    for(ind=0;ind<wLength;ind++) {
      data[ind]=0;
    }
  }
  int retVal=sendVendorRequestMcp(bmRequestType, bRequest, wValue, wIndex, data, wLength);
  
  if(retVal<0){
    ARA_LOG_MESSAGE(LOG_ERR,"%s: Could not send libusb_control_transfer (vendor request): %d %s\n", __FUNCTION__,retVal,getLibUsbErrorAsString(retVal));
    pthread_mutex_unlock(&libusb_command_mutex);
    return retVal;
  }
  pthread_mutex_unlock(&libusb_command_mutex);
  return retVal;
}



// VERY TEMPORARY
int readControlEndPoint(unsigned char *buffer,
			int numBytes,
			int *numBytesRead) {
  uint8_t *dp = buffer+4;
  int rb;
  // goddamn it.
  // the difficulty with all of this is that Ryan's original code was
  // extremely basic and just assumed you __always__ got a full packet
  // from every "readControlEndPoint" because USB is packet based.
  //
  // Except our new library is not packet based, it's stream based
  // since Xillybus's EOF stuff is godawful.
  //
  // So now we have to remember how the damn crap works in the first
  // place.
  // AtriControlPacket_t consists of a header and a variable data field.
  // The header looks like
  // < bbl [data] >
  // where bb are unimportant, and l is the data length.
  //
  // so our tactic here is we're only ever going to read 1 packet
  // at a time. We do this by polling with a timeout, and then
  // reading a single packet and returning. Eff it.
  struct pollfd pfd;
  int retval;
  pfd.fd = controlReadFd;
  pfd.events = POLLIN;
  retval = poll(&pfd, 1, 10);  
  if (!retval) {
    return -7;
  }
  if (!(pfd.revents & POLLIN)) {
    return -7;
  }
  printf("readControlEndPoint has something\n");
  // read the control packet header 
  read(controlReadFd, buffer, 4);
  rb = 4;
  // now read the data + trailer
  read(controlReadFd, dp, buffer[3] + 1);
  rb += buffer[3] + 1;
  // damnit
  *numBytesRead = rb;
  for (int i=0;i<rb;i++) {
    printf("%2.2x", buffer[i]);
    if (!((i+1)%16)) printf("\n");
    else if (i+1 < numBytes) printf(" ");
    else printf("\n");
  }
  return 0;
}

int writeControlEndPoint(unsigned char *buffer,
			 int numBytes,
			 int *numBytesSent) {
  ssize_t rv;
  printf("writeControlEndPoint: %d bytes\n", numBytes);
  for (int i=0;i<numBytes;i++) {
    printf("%2.2x", buffer[i]);
    if (!((i+1)%16)) printf("\n");
    else if (i+1 < numBytes) printf(" ");
    else printf("\n");
  }
  rv = write(controlWriteFd, buffer, numBytes);
  if (rv < 0) return rv;
  *numBytesSent = rv;
  return 0;
}

int readEventEndPoint(unsigned char *buffer,
		      int numBytes,
		      int *numBytesRead) {
  usleep(100);
  // just say 0
  *numBytesRead = 0;
  return 0;
}

// lol
int writeEventEndPoint(unsigned char *buffer,
		       int numBytes,
		       int *numByteSent) {
  return 0;
}

// lol no
char *getLibUsbErrorAsString(int error) {
  return "this is a noFX2 lib";
}

// we do not need the eventEndPointCallback
// because it's static

// I don't think this matters
int readEventEndPointFromSoftwareBuffer(unsigned char *buffer,
					int numBytes,
					int *numBytesRead) {
  fprintf(stderr, "no you actually do need readEventEndPointFromSoftwareBuffer");
  return 0;
}

// also don't think this matters
void submitEventEndPointRequest() {
  fprintf(stderr, "no actually submitEventEndPointRequest does matter");
  return;
}

void pollForUsbEvents() {
  fprintf(stderr, "someone called pollForUsbEvents");
  return;
}

// these are also ryan crap
int getNumAsyncBuffers()
{
  pthread_mutex_lock(&async_buffer_mutex);
  int val=fNumAsyncBuffers;
  pthread_mutex_unlock(&async_buffer_mutex);
  return val;
}

int getNumAsyncRequests()
{
  pthread_mutex_lock(&async_buffer_mutex);
  int val=fNumAsyncRequests;
  pthread_mutex_unlock(&async_buffer_mutex);
  return val;
}
