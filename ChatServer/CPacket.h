#pragma once
#include <Windows.h>


class CPacket
{
public:

	enum en_PACKET
	{
		eBUFFER_DEFAULT = 3000
	};


	CPacket();
	CPacket(int iBufferSize);

	virtual ~CPacket();

	void Release();
	void Clear();
	int GetBufferSize() { return m_iBufferSize; }
	int GetDataSize() { return m_iDataSize; }

	char* GetBufferPtr() { return m_chpBuffer; }

	int MoveWritePos(int iSize);
	int MoveReadPos(int iSize);

	int GetData(char* chpDest, int iSize);
	int PutData(char* chpSrc, int iSrcSize);

	CPacket& operator = (CPacket& cpPakcet);

	CPacket& operator << (BYTE byValue);
	CPacket& operator << (char chValue);

	CPacket& operator << (WCHAR wValue);
	CPacket& operator << (short shValue);
	CPacket& operator << (WORD wValue);

	CPacket& operator << (int iValue);
	CPacket& operator << (DWORD dwValue);
	CPacket& operator << (float fValue);

	CPacket& operator << (long lValue);
	CPacket& operator << (__int64 iValue);
	CPacket& operator << (double dValue);

	CPacket& operator >> (BYTE& byValue);
	CPacket& operator >> (char& chValue);

	CPacket& operator >> (WCHAR& wValue);
	CPacket& operator >> (short& shValue);
	CPacket& operator >> (WORD& wValue);

	CPacket& operator >> (int& iValue);
	CPacket& operator >> (DWORD& dwValue);
	CPacket& operator >> (float& fValue);

	CPacket& operator >> (long& lValue);
	CPacket& operator >> (__int64& iValue);
	CPacket& operator >> (double& dValue);



protected:

	friend class CRingBuffer;

	char* m_chpBuffer;
	int m_iDataSize;
	int m_iBufferSize;

	int m_iWritePos;
	int m_iReadPos;
};