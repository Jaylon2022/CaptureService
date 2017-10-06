/******************************************************************************************
*
*  FileName:    DebugLog.h
*
*  Author:      dailongjian
*
*  Date:        2012-7-15
*
*  Description: 打印log文件
*
*******************************************************************************************/
#ifndef DEBUGLOGFILE_H_
#define DEBUGLOGFILE_H_

#include <windows.h>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <atlconv.h>

#ifdef _WIN64
#define PATH_LOG     "\\QXLLOG64.log"
#else
#define PATH_LOG     "\\QXLLOG.log"
#endif
#define MAX_LOGFILE_SIZE  1024*1024  /* 最大日志文件设置为1M */

class CSRLog{
public:
	CSRLog(std::string strPath){/* 自定义log路径 */
		m_strPath = strPath;
		m_file = _fsopen(m_strPath.c_str(), "ab", _SH_DENYWR);/* 允许共享 */
		InitializeCriticalSection(&m_cs);
	}

	CSRLog(){
		GetLogPath();
		if (GetSize() >= MAX_LOGFILE_SIZE){
			m_file = _fsopen(m_strPath.c_str(), "wb", _SH_DENYWR);/* 允许共享 */
		}
		else{
			m_file = _fsopen(m_strPath.c_str(), "ab", _SH_DENYWR);/* 允许共享 */
		}
		InitializeCriticalSection(&m_cs);
	}

	~CSRLog(){
		if (m_file != NULL){
			fclose(m_file);
		}
	}

	void SR_printf(const char *pMessage, ...){
		if (m_file == NULL){
			return;
		}
		SYSTEMTIME sys;
		GetLocalTime(&sys);
		
		char szBuffer[1024] = { 0 };
		va_list args;
		va_start(args, pMessage);
		_vsnprintf_s(szBuffer, _countof(szBuffer) - 1, pMessage, args);
		va_end(args);
		EnterCriticalSection(&m_cs);
		fprintf_s(m_file, "[%02d-%02d %02d:%02d:%02d:%03d] %s\x0D\x0A", sys.wMonth, sys.wDay, sys.wHour, 
			sys.wMinute, sys.wSecond, sys.wMilliseconds, szBuffer);
		fflush(m_file);
		LeaveCriticalSection(&m_cs);
	}

	void SR_printfW(const WCHAR *pMessage, ...){
		if (m_file == NULL){
			return;
		}
		SYSTEMTIME sys;
		GetLocalTime(&sys);

		WCHAR szBuffer[1024] = {0};
		va_list args;
		va_start(args, pMessage);
		vswprintf_s(szBuffer, _countof(szBuffer) - 1, pMessage, args);
		va_end(args);
		EnterCriticalSection(&m_cs);
		USES_CONVERSION;
		fprintf_s(m_file, "[%02d-%02d %02d:%02d:%02d:%03d] %s\x0D\x0A", sys.wMonth, sys.wDay, sys.wHour, 
			sys.wMinute, sys.wSecond, sys.wMilliseconds, W2A(szBuffer));
		fflush(m_file);
		LeaveCriticalSection(&m_cs);
	}

	static void SR_dbgPrintf(char *pMessage, ...){
		char strBuffer[4096] = { 0 };
		va_list vlArgs;
		va_start(vlArgs, pMessage);
		_vsnprintf_s(strBuffer, sizeof(strBuffer) - 1, pMessage, vlArgs);
		va_end(vlArgs);
		OutputDebugStringA(strBuffer);
	}

	void OutputUUID(REFIID  uuid){
		//unsigned char *str;
		//UuidToStringA(&uuid, &str);
		//OutputDebugPrintf((char *)str);
		//OutputDebugPrintf("processId=%04X  classid=%s", GetCurrentProcessId(), (char *)str);
		//LPOLESTR oleStr;
		//ProgIDFromCLSID(rclsid, &oleStr);
		//OutputDebugString(oleStr);
	}

private:
	void GetLogPath(){
		char szPath[MAX_PATH];
		GetModuleFileNameA(NULL, szPath, sizeof(szPath));
		std::string path = szPath;
		size_t pos = path.rfind('\\');
		std::string subpath = path.substr(0, pos);

		m_strPath = subpath + PATH_LOG;
#if 0
		char szLog[100];
		sprintf_s(szLog, "%d", GetCurrentProcessId());
		m_strPath = subpath + "\\" + szLog + ".log";
#endif
	}

	size_t GetSize() const{
		struct _stat statbuf;
		_stat(m_strPath.c_str(), &statbuf);
		return statbuf.st_size;
	}

private:
	FILE *m_file;
	std::string m_strPath;
	CRITICAL_SECTION m_cs;
};


#endif