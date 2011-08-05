#include <streams.h>

#include "PushSource.h"
#include "PushGuids.h"
#include "DibHelper.h"


/**********************************************
 *
 *  CPushPinDesktop Class
 *  
 *
 **********************************************/
#define MIN(a,b)  ((a) < (b) ? (a) : (b))  
DWORD start;
CPushPinDesktop::CPushPinDesktop(HRESULT *phr, CSource *pFilter)
        : CSourceStream(NAME("Push Source Desktop OS"), phr, pFilter, L"Out"),
        m_FramesWritten(0),
        m_bZeroMemory(0),
        m_iFrameNumber(0),
        m_rtFrameLength(FPS_50), // Capture and display desktop "x" times per second...kind of...
        m_nCurrentBitDepth(32)
{
	// The main point of this sample is to demonstrate how to take a DIB
	// in host memory and insert it into a video stream. 

	// To keep this sample as simple as possible, we just read the desktop image
	// from a file and copy it into every frame that we send downstream.
    //
	// In the filter graph, we connect this filter to the AVI Mux, which creates 
    // the AVI file with the video frames we pass to it. In this case, 
    // the end result is a screen capture video (GDI images only, with no
    // support for overlay surfaces).

    // Get the device context of the main display, just to get some metricts for it...
	start = GetTickCount();
    HDC hDC;
    hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);

    // Get the dimensions of the main desktop window
    m_rScreen.left   = m_rScreen.top = 0;
    m_rScreen.right  = GetDeviceCaps(hDC, HORZRES);
    m_rScreen.bottom = GetDeviceCaps(hDC, VERTRES);


	// my custom config settings...

	// assume 0 means not set...negative ignore :)
	DWORD config_start_x = read_config_setting(TEXT("start_x"));
	if(config_start_x > 0) {
	  m_rScreen.left = config_start_x; // TODO no overflow, that's a bad value too...
	}

	// is there a better way to do this?
	DWORD config_start_y = read_config_setting(TEXT("start_y"));
	if(config_start_y > 0) {
	  m_rScreen.top = config_start_y;
	}

	DWORD config_width = read_config_setting(TEXT("width"));
	if(config_width > 0) {
		DWORD desired = m_rScreen.left + config_width;
		DWORD max_possible = m_rScreen.right;
		if(desired < max_possible)
			m_rScreen.right = desired;
		else
			m_rScreen.right = max_possible; // or should I throw an error?
	}

	DWORD config_height = read_config_setting(TEXT("height"));
	if(config_height > 0) {
		DWORD desired = m_rScreen.top + config_height;
		DWORD max_possible = m_rScreen.bottom;
		if(desired < max_possible)
			m_rScreen.bottom = desired;
		else
			m_rScreen.bottom = max_possible;
	}

	LocalOutput("got2 %d %d %d %d -> %d %d %d %d\n", config_start_x, config_start_y, config_height, config_width, m_rScreen.top, m_rScreen.bottom, m_rScreen.left, m_rScreen.right);
    // Save dimensions for later use in FillBuffer()
    m_iImageWidth  = m_rScreen.right  - m_rScreen.left;
    m_iImageHeight = m_rScreen.bottom - m_rScreen.top;

    // Release the device context
    DeleteDC(hDC);
}


CPushPinDesktop::~CPushPinDesktop()
{   
	// I don't think it ever gets here... somebody doesn't call it anyway :)
    DbgLog((LOG_TRACE, 3, TEXT("Frames written %d"), m_iFrameNumber));
}


// This is where we insert the DIB bits into the video stream.
// FillBuffer is called once for every sample in the stream.
HRESULT CPushPinDesktop::FillBuffer(IMediaSample *pSample)
{
	DWORD local_start = GetTickCount();
	BYTE *pData;
    long cbData;

    CheckPointer(pSample, E_POINTER);

    CAutoLock cAutoLockShared(&m_cSharedState);

    // Access the sample's data buffer
    pSample->GetPointer(&pData);
    cbData = pSample->GetSize();

    // Check that we're still using video
    ASSERT(m_mt.formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;

	// Copy the DIB bits over into our filter's output buffer.
    // Since sample size may be larger than the image size, bound the copy size.
    int nSize = min(pVih->bmiHeader.biSizeImage, (DWORD) cbData);
    HDIB hDib = CopyScreenToBitmap(&m_rScreen, pData, (BITMAPINFO *) &(pVih->bmiHeader));

    if (hDib)
        DeleteObject(hDib);
	/*
	  CRefTime now;
    m_pParent->StreamTime(now);
    pSample->SetTime(&rtStart, &rtStop);
    m_iFrameNumber++;*/

/*  
    REFERENCE_TIME endThisFrame = now + avgFrameTime;
    pms->SetTime((REFERENCE_TIME *) &now, &endThisFrame); TODO */
	// Set TRUE on every sample for uncompressed frames
    //pms->SetSyncPoint(TRUE);
	//pms->SetDis(TRUE);

	double fps = ((double) m_iFrameNumber)/(GetTickCount() - start)*1000;
	LocalOutput("end total frames %d %dms, total %dms %f fps\n", m_iFrameNumber, GetTickCount() - local_start, GetTickCount() - local_start, fps);

    return S_OK;
}
