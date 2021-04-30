#pragma once
//media foundation headers
#include <Windows.h>
#include <mfidl.h> 
#include <Mfapi.h> 
#include <Mfreadwrite.h>
#include <Shlwapi.h>

//include and lib dependencies for Media Foundation
#pragma comment(lib,"Mfplat.lib")
#pragma comment(lib,"Mf.lib")
#pragma comment(lib,"Mfreadwrite.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"shlwapi.lib")

#include <stdio.h>

#define CLEAN_ATTRIBUTES() if (attributes) { attributes->Release(); attributes = NULL; }for (DWORD i = 0; i < count; i++){if (&devices[i]) { devices[i]->Release(); devices[i] = NULL; }}CoTaskMemFree(devices);return hr;


class Media : public IMFSourceReaderCallback //this class inhertis from IMFSourceReaderCallback
{
	CRITICAL_SECTION criticalSection;
	long referenceCount;
	WCHAR                   *wSymbolicLink;
	UINT32                  cchSymbolicLink;
	IMFSourceReader* sourceReader;
	
	
public:
	LONG stride;
	int bytesPerPixel;
	GUID videoFormat; // MFVideoFormat_RGB32 / MFVideoFormat_RGB24 / MFVideoFormat_YUY2 / MFVideoFormat_NV12
	UINT height;
	UINT width;
	WCHAR deviceNameString[2048];
	BYTE* rawData;

	HRESULT CreateCaptureDevice();
	HRESULT SetSourceReader(IMFActivate *device);
	HRESULT IsMediaTypeSupported(IMFMediaType* type);
	HRESULT GetDefaultStride(IMFMediaType *pType, LONG *plStride);
	HRESULT Close();
	Media();
	~Media();	

	// the class must implement the methods from IUnknown 
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	//  the class must implement the methods from IMFSourceReaderCallback 
	STDMETHODIMP OnReadSample(HRESULT status, DWORD streamIndex, DWORD streamFlags, LONGLONG timeStamp, IMFSample *sample);
	STDMETHODIMP OnEvent(DWORD, IMFMediaEvent *);
	STDMETHODIMP OnFlush(DWORD);

};
