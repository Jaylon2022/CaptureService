#include "stdafx.h"
#include "TinyClient.h"
#include <stdio.h>
#include <WS2tcpip.h>
#include "protocol.h"

CTinyClient::CTinyClient()
{
	m_sockClient = INVALID_SOCKET;
}

CTinyClient::~CTinyClient()
{

}

BOOL CTinyClient::Init()
{
#ifdef WIN32
	WORD wVersion = MAKEWORD( 1, 1 );
	WSADATA wsData;
	int nResult = WSAStartup( wVersion, &wsData );

	if ( nResult != 0 ){
		return FALSE;
	}
#endif

	return TRUE;
}

BOOL CTinyClient::Connect(const char *szAddr, int nPort)
{
	SOCKET sock;
	struct sockaddr_in stSockAddrInfo;
	int nSize = sizeof( struct sockaddr_in );
	int flag = 1; 
	if( NULL == szAddr || nPort < 0 || nPort > 65535 ){
		return FALSE;
	}

	memset( &stSockAddrInfo, 0, sizeof( stSockAddrInfo ));
	stSockAddrInfo.sin_family = AF_INET;
	stSockAddrInfo.sin_port = htons( nPort );

	inet_pton(AF_INET, szAddr, (void *)&stSockAddrInfo.sin_addr.s_addr);

	//stSockAddrInfo.sin_addr.s_addr = inet_addr( szAddr );
	sock = socket( AF_INET, SOCK_STREAM, 0 );
	if( sock < 0 ){
		return FALSE;
	}

	setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) ); 
	if( connect( sock, (struct sockaddr *)&stSockAddrInfo, nSize ) < 0 ){
		closesocket( sock );
		return FALSE;
	}
	m_sockClient = sock;
	return TRUE;
}


BOOL CTinyClient::Write(LPVOID arg, int nBytesToWt)
{
	int nWriteLen = 0;
	int nTemp;
	if( arg == NULL || m_sockClient <= 0  || nBytesToWt <= 0 ){
		return FALSE;
	}
	
	while( nWriteLen < nBytesToWt )	{
		nTemp = send ( m_sockClient, ((char *)arg) + nWriteLen, nBytesToWt - nWriteLen, 0 );
		if( nTemp < 0 ){
			return FALSE;
		}

		nWriteLen += nTemp;
	}
	return TRUE;
}

int CTinyClient::Read(char *byBuffer, int nBuffLen)
{
	return 	recv(m_sockClient, byBuffer, nBuffLen, 0);
}

int CTinyClient::ReadSafety(char *byBuffer, int nBuffLen)
{
	DWORD dwHasRead = 0;
	while (dwHasRead < nBuffLen){
		dwHasRead += recv(m_sockClient, byBuffer+dwHasRead, nBuffLen-dwHasRead, 0);
	}

	return dwHasRead;
}


void CTinyClient::Disconnect()
{
	if (m_sockClient > 0){
		closesocket(m_sockClient);
		m_sockClient = INVALID_SOCKET;
	}
}

void CTinyClient::Relese()
{
#ifdef WIN32
	WSACleanup();
#endif
}

BOOL CTinyClient::IsInit() const
{
	return m_sockClient > 0;
}


void CTinyClient::SendCreateEncoder(int nWidth, int nHeight)
{
	protocol_head_t head;
	head.type = MSG_MODE_STREAM_CREATE;
	head.length = sizeof(msg_stream_create);
	Write(&head, sizeof(protocol_head_t));

	msg_stream_create msc;
	msc.stream_width = nWidth;
	msc.stream_height = nHeight;
	Write(&msc, sizeof(msg_stream_create));
}

void CTinyClient::SendDestroyEncoder()
{
	protocol_head_t head;
	head.type = MSG_MODE_STREAM_DESTROY;
	head.length = sizeof(msg_stream_destroy);
	Write(&head, sizeof(protocol_head_t));

	msg_stream_destroy msd;
	msd.stream_id = 0;
	Write(&msd, sizeof(msg_stream_destroy));
}

void CTinyClient::SendEncodeData(BYTE *data, int nDataLen)
{
	protocol_head_t head;
	head.type = MSG_MODE_STREAM_DATA;
	head.length = nDataLen;
	Write(&head, sizeof(protocol_head_t));
	Write(data, nDataLen);
}