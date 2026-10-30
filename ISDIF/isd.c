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
#include "NitEmu/isd_cmd.h"
#include "NitEmu/IsdPrivate.h"



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



static wchar_t *PathCat(const wchar_t *str1, const wchar_t *str2) {
	size_t len1 = wcslen(str1), len2 = wcslen(str2);
	wchar_t *buf = (wchar_t *) malloc((len1 + len2 + 1) * sizeof(wchar_t));
	if (buf == NULL) return NULL;

	memcpy(buf, str1, len1 * sizeof(wchar_t));
	memcpy(buf + len1, str2, (len2 + 1) * sizeof(wchar_t));
	return buf;
}

static IsdDevice *IsdOpenInternal(const wchar_t *devPath, IsdShareMode shareMode) {
	IsdDevice *pDev = (IsdDevice *) calloc(1, sizeof(IsdDevice));
	if (pDev == NULL) goto Error;

	pDev->status = ISD_STATUS_OK;
	pDev->shareMode = shareMode;

	wchar_t *pipe00 = PathCat(devPath, L"\\Pipe00");
	wchar_t *pipe01 = PathCat(devPath, L"\\Pipe01");
	wchar_t *mutexPath = PathCat(L"Global@ISU_", devPath + 4);

	//main device handle
	pDev->hDev = INVALID_HANDLE_VALUE;
	if (devPath != NULL) {
		pDev->hDev = CreateFile(devPath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
	}

	//write pipe end
	pDev->hWritePipe = INVALID_HANDLE_VALUE;
	if (pipe00 != NULL) {
		pDev->hWritePipe = CreateFile(pipe00,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
		free(pipe00);
	}

	//read pipe end
	pDev->hReadPipe = INVALID_HANDLE_VALUE;
	if (pipe01 != NULL) {
		pDev->hReadPipe = CreateFile(pipe01,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, NULL);
		free(pipe01);
	}

	//shared mutex
	pDev->hMutex = NULL;
	if (mutexPath != NULL) {
		pDev->hMutex = CreateMutex(NULL, FALSE, mutexPath);
		free(mutexPath);
	}

	//checking our device handles opened correctly
	if (pDev->hDev       == INVALID_HANDLE_VALUE) goto Error;
	if (pDev->hReadPipe  == INVALID_HANDLE_VALUE) goto Error;
	if (pDev->hWritePipe == INVALID_HANDLE_VALUE) goto Error;
	if (pDev->hMutex     ==                 NULL) goto Error;

	//checking the USB device descriptor
	DWORD dwBytesReturned;
	UsbDeviceDescriptor desc = { 0 };
	BOOL b = DeviceIoControl(pDev->hDev,
		ISD_IOCTL_GET_DEVICE_DESC,
		&desc, sizeof(desc),
		&desc, sizeof(desc),
		&dwBytesReturned, NULL);

	//checking the USB device descriptor
	if (!b || dwBytesReturned < sizeof(desc)) goto Error;
	if (desc.idVendor != ISD_VID || desc.idProduct != ISD_PID) goto Error;

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

IsdDevice *IsdOpen(const wchar_t *devPath, IsdShareMode shareMode) {
	if (devPath != NULL) return IsdOpenInternal(devPath, shareMode);

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData = { 0 };
	SP_DEVICE_INTERFACE_DETAIL_DATA *pDetail = NULL;
	DWORD requiredSize = 0, memberIndex = 0;
	IsdDevice *pDev = NULL;

	HDEVINFO hDevInfo = SetupDiGetClassDevs(&ISD_GUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE) return NULL;

	while (pDev == NULL) {
		deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
		if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &ISD_GUID, memberIndex++, &deviceInterfaceData)) break;

		//query required size
		if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0, &requiredSize, NULL)) {
			if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) break;
		}

		free(pDetail); // free old detail
		pDetail = (SP_DEVICE_INTERFACE_DETAIL_DATA *) calloc(1, requiredSize);
		if (pDetail == NULL) break;

		pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		//real query
		if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, pDetail, requiredSize, &requiredSize, NULL)) break;

		//try open
		pDev = IsdOpenInternal(pDetail->DevicePath, shareMode);
	}

	if (hDevInfo != INVALID_HANDLE_VALUE) SetupDiDestroyDeviceInfoList(hDevInfo);
	free(pDetail);
	return pDev;
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



