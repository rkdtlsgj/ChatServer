#pragma once

#define BUF_SIZE 1000
#include <memory.h>
#include <Windows.h>
#include "CPacket.h"

class CRingBuffer
{
public:
	CRingBuffer();
	CRingBuffer(int iBuffSize);


	void ReSize(int size);
	int GetBufferSize();

	int GetUseSize();
	int GetFreeSize();

	int DirectEnqueueSize();
	int DirectDequeueSize();

	int Enqueue(const char* chpData, int iSize);
	int Enqueue(CPacket* cpPacket);
	int Dequeue(char* chpDest, int iSize);
	int Dequeue(CPacket* cpPacket);

	int Peek(char* chpDest, int iSize);

	int MoveRear(int iSize);
	int MoveFront(int iSize);

	void ClearBuffer();

	char* GetFrontBufferPtr();
	char* GetRearBufferPtr();

private:
	int iFrontPos;
	int iRearPos;
	int iBufSize;
	char* data;
};
