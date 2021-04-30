#include "RenderingWindow.h"
#include "Media.h"
#include <Windows.h>
#include <stdio.h>

#include <iostream>

struct BGRAPixel
{
	BYTE b;
	BYTE g;
	BYTE r;
	BYTE a;
};
void RGB24_to_BGRA32(BGRAPixel* destinationBuffer, const BYTE* sourceBuffer, DWORD  _width, DWORD  _height)
{
	if (destinationBuffer == NULL || sourceBuffer == NULL)
		return;
	UINT totalPixels = _width*_height;
	RGBTRIPLE *sourceRGB24 = (RGBTRIPLE*)sourceBuffer;
	DWORD c2 = totalPixels - 1;
	for (DWORD c = 0; c <totalPixels; c++)
	{
		destinationBuffer[c].b = sourceRGB24[c2].rgbtBlue;
		destinationBuffer[c].g = sourceRGB24[c2].rgbtGreen;
		destinationBuffer[c].r = sourceRGB24[c2].rgbtRed;
		destinationBuffer[c].a = 255;
		c2--;
	}

}

__forceinline BYTE Clip(int clr)
{
	return (BYTE)(clr < 0 ? 0 : (clr > 255 ? 255 : clr));
}

BGRAPixel ConvertYCrCbToRGB(int  	y,
	int  	cr,
	int  	cb
)
{
	BGRAPixel rgbq;
	
	int c = y - 16;
	int d = cb - 128;
	int e = cr - 128;
	
	rgbq.r = Clip((298 * c + 409 * e + 128) >> 8);
	rgbq.g = Clip((298 * c - 100 * d - 208 * e + 128) >> 8);
	rgbq.b = Clip((298 * c + 516 * d + 128) >> 8);
	
	return rgbq;
}

/// Note: this is taken from https://software.intel.com/sites/landingpage/mmsf/documentation/preview_8cpp.html#af3d9de67be8e955b824d4b497bba4c96
/// In the online version (from Intel) the comment below on Byte order was incorrect - although the code was fine (!). Never trust documentation..(?)
void TransformImage_YUY2(BYTE* pDest,
	LONG  	lDestStride,
	const BYTE* pSrc,
	LONG  	lSrcStride,
	DWORD  	dwWidthInPixels,
	DWORD  	dwHeightInPixels
)
{
	if (pDest == NULL || pSrc == NULL)
		return;

	for (DWORD y = 0; y < dwHeightInPixels; y++)
	{
		BGRAPixel* pDestPel = (BGRAPixel*)pDest;
		WORD * pSrcPel = (WORD*)pSrc;

		for (DWORD x = 0; x < dwWidthInPixels; x += 2)
		{
			// Byte order is Y0 U0 Y1 V0  /// NOTE: On the Intel site this comment was wrong.

			int y0 = (int)LOBYTE(pSrcPel[x]);
			int u0 = (int)HIBYTE(pSrcPel[x]);
			int y1 = (int)LOBYTE(pSrcPel[x + 1]);
			int v0 = (int)HIBYTE(pSrcPel[x + 1]);

			pDestPel[x] = ConvertYCrCbToRGB(y0, v0, u0);
			pDestPel[x + 1] = ConvertYCrCbToRGB(y1, v0, u0);
		}

		pSrc +=  lSrcStride;
		pDest += lDestStride;
	}

}



int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine,int nCmdShow)
{
	//debug console
	AllocConsole();
	AttachConsole(GetCurrentProcessId());
	freopen("CON", "w", stdout);
	
	
	Media* m = new Media();
	m->CreateCaptureDevice();

	BGRAPixel *bgraBuffer = new BGRAPixel[m->width* m->height];
	wchar_t titleString[2048];
	wsprintf(titleString, L"Microsoft Media Foundation Example: %ls %d x %d",m->deviceNameString, m->width, m->height);
	RenderingWindow window(titleString, m->width, m->height, nCmdShow);
	
	MSG msg{ 0 };
	

	while (msg.message != WM_QUIT)
	{
		if(m->videoFormat == MFVideoFormat_RGB24)
		{ 
			RGB24_to_BGRA32(bgraBuffer, m->rawData, m->width, m->height);
		}
		else if (m->videoFormat == MFVideoFormat_YUY2)
		{
			// Dest stride in the following call is hardwired to 4 * width because it is RGBA (i.e. 4 bytes per pixel)
			TransformImage_YUY2((BYTE*)(bgraBuffer),4* m->width, m->rawData, m->stride, m->width, m->height);
		}
		else if (m->videoFormat == MFVideoFormat_RGB32)
		{
			std::cerr << "Unsupported type MFVideoFormat_RGB32" << std::endl;
		}
		else if (m->videoFormat == MFVideoFormat_NV12)
		{
			std::cerr << "Unsupported type MFVideoFormat_NV12" << std::endl;
		}
		else
		{
			// Houston we have a problem...
			/// Note: people are welcome to paste the relevant function here to test
			std::cerr << "Unsupported video type" << std::endl;
		}

		
		window.Draw((BYTE*)bgraBuffer, m->width, m->height);
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (window.windowHandle && IsDialogMessage(window.windowHandle, &msg))
			{
				continue;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	}

	m->Release();
	delete bgraBuffer;
	return 0;
}