static void IsdStartTimer(IsdDevice *pDev) {
	//assumed by now to have successful TSC
	QueryPerformanceCounter(&pDev->timerStart);
}

static int IsdCheckTimerExpired(IsdDevice *pDev) {
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



// ----- Low-level routines

static int IsdWaitMon_CmdComplete(IsdDevice *pDev) {
	IsdStartTimer(pDev);

	while (1) {
		int status = IsdCmdSend_WaitMonCmdComplete(pDev, &pDev->monStatus);
		if (!status) return 0;

		if (pDev->monStatus.cmdComplete) return 1;

		//check timeout state
		if (IsdCheckTimerExpired(pDev)) return 0;
	}
}

static int IsdWaitMon_FiqEntry(IsdDevice *pDev) {
	IsdStartTimer(pDev);

	while (1) {
		int status = IsdCmdSend_WaitMonEntry(pDev, &pDev->monStatus);
		if (!status) return 0;

		if (pDev->monStatus.fiqEnter) return 1;

		//check timeout state
		if (IsdCheckTimerExpired(pDev)) return 0;
	}
}

static int IsdReadMemChunk(IsdDevice *pDev, uint32_t addr, void *dest, unsigned int size) {
	int status;

	status = IsdCmdSend_StartReadMem(pDev, addr, size);
	if (!status) return 0;

	status = IsdWaitMon_CmdComplete(pDev);
	if (!status) return 0;

	status = IsdCmdSend_GetReadMemResult(pDev, dest, size);
	if (!status) return 0;

	return 1;
}

static int IsdWriteMemChunk(IsdDevice *pDev, uint32_t dest, const void *buf, unsigned int size) {
	return IsdCmdSend_WriteMem(pDev, dest, buf, size);
}



// ----- Chunked transfer routines

typedef int (*IsdWriteRoutine) (IsdDevice *pDev, uint32_t addr, const void *p, size_t size);
typedef int (*IsdReadRoutine) (IsdDevice *pDev, uint32_t addr, void *p, size_t size);

static int IsdWriteByFuncp(IsdDevice *pDev, uint32_t dest, const void *src, size_t size, IsdWriteRoutine funcp, size_t maxBlock) {
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

static int IsdReadByFuncp(IsdDevice *pDev, void *dest, uint32_t src, size_t size, IsdReadRoutine funcp, size_t maxBlock) {
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



// ----- high-level procedures

int IsdGetSerialInternal(IsdDevice *pDev, unsigned char *pSerial) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdCmdSend_GetSerial(pDev, pSerial);

	(void) IsdLockRelease(pDev);
	return status;
}



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



int IsdSelectProc(IsdDevice *pDev, IsdProc proc) {
	int status;

	status = IsdCmdSend_SelectProc(pDev, proc);
	if (!status) return 0;

	pDev->curproc = proc;
	return 1;
}



int IsdRaiseFIQ(IsdDevice *pDev) {
	return IsdCmdSend_SetFIQ(pDev, 1);
}

int IsdLowerFIQ(IsdDevice *pDev) {
	return IsdCmdSend_SetFIQ(pDev, 0);
}



static int IsdMonEnter_NoLock(IsdDevice *pDev) {
	if (!IsdRaiseFIQ(pDev)) return 0;

	int status = IsdWaitMon_FiqEntry(pDev);

	if (!IsdLowerFIQ(pDev)) return 0;

	return status;
}

int IsdMonEnter(IsdDevice *pDev) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdMonEnter_NoLock(pDev);
	
	(void) IsdLockRelease(pDev);
	return status;
}



static int IsdMonLeave_NoLock(IsdDevice *pDev) {
	int status;

	status = IsdWaitMon_CmdComplete(pDev);
	if (!status) return 0;

	status = IsdCmdSend_FiqLeave(pDev);
	if (!status) return 0;

	status = IsdWaitMon_CmdComplete(pDev);
	if (!status) return 0;

	return 1;
}

int IsdMonLeave(IsdDevice *pDev) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdMonLeave_NoLock(pDev);

	(void) IsdLockRelease(pDev);
	return status;
}

