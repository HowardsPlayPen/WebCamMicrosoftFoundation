#include "Media.h"

Media::Media()
{
	InitializeCriticalSection(&criticalSection);
	referenceCount = 1;
	wSymbolicLink = NULL;
	cchSymbolicLink = 0;
	width = 0;
	height = 0;
	sourceReader = NULL;
	rawData = NULL;
	
}
Media::~Media()
{
	
	if (wSymbolicLink)
	{	
		delete wSymbolicLink;
		wSymbolicLink = NULL;
	}
	EnterCriticalSection(&criticalSection);

	if (sourceReader)
	{
		sourceReader->Release();
		sourceReader = NULL;
	}

	
	if (rawData)
	{
		delete rawData;
		rawData = NULL;
	}

	CoTaskMemFree(wSymbolicLink);
	wSymbolicLink = NULL;
	cchSymbolicLink = 0;

	LeaveCriticalSection(&criticalSection);
	DeleteCriticalSection(&criticalSection);
}

HRESULT Media::CreateCaptureDevice()
{
	HRESULT hr = S_OK;
	
	//this is important!!
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	UINT32 count = 0;
	IMFAttributes *attributes = NULL;
	IMFActivate **devices = NULL;

	if (FAILED(hr)) { CLEAN_ATTRIBUTES() }
	// Create an attribute store to specify enumeration parameters.
	hr = MFCreateAttributes(&attributes, 1);

	if (FAILED(hr)) { CLEAN_ATTRIBUTES() }

	//The attribute to be requested is devices that can capture video
	hr = attributes->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
	);
	if (FAILED(hr)) { CLEAN_ATTRIBUTES() }
	//Enummerate the video capture devices
	hr = MFEnumDeviceSources(attributes, &devices, &count);
	
	if (FAILED(hr)) { CLEAN_ATTRIBUTES() }
	//if there are any available devices
	if (count > 0)
	{
		/*If you actually need to select one of the available devices
		this is the place to do it. For this example the first device
		is selected
		*/
		//Get a source reader from the first available device
		hr = SetSourceReader(devices[0]);
		
		WCHAR *nameString = NULL;
		// Get the human-friendly name of the device
		UINT32 cchName;
		hr = devices[0]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
			&nameString, &cchName);

		if (SUCCEEDED(hr))
		{
			/// Note: See https://docs.microsoft.com/en-us/windows/win32/medfound/recommended-8-bit-yuv-formats-for-video-rendering
			/// for a good description of the formats. 
			/// On my computer the webcam support YUY2 which has a stride of 2 
			
			//allocate a byte buffer for the raw pixel data
			bytesPerPixel = abs(stride) / width;
			rawData = new BYTE[width*height * bytesPerPixel];
			wcscpy(deviceNameString,nameString);
		}
		CoTaskMemFree(nameString);
	}

	//clean
	CLEAN_ATTRIBUTES()
}


HRESULT Media::SetSourceReader(IMFActivate *device)
{
	HRESULT hr = S_OK;

	IMFMediaSource *source = NULL;
	IMFAttributes *attributes = NULL;
	IMFMediaType *mediaType = NULL;

	EnterCriticalSection(&criticalSection);

	// Note: I have found the following call to fail with the error: 		
	//       hr	0xc00d36b4 : The data specified for the media type is invalid, inconsistent, or not supported by this object.	HRESULT
	// So some vidcap devices are not happy with this combination.
	hr = device->ActivateObject(__uuidof(IMFMediaSource), (void**)&source);

	//get symbolic link for the device
	if(SUCCEEDED(hr))
		hr = device->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &wSymbolicLink, &cchSymbolicLink);
	//Allocate attributes
	if (SUCCEEDED(hr))
		hr = MFCreateAttributes(&attributes, 2);
	//get attributes
	if (SUCCEEDED(hr))
		hr = attributes->SetUINT32(MF_READWRITE_DISABLE_CONVERTERS, TRUE);
	// Set the callback pointer.
	if (SUCCEEDED(hr))
		hr = attributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK,this);
	//Create the source reader
	if (SUCCEEDED(hr))
		hr = MFCreateSourceReaderFromMediaSource(source,attributes,&sourceReader);
	// Try to find a suitable output type.
	if (SUCCEEDED(hr))
	{
		/// Update: This for loop was aimed at iterating across all supported (native) media types - but there were a couple of strange things which have been updated below
		for (DWORD i = 0; ; i++)
		{
			hr = sourceReader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,i,&mediaType);

			/// Note: FAILED() here does not stop on S_FALSE - which is correct, as it should continue in this loop but might not sound logical
			/// but if you loop through ALL supported media types the GetNativeMediaType will eventually return a code to indicate there are no more types available
			if (FAILED(hr)) { break; }
			
			// Note: I moved this call to here as elsewhere in this code it uses stride without checking if it was zero
			//Get the stride for this format so we can calculate the number of bytes per pixel
			GetDefaultStride(mediaType, &stride);

			hr = IsMediaTypeSupported(mediaType);
			if (FAILED(hr)) { break; } /// Note: this is not detecting S_FALSE
			//Get width and height
			MFGetAttributeSize(mediaType, MF_MT_FRAME_SIZE, &width, &height);
			if (mediaType) 
			{ mediaType->Release(); mediaType = NULL; }

			/// Note: this SUCCESS(hr) is WRONG - i.e. if 'hr' was S_FALSE (did not find media type) then this still goes ahead thinking it HAD found an output type
			if (S_OK == hr)// Found an output type - by explicityly checking for S_OK
				break;
		}
	}
	if (SUCCEEDED(hr))
	{
		// Ask for the first sample.
		hr = sourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,	0, NULL, NULL,NULL,NULL);
	}

	if (FAILED(hr))
	{
		if (source)
		{
			source->Shutdown();	
		}
		Close();
	}
	if (source) { source->Release(); source = NULL; }
	if (attributes) { attributes->Release(); attributes = NULL; }
	if (mediaType) { mediaType->Release(); mediaType = NULL; }

	LeaveCriticalSection(&criticalSection);
	return hr;
}

