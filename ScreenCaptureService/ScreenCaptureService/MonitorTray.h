#pragma once
#include <shlwapi.h>
#include <Shellapi.h>

#define WM_NOTIFYICON			WM_USER+100

class CMonitorTray
{
public:
	CMonitorTray(void);
	~CMonitorTray(void);
	BOOL Init( HINSTANCE hInst, HWND hDlg, const TCHAR* szTip, const DWORD dwMenuId, const DWORD dwSubIndex, const HICON hIcon );//初始化托盘
	BOOL Release();
	void OnNotifyIcon( WPARAM wParam, LPARAM lParam );
	void SetIcon( HICON hIcon );//设置图标
	HICON GetIcon( );//获得图标
	HWND GetDlg();
	HMENU GetMenu();//获得菜单
	int GetSubMenu();
	BOOL m_bInit;//判断是否创建托盘成功
private:
	//BOOL m_bInit;//判断是否创建托盘成功
	HWND m_hDlg;
	HMENU m_hMenu;
	NOTIFYICONDATA m_Nid;
	int m_nSubMenu;
};
