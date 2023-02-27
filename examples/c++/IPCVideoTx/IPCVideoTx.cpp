// IPCVideoTx.cpp : Ipc video tx example
//

#include <iostream>
#include <conio.h>
#include "../../../src/ipcvideo_base.h"

int main()
{
  int imgWidth, imgHeight,imgDepth,pixelFmt;
  int frameID   = 0;
  int inputChannel = 1;
  
  imgWidth = 1920;
  imgHeight = 1080;
  imgDepth = 3;
  pixelFmt = 0;
  
  int bufferSize = imgWidth * imgHeight * 3;
  char* pImageData =  new char[bufferSize];
  //const char* szIPCName = "Global\\IPCVIDEO";   // Execute with administrator privileges
  const char* szIPCName = "IPCVIDEO";
  
  if (openShareVideo(szIPCName))
  {
    do
    {
      memset(pImageData, rand(), bufferSize);
      if(writeShareVideo(inputChannel, pImageData, bufferSize, imgWidth, imgHeight, imgDepth, pixelFmt,&frameID) )
      {
        printf("ipcvideo image tx : channel(%d) frameID(%d)\n", inputChannel, frameID);
        Sleep(10);
      }
      
      if(_kbhit())
      {
        if(_getch() == 27) //ESC
          break;
      }
    } while (1);
  }
  delete [] pImageData;
}
