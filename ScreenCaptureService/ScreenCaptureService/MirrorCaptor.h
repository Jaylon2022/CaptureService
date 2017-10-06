#pragma once
#include "ICapture.h"
#include <tchar.h>

class MirrorCaptor :public ICapture
{
public:
	virtual BOOL Init();
	virtual void Deinit();
	virtual BOOL CaptureImage();

private:
	BOOL ActivateMirror();
	BOOL DeActivateMirror();
	HDC  GetMirrorHdc();

	BOOL GetPrimaryInfo(DEVMODE *devmode);
	BOOL GetMirrorInfo();

private:
	HDC m_hDc;
	int m_curCx;
	int m_curCy;
	int m_curBpp;
	BOOL m_bInited;
	DISPLAY_DEVICE m_dispDevice;
	PVOID m_surface;
};