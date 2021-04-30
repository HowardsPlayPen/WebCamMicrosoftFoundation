#include "RenderingWindow.h"
#include "Media.h"
#include <Windows.h>
#include <stdio.h>

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
		RGB24_to_BGRA32(bgraBuffer,m->rawData, m->width, m->height);
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