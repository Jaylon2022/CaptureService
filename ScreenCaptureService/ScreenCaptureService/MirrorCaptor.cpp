#include "stdafx.h"
#include "MirrorCaptor.h"
#include "display-esc.h"
#include <StrSafe.h>
LPTSTR driverName = _T("Mirage Driver");
BOOL MirrorCaptor::Init()
{
	if (!GetMirrorInfo()) {
		m_log.SR_printf("!GetMirrorInfo");
		return FALSE;
	}
	if (!ActivateMirror()) {
		return FALSE;
	}
	m_hDc = GetMirrorHdc();
	if (m_hDc != nullptr) {
		struct  GETCHANGESBUF buffer;
		int nret = ExtEscape(m_hDc, dmf_esc_usm_pipe_map, 0, nullptr, sizeof(GETCHANGESBUF), (LPSTR)&buffer);
		if (nret > 0) {
			m_surface = buffer.Userbuffer;
		}
	}

	CreateEncoder(m_curCx, m_curCy);
	return m_hDc != nullptr;
}

BOOL MirrorCaptor::GetMirrorInfo()
{
	INT devNum = 0;
	BOOL result;
	FillMemory(&m_dispDevice, sizeof(DISPLAY_DEVICE), 0);
	m_dispDevice.cb = sizeof(DISPLAY_DEVICE);
	while ((result = EnumDisplayDevices(NULL, devNum, &m_dispDevice, 0)) == TRUE) {
		if (_tcsicmp(&m_dispDevice.DeviceString[0], driverName) == 0) {
			break;
		}
		devNum++;
	}

	return result;
}

BOOL MirrorCaptor::ActivateMirror()
{
	DEVMODE devmode_primary;
	if (!GetPrimaryInfo(&devmode_primary)) {
		return FALSE;
	}

	DEVMODE devmode;
	FillMemory(&devmode, sizeof(DEVMODE), 0);
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;

	devmode.dmFields = DM_BITSPERPEL |
		DM_PELSWIDTH |
		DM_PELSHEIGHT |
		DM_POSITION;

	StringCbCopy((LPTSTR)&devmode.dmDeviceName[0], sizeof(devmode.dmDeviceName), _T("mirror"));
	LPTSTR deviceName = (LPTSTR)&m_dispDevice.DeviceName[0];

	devmode.dmPelsWidth = devmode_primary.dmPelsWidth;
	devmode.dmPelsHeight = devmode_primary.dmPelsHeight;
	devmode.dmBitsPerPel = devmode_primary.dmBitsPerPel;

	ChangeDisplaySettingsEx(deviceName, &devmode, NULL, (CDS_UPDATEREGISTRY | CDS_NORESET), NULL);
	ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);

	m_curCx = devmode_primary.dmPelsWidth;
	m_curCy = devmode_primary.dmPelsHeight;

	return TRUE;
}

BOOL MirrorCaptor::DeActivateMirror()
{
	DEVMODE devmode;
	FillMemory(&devmode, sizeof(DEVMODE), 0);
	devmode.dmSize = sizeof(DEVMODE);
	devmode.dmDriverExtra = 0;

	devmode.dmFields = DM_BITSPERPEL |
		DM_PELSWIDTH |
		DM_PELSHEIGHT |
		DM_POSITION;

	StringCbCopy((LPTSTR)&devmode.dmDeviceName[0], sizeof(devmode.dmDeviceName), _T("mirror"));
	LPTSTR deviceName = (LPTSTR)&m_dispDevice.DeviceName[0];

	devmode.dmPelsWidth = 0;
	devmode.dmPelsHeight = 0;

	ChangeDisplaySettingsEx(deviceName, &devmode, NULL, (CDS_UPDATEREGISTRY | CDS_NORESET), NULL);
	ChangeDisplaySettingsEx(NULL, NULL, NULL, 0, NULL);

	return TRUE;
}

void MirrorCaptor::Deinit()
{
	if (m_hDc != nullptr) {
		DeleteDC(m_hDc);
	}
	DeActivateMirror();
	DestroyEncoder();
}

BOOL MirrorCaptor::CaptureImage()
{
	if (m_surface != nullptr) {
		EncodeAndSend((BYTE *)m_surface);
	}

	return true;
}

HDC MirrorCaptor::GetMirrorHdc()
{
	m_log.SR_printfW(m_dispDevice.DeviceName);
	HDC hdc = CreateDC(
		m_dispDevice.DeviceName,
		nullptr,
		nullptr,
		nullptr);

	return hdc;
}

BOOL MirrorCaptor::GetPrimaryInfo(DEVMODE *devmode)
{
	FillMemory(devmode, sizeof(DEVMODE), 0);
	devmode->dmSize = sizeof(DEVMODE);
	devmode->dmDriverExtra = 0;

	DISPLAY_DEVICE dispDevice;
	FillMemory(&dispDevice, sizeof(DISPLAY_DEVICE), 0);
	dispDevice.cb = sizeof(DISPLAY_DEVICE);

	LPSTR deviceName = NULL;
	devmode->dmDeviceName[0] = '\0';

	INT devNum = 0;
	BOOL result;

	// First enumerate for Primary display device:
	while ((result = EnumDisplayDevices(NULL, devNum, &dispDevice, 0)) == TRUE) {
		if (dispDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
			// Primary device. Find out its dmPelsWidht and dmPelsHeight.
			EnumDisplaySettings(dispDevice.DeviceName,
				ENUM_CURRENT_SETTINGS,
				devmode);

			break;
		}
		devNum++;
	}

	return result;
}