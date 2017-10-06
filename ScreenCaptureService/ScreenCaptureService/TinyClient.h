#ifndef _CLIENT_H_
#define _CLIENT_H_
#include <Winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#ifndef WIN32
typedef int SOCKET;
typedef int BOOL;
#endif

class CTinyClient
{
public:
	BOOL Init();
	BOOL Connect( const char *szAddr, int nPort );
	void Disconnect();
	int Read(char *byBuffer, int nBuffLen);
	int ReadSafety(char *byBuffer, int nBuffLen);
	BOOL Write(void *arg, int nBytesToWt);
	void Relese();
	BOOL IsInit() const;

	void SendCreateEncoder(int nWidth, int nHeight);
	void SendDestroyEncoder();
	void SendEncodeData(BYTE *data, int nDataLen);

public:
	CTinyClient();
	~CTinyClient();

private:
	SOCKET	m_sockClient;
};

#endif