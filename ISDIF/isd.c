// -----------------------------------------------------------------------------------------------
// Copyright (c) 2026, Garhoogin
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted
// provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of
//    conditions and the following disclaimer in the documentation and/or other materials provided
//    with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// -----------------------------------------------------------------------------------------------
#include <initguid.h>
#include <Windows.h>
#include <SetupAPI.h>

#include "isd.h"
#include "IsdPrivate.h"
#include "NitEmu/NitEmu.h"



// ----- lock routines

static int IsdMutAcquire(IsdDevice *pDev) {
	if (pDev->mutOwned) return 1;  // lock already held

	DWORD result = WaitForSingleObject(pDev->hMutex, 
		pDev->lockTimeout == 0 ? INFINITE : pDev->lockTimeout);
	
	switch (result) {
		case WAIT_OBJECT_0:
		case WAIT_ABANDONED:
			//success
			pDev->mutOwned = 1;
			return 1;
		case WAIT_TIMEOUT:
			//time out
			pDev->status = ISD_STATUS_BUSY;
			return 0;
	}
	
	//should not reach here
	pDev->status = ISD_STATUS_IRREGULAR;
	return 0;
}

static int IsdMutRelease(IsdDevice *pDev) {
	if (!pDev->mutOwned) return 1;  // lock not held

	ReleaseMutex(pDev->hMutex);
	pDev->mutOwned = 0;
	return 1;
}

static int IsdLockReference(IsdDevice *pDev) {
	if (pDev->lockCount == UINT_MAX) return 0;  // how did we get here?

	if (pDev->lockCount == 0) {
		if (!IsdMutAcquire(pDev)) return 0;
	}

	//successful lock
	pDev->lockCount++;
	return 1;
}

static int IsdLockDereference(IsdDevice *pDev) {
	if (pDev->lockCount == 0) return 1;  // lock not held

	if (--pDev->lockCount == 0) {
		(void) IsdMutRelease(pDev);
	}

	return 1;
}



int IsdLockAcquire(IsdDevice *pDev) {
	switch (pDev->shareMode) {
		case ISD_SHARE_MODE_EXCLUSIVE:
			//in exclusive mode, the lock is already held.
			return 1;
		case ISD_SHARE_MODE_SHARED:
			//in shared mode, we acquire the lock
			return IsdLockReference(pDev);
		case ISD_SHARE_MODE_PASSIVE:
			//passive mode does not use a lock. Always successful.
			return 1;
	}

	//bad mode
	return 0;
}

int IsdLockRelease(IsdDevice *pDev) {
	switch (pDev->shareMode) {
		case ISD_SHARE_MODE_EXCLUSIVE:
			//in exclusive mode, the lock is not released until the device is freed.
			return 1;
		case ISD_SHARE_MODE_SHARED:
			//in shared mode, we free the lock
			return IsdLockDereference(pDev);
		case ISD_SHARE_MODE_PASSIVE:
			//passive mode does not use a lock. Always successful.
			return 1;
	}

	//bad mode
	return 0;
}

void IsdSetLockTimeout(IsdDevice *pDev, unsigned int ms) {
	pDev->lockTimeout = ms;
}



IsdDevice *IsdOpen(const wchar_t *devPath, IsdShareMode shareMode) {
	IsdDevice *pDev = IsdOpen_NitEmu(devPath, shareMode);
	if (pDev == NULL) return NULL;

	//init timer
	if (!QueryPerformanceFrequency(&pDev->timerFreq)) goto Error;
	IsdSetTimeout(pDev, ISD_DEFAULT_MON_TIMEOUT);
	IsdSetLockTimeout(pDev, ISD_DEFAULT_LOCK_TIMEOUT);

	//in exclusive mode, acquire the lock.
	if (shareMode == ISD_SHARE_MODE_EXCLUSIVE) {
		if (!IsdMutAcquire(pDev)) goto Error;
	}

	//test request the device info
	if (!IsdGetSerialInternal(pDev, pDev->serial)) goto Error;

	return pDev;

Error:
	if (pDev != NULL) IsdClose(pDev);
	return NULL;
}

