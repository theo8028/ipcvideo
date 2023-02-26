// Ipc video share service 
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#include <stdio.h>
#include <vector>
#include <windows.h>
#include <Memoryapi.h>
#include <mutex >
#include <string>
#include <iostream>
#include <sstream>
#define  EXTERN_IPC 
#include "ipcvideo_base.h"
#include "slogger.h"

using namespace std;

HANDLE 				  g_hSMemVideoHandle=NULL;
char*  			      g_pSMemVideoBuf=NULL;

HANDLE                g_lockSMem[MAX_CHANNEL];  
HANDLE                g_eventSMem[MAX_CHANNEL];

ShareMemHeader*		  g_pSMemVideoHeader[MAX_CHANNEL];
ShareMemTail*		  g_pSMemVideoTail[MAX_CHANNEL];

char*                 g_pSMemVideoPtr[MAX_CHANNEL];
int 				  g_FrameCount[MAX_CHANNEL];

string toHexString(void *ptr)
{
	stringstream convert_invert;
	convert_invert << std::hex << ptr;
	return convert_invert.str();
}

int clock_gettime(int, struct timespec *spec)    
{ 
   __int64 wintime; 
   GetSystemTimeAsFileTime((FILETIME*)&wintime);
   wintime      -=116444736000000000i64;		   // 1jan1601 to 1jan1970
   spec->tv_sec  =wintime / 10000000i64;           // seconds
   spec->tv_nsec =wintime % 10000000i64 *100;      // nano-seconds
   return 0;
}

/// <summary>
/// IPC Vidoe 공유 서비스를 시작한다.
/// </summary>
/// <param name="szIpcName">IPC Name </param>
/// <returns>True or False </returns>
bool openShareVideo(const char *szIpcName)
{
	slog_trace("openShareVideo CreateFileMapping %s",szIpcName);
	g_hSMemVideoHandle = ::CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SMEM_TOTAL_SIZE, szIpcName);
	if( g_hSMemVideoHandle)
	{
		g_pSMemVideoBuf = (char *)MapViewOfFile( g_hSMemVideoHandle,   // handle to map object
												 FILE_MAP_ALL_ACCESS, // read/write permission
												 0,
												 0,
												 SMEM_TOTAL_SIZE);
		if (g_pSMemVideoBuf == NULL)
		{
			return false;
		}
		for (int ch = 0; ch < MAX_CHANNEL; ch++)
		{
			g_pSMemVideoHeader[ch] = (ShareMemHeader*)(&g_pSMemVideoBuf[SMEM_CHANNEL_SIZE * ch]);
			g_pSMemVideoPtr[ch]    = (char*)(&g_pSMemVideoBuf[SMEM_CHANNEL_SIZE * ch + SMEM_HEADER_SIZE]);
			g_pSMemVideoTail[ch]   = (ShareMemTail*)(&g_pSMemVideoBuf[SMEM_CHANNEL_SIZE * (ch+1)  - SMEM_HEADER_SIZE]);
			g_FrameCount[ch]       = -1;

			if (g_pSMemVideoHeader[ch]->InitKeyCode != INIT_KEY_CODE)
			{
				memset(g_pSMemVideoHeader[ch], 0, SMEM_CHANNEL_SIZE);
				g_pSMemVideoHeader[ch]->headerSize  = sizeof(ShareMemHeader);
				g_pSMemVideoHeader[ch]->InitKeyCode = INIT_KEY_CODE;				
			}
			g_pSMemVideoHeader[ch]->referenceCount++;
			slog_info("InitShareVideo : ch(%d) referenceCount( %d)\n",ch,g_pSMemVideoHeader[ch]->referenceCount);

			char szShareMutexName[MAX_PATH];
			sprintf_s(szShareMutexName,"%s%s%d",szIpcName,"_mutex_",ch);
	
			g_lockSMem[ch] = ::CreateMutexA(nullptr, FALSE, szShareMutexName);
			if (g_lockSMem[ch] == 0)
			{
				slog_error("openShareVideo : Mutex(%s) create fail\n", szShareMutexName);
				return false;
			}
			char szEventName[MAX_PATH];
			sprintf_s(szEventName,"%s%s%d",szIpcName,"_event_",ch);
	
			g_eventSMem[ch] = CreateEventA(NULL,TRUE,FALSE,szEventName);
			if (g_eventSMem[ch] == 0)
			{
				slog_error("openShareVideo : Event(%s) create fail\n", szEventName);
				return false;
			}
		}
		return true;
	}
	else
	{
		DWORD errorCode = GetLastError();
		CHAR message[255];

		if( !FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM ,
		 		nullptr,
				errorCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				message,
				255,
				nullptr) ){
			slog_error("openShareVideo : CreateFileMapping/OpenFileMappingA failed with 0x%x\n", GetLastError());
		}else{
			slog_error("openShareVideo : CreateFileMapping/OpenFileMappingA Error(%d) : %s\n", errorCode, message);
		}
	}
	return false;
}

