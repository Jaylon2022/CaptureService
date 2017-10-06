#include "stdafx.h"
#include "DXGICaptor.h"
#include <stdio.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define RESET_OBJECT(obj) { obj->Release(); obj = NULL; }


VideoDXGICaptor::VideoDXGICaptor()
	:m_hAcquiredDesktopImage(nullptr),
	m_hNewDesktopImage(nullptr),
	m_MetaDataBuffer(nullptr),
	m_MetaDataSize(0),
	m_bInit(FALSE),
	m_hDevice(NULL),
	m_hDeskDupl(NULL),
	m_hContext(NULL),
	m_OutputNumber(0)
{
	ZeroMemory(&m_dxgiOutDesc, sizeof(m_dxgiOutDesc));
	ZeroMemory(&m_PtrInfo, sizeof(PTR_INFO));
}
VideoDXGICaptor::~VideoDXGICaptor()
{
	Deinit();
}
BOOL VideoDXGICaptor::Init()
{
	HRESULT hr = S_OK;

	if (m_bInit){
		return FALSE;
	}
	AttatchToThread();/* 同当前桌面线程关联 */

	// Driver types supported
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

	// Feature levels supported
	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

	D3D_FEATURE_LEVEL FeatureLevel;

	//
	// Create D3D device
	//
	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex){
		hr = D3D11CreateDevice(NULL, DriverTypes[DriverTypeIndex], NULL, 0, FeatureLevels, NumFeatureLevels, D3D11_SDK_VERSION, &m_hDevice, &FeatureLevel, &m_hContext);
		if (SUCCEEDED(hr)){
			break;
		}
	}
	if (FAILED(hr)){
		m_log.SR_printf("D3D11CreateDevice error hr=%d", hr);
		return FALSE;
	}

	//
	// Get DXGI device
	//
	IDXGIDevice *hDxgiDevice = NULL;
	hr = m_hDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&hDxgiDevice));
	if (FAILED(hr)){
		m_log.SR_printf("QueryInterface IDXGIDevice error hr=%d", hr);
		return FALSE;
	}

	//
	// Get DXGI adapter
	//
	IDXGIAdapter *hDxgiAdapter = NULL;
	hr = hDxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&hDxgiAdapter));
	RESET_OBJECT(hDxgiDevice);
	if (FAILED(hr)){
		m_log.SR_printf("hDxgiDevice->GetParent error hr=%d", hr);
		return FALSE;
	}

	//
	// Get output
	//
	//INT nOutput = 0;
	m_OutputNumber = 0;
	IDXGIOutput *hDxgiOutput = NULL;
	hr = hDxgiAdapter->EnumOutputs(m_OutputNumber, &hDxgiOutput);/* Get the first output data */
	RESET_OBJECT(hDxgiAdapter);
	if (FAILED(hr)){
		m_log.SR_printf("hDxgiAdapter->EnumOutputs error hr=%d", hr);
		return FALSE;
	}

	//
	// get output description struct
	//
	hDxgiOutput->GetDesc(&m_dxgiOutDesc);
	
	//
	// QI for Output 1
	//
	IDXGIOutput1 *hDxgiOutput1 = NULL;
	hr = hDxgiOutput->QueryInterface(__uuidof(hDxgiOutput1), reinterpret_cast<void**>(&hDxgiOutput1));
	RESET_OBJECT(hDxgiOutput);
	if (FAILED(hr)){
		m_log.SR_printf("hDxgiOutput->QueryInterface error hr=%d", hr);
		return FALSE;
	}

	//
	// Create desktop duplication
	//
	hr = hDxgiOutput1->DuplicateOutput(m_hDevice, &m_hDeskDupl);
	RESET_OBJECT(hDxgiOutput1);
	if (FAILED(hr)){
		m_log.SR_printf("hDxgiOutput1->DuplicateOutput error hr=%d", hr);
		return FALSE;
	}

	// 初始化成功
	m_bInit = TRUE;
	CreateEncoder(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	m_pBuff = (BYTE *)malloc(GetSystemMetrics(SM_CXSCREEN)*GetSystemMetrics(SM_CYSCREEN) * 4);
	return TRUE;
}
VOID VideoDXGICaptor::Deinit()
{
	if (!m_bInit){
		return;
	}
	m_bInit = FALSE;

	if (m_hDeskDupl){
		m_hDeskDupl->Release();
		m_hDeskDupl = NULL;
	}

	if (m_hDevice){
		m_hDevice->Release();
		m_hDevice = NULL;
	}

	if (m_hContext){
		m_hContext->Release();
		m_hContext = NULL;
	}

#if 0
	if (m_hQxl != INVALID_HANDLE_VALUE){
		CloseHandle(m_hQxl);
		m_hQxl = INVALID_HANDLE_VALUE;
	}
#endif

	if (m_hAcquiredDesktopImage){
		RESET_OBJECT(m_hAcquiredDesktopImage);
	}

	if (m_hNewDesktopImage){
		RESET_OBJECT(m_hNewDesktopImage);
	}

	if (m_MetaDataBuffer){
		delete[] m_MetaDataBuffer;
		m_MetaDataBuffer = nullptr;
		m_MetaDataSize = 0;
	}
	DestroyEncoder();
	free(m_pBuff);
}
BOOL VideoDXGICaptor::AttatchToThread(VOID)
{
	HDESK hCurrentDesktop = OpenInputDesktop(DF_ALLOWOTHERACCOUNTHOOK, FALSE, MAXIMUM_ALLOWED);
	if (!hCurrentDesktop){
		m_log.SR_printf("OpenInputDesktop failed error:%d", GetLastError());
		return FALSE;
	}

	// Attach desktop to this thread
	BOOL bDesktopAttached = SetThreadDesktop(hCurrentDesktop);
	CloseDesktop(hCurrentDesktop);
	hCurrentDesktop = NULL;

	return bDesktopAttached;
}

