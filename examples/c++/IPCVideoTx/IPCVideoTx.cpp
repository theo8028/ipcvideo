// IPCVideoTx.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include <conio.h>
#include "../../../src/ipcvideo_base.h"

int main()
{
	int imgWidth, imgHeight,imgDepth,pixelFmt;
	int frameCount   = 0;
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
			if (writeShareVideo(inputChannel, pImageData, bufferSize, imgWidth, imgHeight, imgDepth, pixelFmt,&frameCount) )
			{
				printf("ipcvideo image tx : channel(%d) frameCount(%d)\n", inputChannel, frameCount);
				Sleep(10);
			}

			if (_kbhit())
			{
				if (_getch() == 27) //ESC
					break;
			}
		} while (1);
	}
	delete [] pImageData;
}
