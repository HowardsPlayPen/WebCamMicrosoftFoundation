#include "RenderingWindow.h"

RenderingWindow::RenderingWindow(LPWSTR name, int width, int height,int cmd)
{
	factory = NULL;
	d2dBitmap = NULL;
	renderTarget = NULL;

	WNDCLASSEX wc{ 0 };
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor = (HCURSOR)LoadCursor(NULL,IDC_ARROW);
	wc.lpfnWndProc = RenderingWindow::MsgRouter;
	wc.lpszClassName = TEXT("WindowClass");
	wc.style = CS_VREDRAW | CS_HREDRAW;

	RegisterClassEx(&wc);
	windowHandle = CreateWindowEx(NULL, wc.lpszClassName, name, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, width, height, NULL, NULL, NULL, this);

	ShowWindow(windowHandle, cmd);
	UpdateWindow(windowHandle);

	//Initialize D2D
	if (renderTarget == NULL)
	{
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
		D2D1_SIZE_U size = D2D1::SizeU(width, height);
		D2D1_RENDER_TARGET_PROPERTIES rtProperties = D2D1::RenderTargetProperties();
		rtProperties.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE);
		rtProperties.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
		factory->CreateHwndRenderTarget(rtProperties, D2D1::HwndRenderTargetProperties(windowHandle, size), &renderTarget);
		renderTarget->CreateBitmap(size,D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
			&d2dBitmap);
	}
}
RenderingWindow::~RenderingWindow()
{
	if (factory)
	{
		factory->Release();
		factory = NULL;
	}
	if (renderTarget)
	{
		renderTarget->Release();
		renderTarget = NULL;
	}
	if (d2dBitmap)
	{
		d2dBitmap->Release();
		d2dBitmap = NULL;
	}

}
void RenderingWindow::Draw(BYTE* pixels,int width, int height) 
{
	d2dBitmap->CopyFromMemory(NULL, pixels, width * 4);
	renderTarget->BeginDraw();
	renderTarget->DrawBitmap(d2dBitmap);
	renderTarget->EndDraw();
}

LRESULT CALLBACK RenderingWindow::MsgRouter(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RenderingWindow* app;
	if (msg == WM_CREATE)
	{
		app = (RenderingWindow*)(((LPCREATESTRUCT)(lParam))->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)app);
	}
	else
	{
		app = (RenderingWindow*)GetWindowLongPtr(hWnd,GWLP_USERDATA);
	}
	return app->WndProc(hWnd,msg,wParam,lParam);
}

LRESULT CALLBACK RenderingWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		CloseWindow(hWnd);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd,msg,wParam,lParam);
		break;
	}
	return 0;
}