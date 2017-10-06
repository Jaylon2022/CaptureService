// ScreenCaptureService.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "ScreenCaptureService.h"
#include "DXGICaptor.h"
#include "MirrorCaptor.h"
#include "protocol.h"
#include "TinyClient.h"
#include "resource.h"

typedef LONG(NTAPI* fnRtlGetVersion)(PRTL_OSVERSIONINFOW lpVersionInformation);

/* 以下两个函数是在系统的VersionHelpers.h定义，xp下不能编译，所以将两个函数拷贝过来 */
BOOL
IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor)
{

	RTL_OSVERSIONINFOEXW verInfo = { 0 };
	verInfo.dwOSVersionInfoSize = sizeof(verInfo);

	static auto RtlGetVersion = (fnRtlGetVersion)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "RtlGetVersion");

	if (RtlGetVersion != 0 && RtlGetVersion((PRTL_OSVERSIONINFOW)&verInfo) == 0)
	{
		if (verInfo.dwMajorVersion > wMajorVersion)
			return true;
		else if (verInfo.dwMajorVersion < wMajorVersion)
			return false;

		if (verInfo.dwMinorVersion > wMinorVersion)
			return true;
		else if (verInfo.dwMinorVersion < wMinorVersion)
			return false;

		if (verInfo.wServicePackMajor >= wServicePackMajor)
			return true;
	}

	return false;
}

BOOL
IsWindows8OrGreater()
{
#ifndef _WIN32_WINNT_WIN8
#define _WIN32_WINNT_WIN8                   0x0602
#endif
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8), 0);
}



volatile bool g_stop = false;
LRESULT CALLBACK CaptureThread(LPVOID lp)
{
	CTinyClient *pClient = (CTinyClient *)lp;

	ICapture *pScreen = nullptr;
	if (IsWindows8OrGreater()) {
		pScreen = new VideoDXGICaptor();
	}
	else {
		pScreen = new MirrorCaptor();
	}
	if (pScreen == nullptr) {
		MessageBox(nullptr, _T("创建对象失败"), 0, 0);
		return -1;
	}

	pScreen->SetClient(pClient);
	pScreen->Init();
	while (!g_stop){
		pScreen->CaptureImage();
		Sleep(10);
	} 

	pScreen->Deinit();
	delete pScreen;

	return TRUE;
}

void Flush() {
	HWND   hwndParent = ::FindWindow(L"Progman", L"Program Manager");
	HWND   hwndSHELLDLL_DefView = ::FindWindowEx(hwndParent, NULL, L"SHELLDLL_DefView", NULL);
	HWND   hwndSysListView32 = ::FindWindowEx(hwndSHELLDLL_DefView, NULL, L"SysListView32", L"FolderView");
	::PostMessage(hwndSysListView32, WM_KEYDOWN, VK_F5, 0);
}



BOOL CALLBACK DialogProc(HWND hWnd, UINT uMessage, WPARAM wp, LPARAM lp)
{
	static CTinyClient s_client;
	static HANDLE s_hThread = nullptr;
	switch (uMessage) {
	case WM_INITDIALOG:
		EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_STOP1), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_START1), TRUE);
	break;

	case WM_COMMAND:
	{
		switch (wp)
		{
		case IDC_BUTTON_START1:
			char szIp[64];
			GetDlgItemTextA(hWnd, IDC_EDIT_IP, szIp, sizeof(szIp));
			s_client.Init();
			if (!s_client.Connect(szIp, 6666)) {
				MessageBox(nullptr, _T("网络通信失败!"), 0, 0);
				break;
			}

			g_stop = false;
			s_hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)CaptureThread, &s_client, 0, nullptr);
			Sleep(100);
			DWORD dwCode;
			GetExitCodeThread(s_hThread, &dwCode);
			if (STILL_ACTIVE == dwCode) {
				EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_STOP1), TRUE);
				EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_START1), FALSE);
			}
			Flush();
			break;
		case IDC_BUTTON_STOP1:
		{
			DWORD dwCode;
			GetExitCodeThread(s_hThread, &dwCode);
			if (STILL_ACTIVE == dwCode) {
				g_stop = true;
				WaitForSingleObject(s_hThread, INFINITE);
			}
			s_client.Disconnect();
			EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_STOP1), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_START1), TRUE);
		}
		break;
		default:
			break;
		}
		break;
	}

	case WM_CLOSE:
		DWORD dwCode;
		GetExitCodeThread(s_hThread, &dwCode);
		if (STILL_ACTIVE == dwCode) {
			g_stop = true;
			WaitForSingleObject(s_hThread, INFINITE);
		}
		s_client.Disconnect();
		EndDialog(hWnd, 0);
		break;
	}
	return 0;
}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG_MAIN), nullptr, DialogProc);
}

