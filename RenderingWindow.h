#pragma once
//include and lib dependencies for Media Foundation
#include <Windows.h>
#include <d2d1.h>
#pragma comment(lib,"d2d1")

class RenderingWindow
{
	static LRESULT CALLBACK MsgRouter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	
	//D2D stuff
	ID2D1Factory* factory;
	ID2D1HwndRenderTarget* renderTarget;
	ID2D1Bitmap* d2dBitmap;
public:
	HWND windowHandle;
	RenderingWindow(LPWSTR name, int width, int height,int cmd);
	~RenderingWindow();
	void Draw(BYTE* pixels,int width, int height);
};