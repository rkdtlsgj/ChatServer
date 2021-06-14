#include "CPacket.h"


CPacket::CPacket()
{
	m_chpBuffer = new char[en_PACKET::eBUFFER_DEFAULT];
	m_iBufferSize = en_PACKET::eBUFFER_DEFAULT;
	m_iWritePos = 0;
	m_iReadPos = 0;
	m_iDataSize = 0;

	ZeroMemory(m_chpBuffer, m_iBufferSize);
}

CPacket::CPacket(int iBufferSize)
{
	m_chpBuffer = new char[iBufferSize];
	m_iBufferSize = iBufferSize;
	m_iWritePos = 0;
	m_iReadPos = 0;
	m_iDataSize = 0;

	ZeroMemory(m_chpBuffer, m_iBufferSize);
}

CPacket::~CPacket()
{
	Release();
}

void CPacket::Release()
{
	delete m_chpBuffer;
}

void CPacket::Clear()
{
	m_iWritePos = 0;
	m_iReadPos = 0;
}

int CPacket::MoveWritePos(int iSize)
{

	if (m_iWritePos + iSize >= m_iBufferSize)
	{
		iSize = 0;
	}
	else
	{
		m_iWritePos += iSize;
	}

	m_iDataSize = m_iWritePos + m_iReadPos;

	return iSize;
}

int CPacket::MoveReadPos(int iSize)
{
	if (m_iReadPos + iSize > m_iWritePos)
	{
		iSize = 0;
	}
	else
	{
		m_iReadPos += iSize;
	}

	m_iDataSize = m_iWritePos + m_iReadPos;

	return iSize;
}

int CPacket::GetData(char* chpDest, int iSize)
{
	int readSize;

	if (m_iDataSize < iSize)
	{
		readSize = m_iDataSize;
	}
	else
	{
		readSize = iSize;
	}

	if (m_iReadPos + readSize > m_iWritePos)
	{
		readSize = 0;
	}
	else
	{
		memcpy_s(chpDest, iSize, &m_chpBuffer[m_iReadPos], m_iDataSize);
		m_iReadPos = (m_iReadPos + readSize) % m_iBufferSize;
		m_iDataSize -= readSize;
	}


	return readSize;
}

int CPacket::PutData(char* chpSrc, int iSrcSize)
{
	int writeSize;

	if (m_iWritePos + iSrcSize > m_iBufferSize)
	{
		writeSize = 0;
	}
	else
	{
		writeSize = iSrcSize;
		memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, chpSrc, writeSize);
		m_iWritePos = (m_iWritePos + writeSize) % m_iBufferSize;
		m_iDataSize += writeSize;
	}


	return writeSize;
}

CPacket& CPacket::operator = (CPacket& cpPakcet)
{
	delete[] m_chpBuffer;
	
	m_chpBuffer = new char[cpPakcet.m_iBufferSize];
	m_iDataSize = cpPakcet.m_iDataSize;
	m_iBufferSize = cpPakcet.m_iBufferSize;
	m_iWritePos = cpPakcet.m_iWritePos;
	m_iReadPos = cpPakcet.m_iReadPos;

	memcpy_s(m_chpBuffer, m_iBufferSize, cpPakcet.m_chpBuffer, cpPakcet.m_iDataSize);

	return *this;

}

CPacket& CPacket::operator<<(BYTE byValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &byValue, 1);
	m_iWritePos += 1;
	m_iDataSize += 1;

	return *this;
}

CPacket& CPacket::operator<<(char chValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &chValue, 1);
	m_iWritePos += 1;
	m_iDataSize += 1;

	return *this;
}

CPacket& CPacket::operator<<(short shValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &shValue, 2);
	m_iWritePos += 2;
	m_iDataSize += 2;

	return *this;
}

CPacket& CPacket::operator<<(WORD wValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &wValue, 2);
	m_iWritePos += 2;
	m_iDataSize += 2;

	return *this;
}

