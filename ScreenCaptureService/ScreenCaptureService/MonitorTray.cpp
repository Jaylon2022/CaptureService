#include "stdAfx.h"
#include "MonitorTray.h"

CMonitorTray::CMonitorTray(void)
{
	m_bInit = FALSE;
	m_nSubMenu = 0;
}

CMonitorTray::~CMonitorTray(void)
{
}

BOOL CMonitorTray::Init( HINSTANCE hInst, HWND hDlg, const TCHAR* szTip, const DWORD dwMenuId, const DWORD dwSubIndex, const HICON hIcon )
{
	if ( !m_bInit )
	{
		m_Nid.cbSize = sizeof( NOTIFYICONDATA );
		m_Nid.hWnd = hDlg;
		m_Nid.uID = 100;
		m_Nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		m_Nid.uCallbackMessage = WM_NOTIFYICON;
		m_Nid.hIcon = hIcon;
		_tcscpy_s( m_Nid.szTip, _countof(m_Nid.szTip), szTip );
		m_hMenu = ::LoadMenu( hInst, MAKEINTRESOURCE(dwMenuId) );
		m_hDlg = hDlg;
		Shell_NotifyIcon(NIM_ADD, &m_Nid);
		m_nSubMenu = dwSubIndex;
		m_bInit = TRUE;
	}
	return m_bInit;
}

BOOL CMonitorTray::Release( )//释放
{
	if ( m_bInit )
	{
		Shell_NotifyIcon( NIM_DELETE, &m_Nid );
	}
	return !m_bInit;
}

void CMonitorTray::SetIcon( HICON hIcon )
{
	if ( !m_bInit )
		return;
	m_Nid.hIcon = hIcon;
	m_Nid.uFlags = NIF_ICON;
	Shell_NotifyIcon( NIM_MODIFY, &m_Nid );
}

HICON CMonitorTray::GetIcon( )
{
	return m_Nid.hIcon;
}

void CMonitorTray::OnNotifyIcon( WPARAM wParam, LPARAM lParam )//点击托盘图标事件
{
	if (wParam == 100)
	{
		if ((lParam == WM_LBUTTONDOWN) || (lParam == WM_RBUTTONDOWN))
		{
			//显示快捷子菜单
			if ( m_hMenu == NULL )
			{
				return;
			}
			POINT point;
			::GetCursorPos( &point );
			::SetForegroundWindow( m_hDlg );
			::TrackPopupMenu( ::GetSubMenu( m_hMenu, m_nSubMenu ), TPM_LEFTBUTTON | TPM_RIGHTBUTTON, point.x, point.y, 0, m_hDlg, 0 );
		}
	}
}

HWND CMonitorTray::GetDlg()
{
	return m_hDlg;
}

HMENU CMonitorTray::GetMenu()
{
	return m_hMenu;
}

int CMonitorTray::GetSubMenu()
{
	return m_nSubMenu;
}