void IsdClose(IsdDevice *pDev) {
	IsdMutRelease(pDev);

	if (pDev->hDev != INVALID_HANDLE_VALUE && pDev->hDev != NULL) {
		CloseHandle(pDev->hDev);
	}
	if (pDev->hWritePipe != INVALID_HANDLE_VALUE && pDev->hWritePipe != NULL) {
		CloseHandle(pDev->hWritePipe);
	}
	if (pDev->hReadPipe != INVALID_HANDLE_VALUE && pDev->hReadPipe != NULL) {
		CloseHandle(pDev->hReadPipe);
	}

	if (pDev->hMutex != NULL) CloseHandle(pDev->hMutex);

	free(pDev);
}

void IsdClearLastError(IsdDevice *pDev) {
	pDev->status = ISD_STATUS_OK;
}

IsdStatus IsdGetLastError(IsdDevice *pDev) {
	return pDev->status;
}

void IsdSetTimeout(IsdDevice *pDev, int ms) {
	pDev->timerTimeout.QuadPart = ms * pDev->timerFreq.QuadPart / 1000;
}



// ----- Chunked transfer routines

int IsdWriteByFuncp(IsdDevice *pDev, uint32_t dest, const void *src, size_t size, IsdWriteRoutine funcp, size_t maxBlock) {
	//we break the reads into chunks.
	while (size > 0) {
		//transfer 1 block
		size_t blocklen = size > maxBlock ? maxBlock : size;
		if (!funcp(pDev, dest, src, blocklen)) return 0;

		src = (const unsigned char *) src + blocklen;
		size -= blocklen;
		dest += blocklen;
	}

	return 1;
}

int IsdReadByFuncp(IsdDevice *pDev, void *dest, uint32_t src, size_t size, IsdReadRoutine funcp, size_t maxBlock) {
	//we break the reads into chunks.
	while (size > 0) {
		//transfer 1 block
		size_t blocklen = size > maxBlock ? maxBlock : size;
		if (!funcp(pDev, src, dest, blocklen)) return 0;

		dest = (unsigned char *) dest + blocklen;
		size -= blocklen;
		src += blocklen;
	}

	return 1;
}



// ----- Info functions

void IsdGetSerial(IsdDevice *pDev, char *out) {
	memcpy(out, pDev->serial, ISD_SERIAL_SIZE);
}


// ----- Device IO routines

int IsdWrite(IsdDevice *pDev, const void *data, unsigned int size) {
	while (size > 0) {
		unsigned int nChunk = size;
		if (nChunk > ISD_WRITE_MAX) nChunk = ISD_WRITE_MAX;

		//can't use IO timeout, not supported by driver
		DWORD nWritten;
		BOOL res = WriteFile(pDev->hWritePipe, data, nChunk, &nWritten, NULL);

		if (!res || nWritten == 0) {
			//transfer fail
			pDev->status = ISD_STATUS_COMM_CLOSE;
			return 0;
		}
		
		size -= nWritten;
		data = (const unsigned char *) data + nWritten;
	}

	return 1;  // success
}

int IsdRead(IsdDevice *pDev, void *out, unsigned int size) {
	while (size > 0) {
		unsigned int nChunk = size;
		if (nChunk > ISD_READ_MAX) nChunk = ISD_READ_MAX;

		//can't use IO timeout, not supported by driver
		DWORD nRead;
		BOOL res = ReadFile(pDev->hReadPipe, out, nChunk, &nRead, NULL);

		if (!res || nRead == 0) {
			//transfer fail
			pDev->status = ISD_STATUS_COMM_CLOSE;
			return 0;
		}

		size -= nRead;
		out = (unsigned char *) out + nRead;
	}

	return 1;  // success
}



void IsdStartTimer(IsdDevice *pDev) {
	//assumed by now to have successful TSC
	QueryPerformanceCounter(&pDev->timerStart);
}

