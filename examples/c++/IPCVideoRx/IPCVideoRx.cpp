// IPCVideoRx.cpp : IPCVideo Image Receive Example
//

#include <iostream>
#include "../../../src/ipcvideo_base.h"
#include <conio.h>
int main()
{
  int imgWidth, imgHeight, imgFmt,imgDepth;
  int    frameID = 0;
  int    channel = 0;
  double latency_sec = 0;
  char*  pFrameBuffer = new char[FRAME_BUFFER_SIZE];
  //const char* szIPCName = "Global\\IPCVIDEO"; // Execute with administrator privileges
  const char* szIPCName = "IPCVIDEO";
  
  printf("IPC video share service : ipc name(%s) , rx channel(%d)\n", szIPCName, channel);

  if (openShareVideo(szIPCName))
  {
    do
    {
      if (readShareVideo(channel, pFrameBuffer, FRAME_BUFFER_SIZE, &imgWidth, &imgHeight, &imgFmt,&imgDepth,&frameID,&latency_sec))
      {
        printf("ipcvideo image rx : channel(%d) frameID(%d), latency(%.3fms) [%02x][%02x][%02x][%02x]\n", channel, frameID, latency_sec*1000,0xff&pFrameBuffer[0], 0xff & pFrameBuffer[1], 0xff & pFrameBuffer[2], 0xff & pFrameBuffer[3]);
      }
      
      if (_kbhit())
      {
        if (_getch() == 27) //ESC
          break;
      }
    }while (1);
    closeShareVideo();
  }
  
  delete[] pFrameBuffer;
}