//#define FPS_DEBUG  

#ifdef FPS_DEBUG  
DWORD g_fps = 0;
DWORD g_dwCurrentTime = 0;
#endif

BOOL VideoDXGICaptor::CaptureImage()
{
	FRAME_DATA data;
	memset(&data, 0, sizeof(FRAME_DATA));
	bool timeOut = false;
	HRESULT hr = QueryFrame(&data, &timeOut); 
	if (hr == DXGI_ERROR_ACCESS_LOST){
		m_log.SR_printf("hr == DXGI_ERROR_ACCESS_LOST need reset");
		return ResetDevice();
	}
	else if (hr == S_OK){
		if (timeOut){
			//OutputDebugStringA("Time out\n");
		}
		else{
#ifdef FPS_DEBUG  
			g_fps++;
			DWORD dwNow = GetTickCount();
			if (dwNow - g_dwCurrentTime > 1000){
				CSRLog::SR_dbgPrintf("fps=%d\n", g_fps);
				g_fps = 0;
				g_dwCurrentTime = dwNow;
			}
#endif
			GetMouse(&data.FrameInfo);
			DoneWithFrame();
			return SendFrame(&data);
		}
		return TRUE;
	}
	return false;
}

BOOL VideoDXGICaptor::ResetDevice()
{
	Deinit();
	return Init();
}