int IsdCheckTimerExpired(IsdDevice *pDev) {
	//if the timeout is set to zero, we treat as no timeout
	if (pDev->timerTimeout.QuadPart == 0) return 0;

	//assumed by now to have successful TSC
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);

	long long int elapse = time.QuadPart - pDev->timerStart.QuadPart;

	if (elapse >= pDev->timerTimeout.QuadPart) {
		//timer expire
		pDev->status = ISD_STATUS_MON_TIMEOUT;
		return 1;
	} else {
		//timer not expire
		return 0;
	}
}



// ----- high-level procedures

int IsdMonRunning(IsdDevice *pDev) {
	//need to check?
	if (!pDev->monStart) {
		//check the signature exists in AGB RAM
		char buf[4];
		if (!IsdReadAGB(pDev, buf, 0x100, sizeof(buf))) return 0;

		if (memcmp(buf, "ISID", 4) == 0) pDev->monStart = 1;
	}

	return pDev->monStart;
}


static int IsdGetSerialInternal_NoLock(IsdDevice *pDev, unsigned char *pSerial) {
	return pDev->pVtbl->GetSerial(pDev, pSerial);
}

int IsdGetSerialInternal(IsdDevice *pDev, unsigned char *pSerial) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdGetSerialInternal_NoLock(pDev, pSerial);

	(void) IsdLockRelease(pDev);
	return status;
}


static int IsdSelectProc_NoLock(IsdDevice *pDev, IsdProc proc) {
	return pDev->pVtbl->SelectProc(pDev, proc);
}

int IsdSelectProc(IsdDevice *pDev, IsdProc proc) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdSelectProc_NoLock(pDev, proc);

	(void) IsdLockRelease(pDev);
	return status;
}


static int IsdMonEnter_NoLock(IsdDevice *pDev) {
	return pDev->pVtbl->MonEnter(pDev);
}

int IsdMonEnter(IsdDevice *pDev) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdMonEnter_NoLock(pDev);
	
	(void) IsdLockRelease(pDev);
	return status;
}


static int IsdMonLeave_NoLock(IsdDevice *pDev) {
	return pDev->pVtbl->MonLeave(pDev);
}

int IsdMonLeave(IsdDevice *pDev) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdMonLeave_NoLock(pDev);

	(void) IsdLockRelease(pDev);
	return status;
}


static int IsdMonGetStatus_NoLock(IsdDevice *pDev, IsdMonStatus *pStatus) {
	return pDev->pVtbl->MonGetStatus(pDev, pStatus);
}

int IsdMonGetStatus(IsdDevice *pDev, IsdMonStatus *pStatus) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdMonGetStatus_NoLock(pDev, pStatus);

	(void) IsdLockRelease(pDev);
	return status;
}



// ----- The various read/write routines

static int IsdReadMem_NoLock(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	return pDev->pVtbl->ReadMem(pDev, dest, src, size);
}

int IsdReadMem(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdReadMem_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}


static int IsdWriteMem_NoLock(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	return pDev->pVtbl->WriteMem(pDev, dest, src, size);
}

int IsdWriteMem(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdWriteMem_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}


static int IsdReadAGB_NoLock(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	return pDev->pVtbl->ReadAGB(pDev, dest, src, size);
}

int IsdReadAGB(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdReadAGB_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}


static int IsdWriteAGB_NoLock(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	return pDev->pVtbl->WriteAGB(pDev, dest, src, size);
}

int IsdWriteAGB(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdWriteAGB_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}


static int IsdReadEmulation_NoLock(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	return pDev->pVtbl->ReadEmulation(pDev, dest, src, size);
}

int IsdReadEmulation(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdReadEmulation_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}


static int IsdWriteEmulation_NoLock(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	return pDev->pVtbl->WriteEmulation(pDev, dest, src, size);
}

int IsdWriteEmulation(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdWriteEmulation_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}