int IsdMonGetStatus(IsdDevice *pDev, IsdMonStatus *pStatus) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdCmdSend_GetMonStatus(pDev, pStatus);

	(void) IsdLockRelease(pDev);
	return status;
}



// ----- The various read/write routines

static int IsdReadMem_Proc(IsdDevice *pDev, uint32_t addr, void *p, size_t size) {
	return IsdReadMemChunk(pDev, addr, p, size);
}

static int IsdReadMem_NoLock(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	return IsdReadByFuncp(pDev, dest, src, size, IsdReadMem_Proc, ISD_READ_MEM_MAX);
}

int IsdReadMem(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdReadMem_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}



static int IsdWriteMem_Proc(IsdDevice *pDev, uint32_t addr, const void *p, size_t size) {
	if (!IsdWriteMemChunk(pDev, addr, p, size)) return 0;

	if (!IsdWaitMon_CmdComplete(pDev)) return 0;

	return 1;
}

static int IsdWriteMem_NoLock(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	return IsdWriteByFuncp(pDev, dest, src, size, IsdWriteMem_Proc, ISD_WRITE_MEM_MAX);
}

int IsdWriteMem(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdWriteMem_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}



static int IsdReadAGB_Proc(IsdDevice *pDev, uint32_t addr, void *buf, size_t size) {
	//the command executes synchronously on NEC. No need to wait explicitly.
	IsdCommandResult res;
	if (!IsdCmdSend_ReadAGB(pDev, addr, size, buf, &res)) return 0;

	//checking the status of the operation on completion.
	if (!res.retVal) {
		pDev->status = ISD_STATUS_FAILED;
		return 0;
	}

	return 1;
}

static int IsdReadAGB_NoLock(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	return IsdReadByFuncp(pDev, dest, src, size, IsdReadAGB_Proc, ISD_READ_AGB_MAX);
}

int IsdReadAGB(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdReadAGB_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}



static int IsdWriteAGB_Proc(IsdDevice *pDev, uint32_t addr, const void *buf, size_t size) {
	//the command executes synchronously on NEC. No need to wait explicitly.
	IsdCommandResult res;
	if (!IsdCmdSend_WriteAGB(pDev, addr, buf, size, &res)) return 0;

	//checking the status of the operation on completion.
	if (!res.retVal) {
		pDev->status = ISD_STATUS_FAILED;
		return 0;
	}

	return 1;
}

static int IsdWriteAGB_NoLock(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	return IsdWriteByFuncp(pDev, dest, src, size, IsdWriteAGB_Proc, ISD_WRITE_AGB_MAX);
}

int IsdWriteAGB(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdWriteAGB_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}



static int IsdReadEmulation_Proc(IsdDevice *pDev, uint32_t addr, void *buf, size_t size) {
	//the command executes synchronously on NEC. No need to wait explicitly.
	IsdCommandResult res;
	if (!IsdCmdSend_ReadSDRAM(pDev, addr, size, buf, &res)) return 0;

	//checking the status of the operation on completion.
	if (!res.retVal) {
		pDev->status = ISD_STATUS_FAILED;
		return 0;
	}

	return 1;
}

static int IsdReadEmulation_NoLock(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	return IsdReadByFuncp(pDev, dest, src, size, IsdReadEmulation_Proc, ISD_READ_SDRAM_MAX);
}

int IsdReadEmulation(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdReadEmulation_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}



static int IsdWriteEmulation_Proc(IsdDevice *pDev, uint32_t addr, const void *buf, size_t size) {
	//the command executes synchronously on NEC. No need to wait explicitly.
	IsdCommandResult res;
	if (!IsdCmdSend_WriteSDRAM(pDev, addr, buf, size, &res)) return 0;

	//checking the status of the operation on completion.
	if (!res.retVal) {
		pDev->status = ISD_STATUS_FAILED;
		return 0;
	}

	return 1;
}

static int IsdWriteEmulation_NoLock(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	return IsdWriteByFuncp(pDev, dest, src, size, IsdWriteEmulation_Proc, ISD_WRITE_SDRAM_MAX);
}

int IsdWriteEmulation(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	if (!IsdLockAcquire(pDev)) return 0;

	int status = IsdWriteEmulation_NoLock(pDev, dest, src, size);

	(void) IsdLockRelease(pDev);
	return status;
}