bool isOpenedShareVideo()
{
	if (g_hSMemVideoHandle)
	{
		return true;		
	}
	return false;
}


/// <summary>
/// IPC Vidoe 공유 서비스를 종료한다.
/// </summary>
/// <returns>True or False</returns>
bool closeShareVideo()
{
	if (g_hSMemVideoHandle)
	{
		for (int ch = 0; ch < MAX_CHANNEL; ch++)
		{
			if (g_lockSMem[ch])
			{
				CloseHandle(g_lockSMem[ch]);
				g_lockSMem[ch] = NULL;
			}
			if (g_eventSMem[ch])
			{
				CloseHandle(g_eventSMem[ch]);
				g_eventSMem[ch] = NULL;
			}
		}
		if (!CloseHandle(g_hSMemVideoHandle))
		{
			slog_error("closeShareVideo : fail\n");
			return false;
		}
		g_hSMemVideoHandle = NULL;
		return true;
	}
	return false;
}

/// <summary>
/// image 데이터를 지정된 채널의 공유메모리 위치로 복사
/// </summary>
/// <param name="ch">channel</param>
/// <param name="pSrcImg">source image buffer </param>
/// <param name="bufSize">image buffer size</param>
/// <param name="imgWidth">image width</param>
/// <param name="imgHeight">image height</param>
/// <param name="imgDepth">image depth</param>
/// <param name="pixelFormat">pixel format</param>
/// <param name="pFrameID">이미지를 구분하기 위한 frame Id 획득</param>
/// <returns>True or False</returns>
bool writeShareVideo(int ch, char *pSrcImg,int bufSize,int imgWidth,int imgHeight,int imgDepth,int pixelFormat,int *pFrameID)
{
	int imgSize = imgWidth * imgHeight * imgDepth;

	if(g_pSMemVideoPtr[ch] == 0 || imgSize<=0)
		return false;

	if (bufSize > FRAME_BUFFER_SIZE)
	{
		slog_error("Insufficient internal memory buffer");
		return false;
	}

	DWORD dwRet = WaitForSingleObject(g_lockSMem[ch], INFINITE);
	if ( dwRet == WAIT_OBJECT_0 || dwRet == WAIT_ABANDONED )
	{
		g_pSMemVideoHeader[ch]->frameCount++;
		if( pFrameID ){
			*pFrameID = g_pSMemVideoHeader[ch]->frameCount;
		}

		g_pSMemVideoHeader[ch]->imgSize   = imgSize;
		g_pSMemVideoHeader[ch]->imgWidth  = imgWidth;
		g_pSMemVideoHeader[ch]->imgHeight = imgHeight;	
		g_pSMemVideoHeader[ch]->imgDepth  = imgDepth;
		g_pSMemVideoHeader[ch]->pixelFormat = pixelFormat;
		clock_gettime(0,&g_pSMemVideoHeader[ch]->time);

		int nCpSize = imgSize;
		if (imgSize > bufSize){
			nCpSize = bufSize;
		}
		memcpy(g_pSMemVideoPtr[ch], pSrcImg, nCpSize);
		g_pSMemVideoTail[ch]->frameCount = g_pSMemVideoHeader[ch]->frameCount;
		ReleaseMutex(g_lockSMem[ch]);		
		SetEvent(g_eventSMem[ch]);
		slog_info("writeShareVideo :  ch(%d), frameCount(%d)\n",ch, g_pSMemVideoHeader[ch]->frameCount);
		return true;
	}
	return false;
}