HRESULT VideoDXGICaptor::QueryFrame(_Out_ FRAME_DATA* Data, _Out_ bool* Timeout)
{
	if (!m_bInit){
		m_log.SR_printf("Not initialized QueryFrame will error.");
		return E_FAIL;
	}

	IDXGIResource *hDesktopResource = NULL;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	HRESULT hr = m_hDeskDupl->AcquireNextFrame(500, &FrameInfo, &hDesktopResource);
	if (hr == DXGI_ERROR_ACCESS_LOST){
		return DXGI_ERROR_ACCESS_LOST;
	}
	if (hr == DXGI_ERROR_WAIT_TIMEOUT){
		*Timeout = true;
		return S_OK;
	}
	else if (FAILED(hr)){
		m_log.SR_printf("AcquireNextFrame error hr=%d", hr);
		return E_FAIL;
	}

	// If still holding old frame, destroy it
	if (m_hAcquiredDesktopImage){
		RESET_OBJECT(m_hAcquiredDesktopImage);
	}
	//
	// query next frame staging buffer
	//
	hr = hDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&m_hAcquiredDesktopImage));
	RESET_OBJECT(hDesktopResource);
	if (FAILED(hr)){
		m_log.SR_printf("hDesktopResource->QueryInterface  ID3D11Texture2D error hr=%d", hr);
		return  E_FAIL;
	}

	if (FrameInfo.TotalMetadataBufferSize){
		// Old buffer too small
		if (FrameInfo.TotalMetadataBufferSize > m_MetaDataSize){
			if (m_MetaDataBuffer){
				delete[] m_MetaDataBuffer;
				m_MetaDataBuffer = nullptr;
			}
			m_MetaDataBuffer = new (std::nothrow) BYTE[FrameInfo.TotalMetadataBufferSize];
			if (!m_MetaDataBuffer){
				m_MetaDataSize = 0;
				Data->MoveCount = 0;
				Data->DirtyCount = 0;
				m_log.SR_printf("Failed to allocate memory for metadata");
				return  E_FAIL;
			}
			m_MetaDataSize = FrameInfo.TotalMetadataBufferSize;
		}

		UINT BufSize = FrameInfo.TotalMetadataBufferSize;

		// Get move rectangles
		hr = m_hDeskDupl->GetFrameMoveRects(BufSize, reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(m_MetaDataBuffer), &BufSize);
		if (FAILED(hr)){
			Data->MoveCount = 0;
			Data->DirtyCount = 0;
			m_log.SR_printf("Failed to get frame move rects");
			return E_FAIL;
		}
		Data->MoveCount = BufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);

		BYTE* DirtyRects = m_MetaDataBuffer + BufSize;
		BufSize = FrameInfo.TotalMetadataBufferSize - BufSize;

		// Get dirty rectangles
		hr = m_hDeskDupl->GetFrameDirtyRects(BufSize, reinterpret_cast<RECT*>(DirtyRects), &BufSize);
		if (FAILED(hr)){
			Data->MoveCount = 0;
			Data->DirtyCount = 0;
			m_log.SR_printf("Failed to get frame dirty rects");
			return  E_FAIL;
		}
		Data->DirtyCount = BufSize / sizeof(RECT);
		Data->MetaData = m_MetaDataBuffer;
	}

	if (Data->DirtyCount == 0){/* 没有图片区域发送 */
		Data->Frame = nullptr; 
	}
	else{
		RECT *dirty = reinterpret_cast<RECT*>(Data->MetaData + (Data->MoveCount * sizeof(DXGI_OUTDUPL_MOVE_RECT)));
		memset(&Data->rect_bbox, 0, sizeof(RECT));
		for (UINT i = 0; i < Data->DirtyCount; i++){
			UnionRect(&Data->rect_bbox, &Data->rect_bbox, &dirty[i]);
		}
		Data->Frame = GetStagingSurf(m_hAcquiredDesktopImage, &Data->rect_bbox);
	}
	Data->FrameInfo = FrameInfo;
	return S_OK;
}

bool VideoDXGICaptor::DoneWithFrame()
{
	HRESULT hr = m_hDeskDupl->ReleaseFrame();
	if (FAILED(hr)){
		return false;
	}

	if (m_hAcquiredDesktopImage){
		RESET_OBJECT(m_hAcquiredDesktopImage);
	}

	return true;
}


IDXGISurface  *VideoDXGICaptor::GetStagingSurf(ID3D11Texture2D * surf, RECT *pSubRect)
{
	D3D11_TEXTURE2D_DESC frameDescriptor;
	HRESULT hr; 
	surf->GetDesc(&frameDescriptor);
	if (m_hNewDesktopImage == nullptr){
		frameDescriptor.Usage = D3D11_USAGE_STAGING;
		frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		frameDescriptor.BindFlags = 0;
		frameDescriptor.MiscFlags = 0;
		frameDescriptor.MipLevels = 1;
		frameDescriptor.ArraySize = 1;
		frameDescriptor.SampleDesc.Count = 1;
		hr = m_hDevice->CreateTexture2D(&frameDescriptor, NULL, &m_hNewDesktopImage);
		if (FAILED(hr)){
			m_log.SR_printf("m_hDevice->CreateTexture2D failed1 hr=%d", hr);
			return nullptr;
		}
	}
	else{
		D3D11_TEXTURE2D_DESC fd;
		m_hNewDesktopImage->GetDesc(&fd);
		if (fd.Height != frameDescriptor.Height
			|| fd.Width != frameDescriptor.Width){/* 分辨率发生变化等情况 */
			RESET_OBJECT(m_hNewDesktopImage);
			frameDescriptor.Usage = D3D11_USAGE_STAGING;
			frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
			frameDescriptor.BindFlags = 0;
			frameDescriptor.MiscFlags = 0;
			frameDescriptor.MipLevels = 1;
			frameDescriptor.ArraySize = 1;
			frameDescriptor.SampleDesc.Count = 1;
			hr = m_hDevice->CreateTexture2D(&frameDescriptor, NULL, &m_hNewDesktopImage);
			if (FAILED(hr)){
				m_log.SR_printf("m_hDevice->CreateTexture2D failed2 hr=%d", hr);
				return nullptr;
			}
		}
	}

	m_hContext->CopyResource(m_hNewDesktopImage, surf);
	IDXGISurface *hStagingSurf = NULL;
	hr = m_hNewDesktopImage->QueryInterface(__uuidof(IDXGISurface), (void **)(&hStagingSurf));
	if (FAILED(hr)){
		m_log.SR_printf("m_hNewDesktopImage->QueryInterface IDXGISurface failed hr=%d", hr);
		return nullptr;
	}
	
	return hStagingSurf;
}


