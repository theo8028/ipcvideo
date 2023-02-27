// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"
#include "../../../src/ipcvideo_base.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

extern "C" 	__declspec(dllexport)int ipcv_open(const char* szIpcName)
{
    return openShareVideo(szIpcName);    
}

extern "C" 	__declspec(dllexport)int ipcv_getImageInfo(int ch, int* pImgWidth, int* pImgHeight, int* pImgDepth, int* pPixelFmt)
{
    return getImageInfo(ch, pImgWidth, pImgHeight, pImgDepth, pPixelFmt);
}

extern "C" 	__declspec(dllexport)int ipcv_readShareVideo(int ch, char* pDestImg, int bufSize, int* pImgWidth,
                                            int* pImgHeight, int* pImgDepth, int* pPixelFormat, int* pFrameCount, double* pLatencySec)
{
    return readShareVideo(ch, pDestImg, bufSize,  pImgWidth, pImgHeight, pImgDepth,
        pPixelFormat, pFrameCount,  pLatencySec);
}

extern "C" 	__declspec(dllexport)int ipcv_writeShareVideo(int ch, char* pDestImg, int bufSize, int imgWidth, int imgHeight, int imgDepth, int pixelFormat, int* pFrameCount)
{
    return writeShareVideo( ch, pDestImg, bufSize, imgWidth, imgHeight, imgDepth, pixelFormat, pFrameCount);
}

extern "C" 	__declspec(dllexport)int ipcv_close()
{
    return closeShareVideo();
}