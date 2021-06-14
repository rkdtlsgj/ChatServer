#include "CRingBuffer.h"

CRingBuffer::CRingBuffer()
{
	iFrontPos = 0;
	iRearPos = 0;
	iBufSize = BUF_SIZE;
	data = new char[BUF_SIZE];

	ZeroMemory(data, iBufSize);
}

CRingBuffer::CRingBuffer(int iBuffSize)
{
	iFrontPos = 0;
	iRearPos = 0;
	iBufSize = iBuffSize;
	data = new char[iBuffSize];

	ZeroMemory(data, iBufSize);
}

void CRingBuffer::ReSize(int size)
{

}

int CRingBuffer::GetBufferSize()
{
	return iBufSize;
}

int CRingBuffer::GetUseSize()
{
	if (iFrontPos <= iRearPos)
	{
		return iRearPos - iFrontPos;
	}
	else
	{
		return iBufSize - iFrontPos + iRearPos;
	}
}

int CRingBuffer::GetFreeSize()
{
	return iBufSize - (GetUseSize() + 1);
}

int CRingBuffer::DirectEnqueueSize()
{
	if (iRearPos < iFrontPos)
	{
		return (iFrontPos - iRearPos) - 1;
	}
	else
	{
		if (iRearPos < 1)
		{
			return (iBufSize - iRearPos) - (1 - iFrontPos);
		}
		else
		{
			return iBufSize - iRearPos;
		}
	}	
}

int CRingBuffer::DirectDequeueSize()
{
	if (iFrontPos <= iRearPos)
	{
		return iRearPos - iFrontPos;
	}
	else
	{
		return iBufSize - iFrontPos;
	}
}

int CRingBuffer::Enqueue(const char* chpData, int iSize)
{
	int writeSize;

	////남아있는 용량이 들어오는 용량보다 적다면
	if (GetFreeSize() < iSize)
	{
		writeSize = GetFreeSize();
	}
	else
	{
		writeSize = iSize;
	}
		
		//memcpy_s(&data[iRearPos], iBufSize, chpData, writeSize);
		
	if (iRearPos + writeSize > iBufSize)
	{
		int size = iBufSize - iRearPos;
		memcpy_s(&data[iRearPos], iBufSize, chpData, size);
		memcpy_s(&data[0], iBufSize, chpData + size, writeSize - size);
	}
	else
	{
		memcpy_s(&data[iRearPos], iBufSize, chpData, writeSize);
	}

	iRearPos = (iRearPos + writeSize) % iBufSize;

	return writeSize;
		
}

int CRingBuffer::Enqueue(CPacket* cpPacket)
{
	int writeSize;

	////남아있는 용량이 들어오는 용량보다 적다면
	if (GetFreeSize() < cpPacket->m_iDataSize)
	{
		writeSize = GetFreeSize();
	}
	else
	{
		writeSize = cpPacket->m_iDataSize;
	}

	if (iRearPos + writeSize > iBufSize)
	{
		int size = iBufSize - iRearPos;
		memcpy_s(&data[iRearPos], iBufSize, cpPacket->m_chpBuffer, size);
		memcpy_s(&data[0], iBufSize, cpPacket->m_chpBuffer + size, writeSize - size);
	}
	else
	{
		memcpy_s(&data[iRearPos], iBufSize, cpPacket->m_chpBuffer, writeSize);
	}

	iRearPos = (iRearPos + writeSize) % iBufSize;

	return writeSize;

}

int CRingBuffer::Dequeue(char* chpDest, int iSize)
{
	int readSize;


	if (GetUseSize() < iSize)
	{
		readSize = GetUseSize();		
	}
	else
	{
		readSize = iSize;
	}

	if (iFrontPos + readSize > iBufSize)
	{
		int size = iBufSize - iFrontPos;
		memcpy_s(chpDest, iSize, &data[iFrontPos], size);
		memcpy_s(chpDest+size, iSize, &data[0], readSize - size);
	}
	else
	{
		memcpy_s(chpDest,iSize, &data[iFrontPos], readSize);
	}

	iFrontPos = (iFrontPos + readSize) % iBufSize;

	return readSize;
}


int CRingBuffer::Dequeue(CPacket* cpPacket)
{
	int readSize;


	if (GetUseSize() < cpPacket->m_iBufferSize)
	{
		readSize = GetUseSize();
	}
	else
	{
		readSize = cpPacket->m_iBufferSize;
	}

	if (iFrontPos + readSize > iBufSize)
	{
		int size = iBufSize - iFrontPos;
		memcpy_s(cpPacket->m_chpBuffer, cpPacket->m_iBufferSize, &data[iFrontPos], size);
		memcpy_s(cpPacket->m_chpBuffer + size, cpPacket->m_iBufferSize, &data[0], readSize - size);
	}
	else
	{
		memcpy_s(cpPacket->m_chpBuffer, cpPacket->m_iBufferSize, &data[iFrontPos], readSize);
	}

	iFrontPos = (iFrontPos + readSize) % iBufSize;

	return readSize;
}

int CRingBuffer::Peek(char* chpDest, int iSize)
{
	int readSize;


	if (GetUseSize() < iSize)
	{
		readSize = GetUseSize();
	}
	else
	{
		readSize = iSize;
	}

	if (iFrontPos + readSize > iBufSize)
	{
		int size = iBufSize - iFrontPos;		
		memcpy_s(chpDest, iSize, &data[iFrontPos], size);
		memcpy_s(chpDest+size, iSize, &data[0], readSize - size);
	}
	else
	{
		memcpy_s(chpDest, iSize, &data[iFrontPos], readSize);
	}

	return readSize;
}

int CRingBuffer::MoveFront(int iSize)
{
	iFrontPos = (iFrontPos + iSize) % iBufSize;
	return iFrontPos;
}


int CRingBuffer::MoveRear(int iSize)
{
	iRearPos = (iRearPos + iSize) % iBufSize;
	return iRearPos;
}

void CRingBuffer::ClearBuffer()
{
	iFrontPos = 0;
	iRearPos = 0;
}

char* CRingBuffer::GetFrontBufferPtr()
{
	return data + iFrontPos;
}

char* CRingBuffer::GetRearBufferPtr()
{
	return &data[iRearPos];
}