/// <summary>
/// 지정된 채널 공유메모리의 이미지 읽기
/// </summary>
/// <param name="ch">channel</param>
/// <param name="pDestImg">destination image buffer </param>
/// <param name="bufSize">image buffer size</param>
/// <param name="pImgWidth">image width </param>
/// <param name="pImgHeight">image height</param>
/// <param name="pImgDepth">image depth</param>
/// <param name="pPixelFormat">pixel foramt</param>
/// <param name="pFrameCount">frame id</param>
/// <param name="pLatencySec">Delay time in seconds to transmit and receive </param>
/// <returns>image read size </returns>
int readShareVideo(int ch,char *pDestImg,int bufSize,int *pImgWidth,int *pImgHeight,int *pImgDepth,
			      int *pPixelFormat,int *pFrameCount,double *pLatencySec )
{
	if (g_pSMemVideoHeader[ch] == NULL){
		return 0;	
	}

	if (bufSize > FRAME_BUFFER_SIZE)
	{
		slog_error("Insufficient internal memory buffer");
		return 0;
	}

	if(g_pSMemVideoPtr[ch] == 0)
		return 0;

	if(g_FrameCount[ch] == g_pSMemVideoHeader[ch]->frameCount)
	{
		DWORD t1 = GetTickCount();
		DWORD dwWaitResult = WaitForSingleObject(g_eventSMem[ch], IMAGE_WAIT_MS);
		DWORD t2 = GetTickCount();

		if (g_FrameCount[ch] == g_pSMemVideoHeader[ch]->frameCount) {
			DWORD dt = t2 - t1;
			// Manually ResetEvent considering the situation in which multiple programs receive events
			if (dt == 0)
			{
				ResetEvent(g_eventSMem[ch]);
				Sleep(10);
			}
			slog_info("ipcvideo_base::readShareVideo : image not ready , wait(%d)\n",dt);
			return 0;
		}
	}

	int imgSize = 0;
	DWORD dwRet = WaitForSingleObject(g_lockSMem[ch], INFINITE);
	if (dwRet == WAIT_OBJECT_0 || dwRet == WAIT_ABANDONED)
	{
		imgSize = g_pSMemVideoHeader[ch]->imgSize;
		if (bufSize < g_pSMemVideoHeader[ch]->imgSize) {
			imgSize = bufSize;
		}
		memcpy(pDestImg, g_pSMemVideoPtr[ch], imgSize);
		*pImgWidth = g_pSMemVideoHeader[ch]->imgWidth;
		*pImgHeight = g_pSMemVideoHeader[ch]->imgHeight;
		*pImgDepth  = g_pSMemVideoHeader[ch]->imgDepth;
		*pPixelFormat = g_pSMemVideoHeader[ch]->pixelFormat;
		g_FrameCount[ch] = g_pSMemVideoHeader[ch]->frameCount;	
		if (pFrameCount) {
			*pFrameCount = g_FrameCount[ch];		
		}
		if (pLatencySec)
		{
			struct timespec tmInput = g_pSMemVideoHeader[ch]->time;
			struct timespec tmOut;
			clock_gettime(0, &tmOut);
			double diff_sec  = (double)( tmOut.tv_sec - tmInput.tv_sec);
			double diff_nsec = tmOut.tv_nsec - tmInput.tv_nsec;
			*pLatencySec = diff_sec + diff_nsec / 1000000000;
		}
		ReleaseMutex(g_lockSMem[ch]);
		slog_trace("readShareVideo : success\n");
		return imgSize;
	}
	return 0;	
}

/// <summary>
/// 지정된 채널의 공유메모리에 저장되어있는 이미지의 포맷 정보 
/// </summary>
/// <param name="ch"> channel</param>
/// <param name="pImgWidth">imgage widtd</param>
/// <param name="pImgHeight">image height</param>
/// <param name="pImgDepth">image depth</param>
/// <param name="pPixelFmt">pixel format</param>
/// <returns>True or False</returns>
bool getImageInfo(int ch, int* pImgWidth, int* pImgHeight,int *pImgDepth,int *pPixelFmt)
{
	if (g_pSMemVideoHeader)
	{
		*pImgWidth = g_pSMemVideoHeader[ch]->imgWidth;
		*pImgHeight = g_pSMemVideoHeader[ch]->imgHeight;
		*pImgDepth = g_pSMemVideoHeader[ch]->imgDepth;
		*pPixelFmt = g_pSMemVideoHeader[ch]->pixelFormat;
		return true;
	}
	return false;
}