//CSRLog g_log;
BOOL  VideoDXGICaptor::SendFrame(FRAME_DATA* frameData)
{
	if (frameData->Frame == nullptr){
		return true;
	}

	//
	// copy bits to user space
	//
	DXGI_MAPPED_RECT mappedRect;
	HRESULT hr = frameData->Frame->Map(&mappedRect, DXGI_MAP_READ);
	DXGI_SURFACE_DESC dxgi_des;
	frameData->Frame->GetDesc(&dxgi_des);

	if (SUCCEEDED(hr)){
		if (frameData->DirtyCount > 0 || frameData->MoveCount > 0) {
			int width = GetSystemMetrics(SM_CXSCREEN);
			int height = GetSystemMetrics(SM_CYSCREEN);
			if (mappedRect.Pitch != width * 4)
			{
				//BYTE *pBuff = (BYTE *)malloc(width * height * 4);
				for (int i = 0; i < height; i++) {
					memcpy(m_pBuff + i*(width * 4), mappedRect.pBits + i*mappedRect.Pitch, width * 4);
				}
				EncodeAndSend(m_pBuff);
			}
			else {
				EncodeAndSend(mappedRect.pBits);
			}
		}

#if 0
		DWORD srcSize = mappedRect.Pitch * dxgi_des.Height;
		QXLBitblt data;
		memset(&data, 0, sizeof(QXLBitblt));
		data.SrcAddr = mappedRect.pBits;
		data.bufferLen = srcSize;
		data.DirtyCount = frameData->DirtyCount;
		data.MoveCount = frameData->MoveCount;
		data.MetaData = frameData->MetaData;

		/* 获取当前分辨率 */
		data.Width = GetSystemMetrics(SM_CXSCREEN);
		data.Height = GetSystemMetrics(SM_CYSCREEN);
		data.Pitch = mappedRect.Pitch;// mappedRect.Pitch / dxgi_des.Width * screenX;

		if (data.MoveCount > 0 || data.DirtyCount > 0){
			data.bbox = frameData->rect_bbox;
			DeviceIoControl(m_hQxl, IOCTL_BITBLT, &data, sizeof(QXLBitblt), NULL, 0, NULL, NULL);
		}
#endif
		frameData->Frame->Unmap();
	}

	RESET_OBJECT(frameData->Frame);
	return SUCCEEDED(hr);
}