CPacket& CPacket::operator<<(int iValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &iValue, 4);
	m_iWritePos += 4;
	m_iDataSize += 4;

	return *this;
}

CPacket& CPacket::operator<<(DWORD dwValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &dwValue, 4);
	m_iWritePos += 4;
	m_iDataSize += 4;

	return *this;
}

CPacket& CPacket::operator<<(WCHAR dwValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &dwValue, 4);
	m_iWritePos += 4;
	m_iDataSize += 4;

	return *this;
}

CPacket& CPacket::operator<<(float fValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &fValue, 4);
	m_iWritePos += 4;
	m_iDataSize += 4;

	return *this;
}

CPacket& CPacket::operator<<(long lValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &lValue, 8);
	m_iWritePos += 8;
	m_iDataSize += 8;

	return *this;
}

CPacket& CPacket::operator<<(__int64 iValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &iValue, 8);
	m_iWritePos += 8;
	m_iDataSize += 8;

	return *this;
}

CPacket& CPacket::operator<<(double dValue)
{
	memcpy_s(&m_chpBuffer[m_iWritePos], m_iBufferSize, &dValue, 8);
	m_iWritePos += 8;
	m_iDataSize += 8;

	return *this;
}


CPacket& CPacket::operator>>(BYTE& byValue)
{
	memcpy_s(&byValue, 1, &m_chpBuffer[m_iReadPos], 1);
	m_iReadPos += 1;
	m_iDataSize -= 1;

	return *this;
}

CPacket& CPacket::operator>>(char& chValue)
{
	memcpy_s(&chValue, 1, &m_chpBuffer[m_iReadPos], 1);
	m_iReadPos += 1;
	m_iDataSize -= 1;

	return *this;
}

CPacket& CPacket::operator>>(short& shValue)
{
	memcpy_s(&shValue, 2, &m_chpBuffer[m_iReadPos], 2);
	m_iReadPos += 2;
	m_iDataSize -= 2;

	return *this;
}

CPacket& CPacket::operator>>(WORD& wValue)
{
	memcpy_s(&wValue, 2, &m_chpBuffer[m_iReadPos], 2);
	m_iReadPos += 2;
	m_iDataSize -= 2;

	return *this;
}

CPacket& CPacket::operator>>(WCHAR& wValue)
{
	memcpy_s(&wValue, 2, &m_chpBuffer[m_iReadPos], 2);
	m_iReadPos += 2;
	m_iDataSize -= 2;

	return *this;
}

CPacket& CPacket::operator>>(int& iValue)
{
	memcpy_s(&iValue, 4, &m_chpBuffer[m_iReadPos], 4);
	m_iReadPos += 4;
	m_iDataSize -= 4;

	return *this;
}

CPacket& CPacket::operator>>(DWORD& dwValue)
{
	memcpy_s(&dwValue, 4, &m_chpBuffer[m_iReadPos], 4);
	m_iReadPos += 4;
	m_iDataSize -= 4;

	return *this;
}

CPacket& CPacket::operator>>(float& fValue)
{
	memcpy_s(&fValue, 4, &m_chpBuffer[m_iReadPos], 4);
	m_iReadPos += 4;
	m_iDataSize += 4;

	return *this;
}

CPacket& CPacket::operator>>(long& lValue)
{
	memcpy_s(&lValue, 8, &m_chpBuffer[m_iReadPos], 8);
	m_iReadPos += 8;
	m_iDataSize += 8;

	return *this;
}

CPacket& CPacket::operator>>(__int64& iValue)
{
	memcpy_s(&iValue, 8, &m_chpBuffer[m_iReadPos], 8);
	m_iReadPos += 8;
	m_iDataSize += 8;

	return *this;
}


CPacket& CPacket::operator>>(double& dValue)
{
	memcpy_s(&dValue, 8, &m_chpBuffer[m_iReadPos], 8);
	m_iReadPos += 8;
	m_iDataSize += 8;

	return *this;
}