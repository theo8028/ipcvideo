#ifndef IPCVIDEO_HEADER
#define IPCVIDEO_HEADER

#include <stdio.h>
#include <vector>
#include <windows.h>
#include <Memoryapi.h>
#include <mutex >

#define MAX_CHANNEL		  	2           
#define FRAME_BUFFER_SIZE   10000000

#define SMEM_HEADER_SIZE    1024								
#define SMEM_CHANNEL_SIZE   (FRAME_BUFFER_SIZE+SMEM_HEADER_SIZE*2)	
#define SMEM_TOTAL_SIZE	    (SMEM_CHANNEL_SIZE*MAX_CHANNEL) 
#define INIT_KEY_CODE       0x12345678
#define IMAGE_WAIT_MS 		30

#define IPCV_PixelFormat_BGR24  0 // Default
#define IPCV_PixelFormat_Gray   1

#pragma pack(push, 1)  

struct ShareMemHeader
{
	short 	 headerSize;
	int 	 frameCount;
	int 	 InitKeyCode; 		 // first run check
	int 	 imgSize;
	int 	 imgWidth;
	int 	 imgHeight;
	int      imgDepth;
	int 	 pixelFormat;
	timespec time;
	int 	 referenceCount;
};

struct ShareMemTail
{
	short headerSize;
	int frameCount;
};

#pragma pack(pop) 

bool openShareVideo(const char *szIpcName);
bool writeShareVideo(int ch,char *pMemVideo,int bufSize, int imgWidth, int imgHeight,int imgDepth,int pixelFormat=0,int *pFrameCount=NULL);
int  readShareVideo(int ch,char *pDestImg, int bufSize, int *pImgWidth, int *pImgHeight,
					       int *pImgDepth,int *pImgFmt,int *pFrameCount ,double *pLatencySec);
bool getImageInfo(int ch,int* pImgWidth, int* pImgHeight,int *pImgDepth,int *pPixelFmt);
bool isOpenedShareVideo();
bool closeShareVideo();

#endif // IPCVIDEO_HEADER