#ifndef __DXGI_SCREEN_CAPTURE_H__
#define __DXGI_SCREEN_CAPTURE_H__

#include <d3d11.h>
#include <dxgi1_2.h>
#include <new>
#include "debug.h"
#include "ICapture.h"

typedef struct _FRAME_DATA
{
	IDXGISurface* Frame;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	_Field_size_bytes_((MoveCount * sizeof(DXGI_OUTDUPL_MOVE_RECT)) + (DirtyCount * sizeof(RECT))) BYTE* MetaData;
	UINT DirtyCount;
	UINT MoveCount;
	RECT rect_bbox;
} FRAME_DATA;

//
// Holds info about the pointer/cursor
//
typedef struct _PTR_INFO
{
	_Field_size_bytes_(BufferSize) BYTE* PtrShapeBuffer;
	DXGI_OUTDUPL_POINTER_SHAPE_INFO ShapeInfo;
	POINT Position;
	bool Visible;
	UINT BufferSize;
	UINT WhoUpdatedPositionLast;
	LARGE_INTEGER LastTimeStamp;
} PTR_INFO;


typedef struct QXLPointerShape{
	UINT                            Type;
	UINT                            Width;
	UINT                            Height;
	UINT                            Pitch;
	CONST VOID*                     pPixels;
	UINT                            XHot;
	UINT                            YHot;
	UINT                            x;
	UINT                            y;
}QXLPointerShape;

typedef struct QXLPointerPosition{
	UINT                            x;
	UINT                            y;
	UINT                            visible;
}QXLPointerPosition;

typedef enum SpiceCursorType {
	SPICE_CURSOR_TYPE_ALPHA,
	SPICE_CURSOR_TYPE_MONO,
	SPICE_CURSOR_TYPE_COLOR4,
	SPICE_CURSOR_TYPE_COLOR8,
	SPICE_CURSOR_TYPE_COLOR16,
	SPICE_CURSOR_TYPE_COLOR24,
	SPICE_CURSOR_TYPE_COLOR32,

	SPICE_CURSOR_TYPE_ENUM_END
} SpiceCursorType;


class VideoDXGICaptor:public ICapture
{
public:
	VideoDXGICaptor();
	~VideoDXGICaptor();

public:
	virtual BOOL Init();
	virtual void Deinit();
	virtual BOOL CaptureImage();

private:
	BOOL ResetDevice();
	BOOL  AttatchToThread(VOID);
	HRESULT  QueryFrame(_Out_ FRAME_DATA* Data, _Out_ bool* Timeout);
	IDXGISurface *GetStagingSurf(ID3D11Texture2D * surf, RECT *pSubRect);
	bool  GetMouse(_In_ DXGI_OUTDUPL_FRAME_INFO* FrameInfo);
	BOOL  SendFrame(FRAME_DATA* Data);
	bool DoneWithFrame();

private:
	BOOL                    m_bInit;
	int                     m_iWidth, m_iHeight;

	ID3D11Device           *m_hDevice;
	ID3D11DeviceContext    *m_hContext;

	IDXGIOutputDuplication *m_hDeskDupl;
	DXGI_OUTPUT_DESC        m_dxgiOutDesc;
	_Field_size_bytes_(m_MetaDataSize) BYTE* m_MetaDataBuffer;
	UINT m_MetaDataSize;
	ID3D11Texture2D*        m_hAcquiredDesktopImage;
	ID3D11Texture2D*        m_hNewDesktopImage;
	PTR_INFO  m_PtrInfo;
	UINT                    m_OutputNumber;
	BYTE                    *m_pBuff; 
};

#endif