bool VideoDXGICaptor::GetMouse(_In_ DXGI_OUTDUPL_FRAME_INFO* FrameInfo)
{
	// A non-zero mouse update timestamp indicates that there is a mouse position update and optionally a shape change
	if (FrameInfo->LastMouseUpdateTime.QuadPart == 0){
		if (GetAsyncKeyState(VK_LBUTTON)){/* 当鼠标左键按下后，duplicate desktop api是无法准确获取到鼠标光标位置的，需要自己手动去获取 */
			POINT point;
			//GetCursorPos(&point);
			GetPhysicalCursorPos(&point);
			if (point.x != m_PtrInfo.Position.x
				|| point.y != m_PtrInfo.Position.y){
				QXLPointerPosition position;
				position.x = point.x;
				position.y = point.y;
				position.visible = TRUE;
#if 0
				DeviceIoControl(m_hQxl, IOCTL_SETCUSORPOS, &position, sizeof(QXLPointerPosition), NULL, 0, NULL, NULL);
#endif
				m_PtrInfo.Position.x = point.x;
				m_PtrInfo.Position.y = point.y;
			}
		}
		return true;
	}

	bool UpdatePosition = true;

	// Make sure we don't update pointer position wrongly
	// If pointer is invisible, make sure we did not get an update from another output that the last time that said pointer
	// was visible, if so, don't set it to invisible or update.
	if (!FrameInfo->PointerPosition.Visible && (m_PtrInfo.WhoUpdatedPositionLast != m_OutputNumber)){
		UpdatePosition = false;
	}

	// If two outputs both say they have a visible, only update if new update has newer timestamp
	if (FrameInfo->PointerPosition.Visible && m_PtrInfo.Visible && (m_PtrInfo.WhoUpdatedPositionLast != m_OutputNumber)
		&& (m_PtrInfo.LastTimeStamp.QuadPart > FrameInfo->LastMouseUpdateTime.QuadPart)){

		UpdatePosition = false;
	}

	// Update position
	if (UpdatePosition){
		m_PtrInfo.Position.x = FrameInfo->PointerPosition.Position.x;// +m_OutputDesc.DesktopCoordinates.left - OffsetX;
		m_PtrInfo.Position.y = FrameInfo->PointerPosition.Position.y;// +m_OutputDesc.DesktopCoordinates.top - OffsetY;
		m_PtrInfo.WhoUpdatedPositionLast = m_OutputNumber;
		m_PtrInfo.LastTimeStamp = FrameInfo->LastMouseUpdateTime;
		m_PtrInfo.Visible = FrameInfo->PointerPosition.Visible != 0;
		QXLPointerPosition position;
		position.x = m_PtrInfo.Position.x;// +m_PtrInfo.ShapeInfo.HotSpot.x;
		position.y = m_PtrInfo.Position.y;// +m_PtrInfo.ShapeInfo.HotSpot.y;
		position.visible = m_PtrInfo.Visible;

		if (m_PtrInfo.Position.x != 0
			&& m_PtrInfo.Position.y != 0){
#if 0
			DeviceIoControl(m_hQxl, IOCTL_SETCUSORPOS, &position, sizeof(QXLPointerPosition), NULL, 0, NULL, NULL);
#endif
		}
	}

	if (!FrameInfo->PointerPosition.Visible){/* 鼠标隐藏 */
		QXLPointerPosition position;
		position.visible = 0;

#if 0
		DeviceIoControl(m_hQxl, IOCTL_SETCUSORPOS, &position, sizeof(QXLPointerPosition), NULL, 0, NULL, NULL);
#endif
	}
	// No new shape
	if (FrameInfo->PointerShapeBufferSize == 0){
		return true;
	}

	// Old buffer too small
	if (FrameInfo->PointerShapeBufferSize > m_PtrInfo.BufferSize){
		if (m_PtrInfo.PtrShapeBuffer){
			delete[] m_PtrInfo.PtrShapeBuffer;
			m_PtrInfo.PtrShapeBuffer = nullptr;
		}
		m_PtrInfo.PtrShapeBuffer = new (std::nothrow) BYTE[FrameInfo->PointerShapeBufferSize];
		if (!m_PtrInfo.PtrShapeBuffer){
			m_log.SR_printf("Failed to allocate memory for pointer shape");
			m_PtrInfo.BufferSize = 0;
			return false;
		}

		// Update buffer size
		m_PtrInfo.BufferSize = FrameInfo->PointerShapeBufferSize;
	}

	// Get shape
	UINT BufferSizeRequired;
	HRESULT hr = m_hDeskDupl->GetFramePointerShape(FrameInfo->PointerShapeBufferSize, reinterpret_cast<VOID*>(m_PtrInfo.PtrShapeBuffer), &BufferSizeRequired, &(m_PtrInfo.ShapeInfo));
	if (FAILED(hr)){
		delete[] m_PtrInfo.PtrShapeBuffer;
		m_PtrInfo.PtrShapeBuffer = nullptr;
		m_PtrInfo.BufferSize = 0;
		m_log.SR_printf("Failed to get frame pointer shape");
		return false;
	}


	QXLPointerShape shape;
	memset(&shape, 0, sizeof(QXLPointerShape));
	if (m_PtrInfo.ShapeInfo.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME){
		m_PtrInfo.ShapeInfo.Height = m_PtrInfo.ShapeInfo.Height / 2;
		shape.Type = SPICE_CURSOR_TYPE_MONO; 
	}
	else if (m_PtrInfo.ShapeInfo.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR){
		shape.Type = SPICE_CURSOR_TYPE_ALPHA;
	}
	else{
		return true;
	}
	shape.Width = m_PtrInfo.ShapeInfo.Width;
	shape.Height = m_PtrInfo.ShapeInfo.Height;
	shape.Pitch = m_PtrInfo.ShapeInfo.Pitch;
	shape.XHot = 0;// m_PtrInfo.ShapeInfo.HotSpot.x;
	shape.YHot = 0;// m_PtrInfo.ShapeInfo.HotSpot.y;
	shape.pPixels = m_PtrInfo.PtrShapeBuffer;
	shape.x = FrameInfo->PointerPosition.Position.x;// +m_PtrInfo.ShapeInfo.HotSpot.x;
	shape.y = FrameInfo->PointerPosition.Position.y;// +m_PtrInfo.ShapeInfo.HotSpot.y;

#if 0
	DeviceIoControl(m_hQxl, IOCTL_SETCUSORSHAPE, &shape, sizeof(QXLPointerShape), NULL, 0, NULL, NULL);
#endif

	return true;
}