HRESULT Media::IsMediaTypeSupported(IMFMediaType *pType)
{
	HRESULT hr = S_OK;

	BOOL bFound = FALSE;
	GUID subtype = { 0 };

	if (FAILED(hr)) { return hr; }
	hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);

	videoFormat = subtype;

	if (FAILED(hr))	{return hr;	}

	/// Update: The below is still lying - eg NV12 or RGB32 are not supported as there is no NV12 to BGRA function in main.cpp
	if (subtype == MFVideoFormat_RGB32 || subtype == MFVideoFormat_RGB24 || subtype == MFVideoFormat_YUY2 || subtype == MFVideoFormat_NV12)
		return S_OK;
	else
		/// Update: Returning S_FALSE here is unclear as SUCCESS(S_FALSE) is actually true 
		/// See https://docs.microsoft.com/en-us/windows/win32/learnwin32/error-handling-in-com
		/// So either return S_FALSE and check for if(S_FALSE == hr) specifically, but not if you use SUCCESS() or FAIL() as was done in this wider code
		return S_FALSE; 
	
	return hr;
}

HRESULT Media::Close()
{
	EnterCriticalSection(&criticalSection);
	if(sourceReader)
	{sourceReader->Release(); sourceReader = NULL;}

	CoTaskMemFree(wSymbolicLink);
	wSymbolicLink = NULL;
	cchSymbolicLink = 0;

	LeaveCriticalSection(&criticalSection);
	return S_OK;
}

//From IUnknown 
STDMETHODIMP Media::QueryInterface(REFIID riid, void** ppvObject)
{
	static const QITAB qit[] = {QITABENT(Media, IMFSourceReaderCallback),{ 0 },};
	return QISearch(this, qit, riid, ppvObject);
}
//From IUnknown
ULONG Media::Release()
{
	ULONG count = InterlockedDecrement(&referenceCount);
	if (count == 0)
		delete this;
	// For thread safety
	return count;
}
//From IUnknown
ULONG Media::AddRef()
{
	return InterlockedIncrement(&referenceCount);
}


//Calculates the default stride based on the format and size of the frames
HRESULT Media::GetDefaultStride(IMFMediaType *type, LONG *stride)
{
	LONG tempStride = 0;

	// Try to get the default stride from the media type.
	HRESULT hr = type->GetUINT32(MF_MT_DEFAULT_STRIDE, (UINT32*)&tempStride);
	if (FAILED(hr))
	{
		//Setting this atribute to NULL we can obtain the default stride
		GUID subtype = GUID_NULL;

		UINT32 width = 0;
		UINT32 height = 0;

		// Obtain the subtype
		hr = type->GetGUID(MF_MT_SUBTYPE, &subtype);
		//obtain the width and height
		if (SUCCEEDED(hr))
			hr = MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height);
		//Calculate the stride based on the subtype and width
		if (SUCCEEDED(hr))
			hr = MFGetStrideForBitmapInfoHeader(subtype.Data1, width, &tempStride);
		// set the attribute so it can be read
		if (SUCCEEDED(hr))
			(void)type->SetUINT32(MF_MT_DEFAULT_STRIDE, UINT32(tempStride));
	}

	if (SUCCEEDED(hr))
			*stride = tempStride;
	return hr;
}

//Method from IMFSourceReaderCallback
HRESULT Media::OnReadSample(HRESULT status, DWORD streamIndex, DWORD streamFlags, LONGLONG timeStamp, IMFSample *sample)
{
	HRESULT hr = S_OK;
	IMFMediaBuffer *mediaBuffer = NULL;

	EnterCriticalSection(&criticalSection);

	if (FAILED(status))
		hr = status;

	if (SUCCEEDED(hr))
	{
		if (sample)
		{// Get the video frame buffer from the sample.
			hr = sample->GetBufferByIndex(0, &mediaBuffer);
			// Draw the frame.
			if (SUCCEEDED(hr))
			{
				BYTE* data;
				mediaBuffer->Lock(&data, NULL, NULL);
				//This is a good place to perform color conversion and drawing
				//Instead we're copying the data to a buffer
				CopyMemory(rawData, data, width*height * bytesPerPixel);

			}
		}
	}
	// Request the next frame.
	if (SUCCEEDED(hr))
		hr = sourceReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);

	if (FAILED(hr))
	{
		//Notify there was an error
		printf("Error HRESULT = 0x%d", hr);
		PostMessage(NULL, 1, (WPARAM)hr, 0L);
	}
	if (mediaBuffer) { mediaBuffer->Release(); mediaBuffer = NULL; }

	LeaveCriticalSection(&criticalSection);
	return hr;
}
//Method from IMFSourceReaderCallback 
STDMETHODIMP Media::OnEvent(DWORD, IMFMediaEvent *) { return S_OK; }
//Method from IMFSourceReaderCallback 
STDMETHODIMP Media::OnFlush(DWORD) { return S_OK; }