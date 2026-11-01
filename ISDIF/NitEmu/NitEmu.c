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
#include "IsdCommand.h"
#include "NitEmu.h"
#include "NitEmuPrivate.h"


// ----- virtual table

static int Isd_NitEmu_WaitMon_CmdComplete(IsdDevice *pDev);
static int Isd_NitEmu_WaitMon_FiqEntry(IsdDevice *pDev);
static int Isd_NitEmu_MonEnter(IsdDevice *pDev);
static int Isd_NitEmu_MonLeave(IsdDevice *pDev);
static int Isd_NitEmu_MonGetStatus(IsdDevice *pDev, IsdMonStatus *pStatus);
static int Isd_NitEmu_GetSerial(IsdDevice *pDev, unsigned char *pSerial);
static int Isd_NitEmu_SelectProc(IsdDevice *pDev, IsdProc proc);
static int Isd_NitEmu_ReadMem(IsdDevice *pDev, void *dest, uint32_t src, size_t size);
static int Isd_NitEmu_WriteMem(IsdDevice *pDev, uint32_t dest, const void *src, size_t size);
static int Isd_NitEmu_ReadAGB(IsdDevice *pDev, void *dest, uint32_t src, size_t size);
static int Isd_NitEmu_WriteAGB(IsdDevice *pDev, uint32_t dest, const void *src, size_t size);
static int Isd_NitEmu_ReadEmulation(IsdDevice *pDev, void *dest, uint32_t src, size_t size);
static int Isd_NitEmu_WriteEmulation(IsdDevice *pDev, uint32_t dest, const void *src, size_t size);

static const IsdDeviceVirtual sVtbl = {
	Isd_NitEmu_WaitMon_CmdComplete,
	Isd_NitEmu_WaitMon_FiqEntry,
	Isd_NitEmu_MonEnter,
	Isd_NitEmu_MonLeave,
	Isd_NitEmu_MonGetStatus,
	Isd_NitEmu_GetSerial,
	Isd_NitEmu_SelectProc,
	Isd_NitEmu_ReadMem,
	Isd_NitEmu_WriteMem,
	Isd_NitEmu_ReadAGB,
	Isd_NitEmu_WriteAGB,
	Isd_NitEmu_ReadEmulation,
	Isd_NitEmu_WriteEmulation
};



static wchar_t *PathCat(const wchar_t *str1, const wchar_t *str2) {
	size_t len1 = wcslen(str1), len2 = wcslen(str2);
	wchar_t *buf = (wchar_t *) malloc((len1 + len2 + 1) * sizeof(wchar_t));
	if (buf == NULL) return NULL;

	memcpy(buf, str1, len1 * sizeof(wchar_t));
	memcpy(buf + len1, str2, (len2 + 1) * sizeof(wchar_t));
	return buf;
}

IsdDevice *IsdOpen_NitEmuInternal(const wchar_t *devPath, IsdShareMode shareMode) {
	IsdDevice *pDev = (IsdDevice *) calloc(1, sizeof(IsdDevice));
	if (pDev == NULL) goto Error;

	pDev->pVtbl = &sVtbl;
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

	return pDev;

Error:
	if (pDev != NULL) IsdClose(pDev);
	return NULL;
}

IsdDevice *IsdOpen_NitEmu(const wchar_t *devPath, IsdShareMode shareMode) {
	if (devPath != NULL) return IsdOpen_NitEmuInternal(devPath, shareMode);

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
		pDev = IsdOpen_NitEmuInternal(pDetail->DevicePath, shareMode);
	}

	if (hDevInfo != INVALID_HANDLE_VALUE) SetupDiDestroyDeviceInfoList(hDevInfo);
	free(pDetail);
	return pDev;
}



// ----- Low-level routines

static int IsdRaiseFIQ(IsdDevice *pDev) {
	return IsdCmdSend_SetFIQ(pDev, 1);
}

static int IsdLowerFIQ(IsdDevice *pDev) {
	return IsdCmdSend_SetFIQ(pDev, 0);
}

static int IsdReadMemChunk(IsdDevice *pDev, uint32_t addr, void *dest, unsigned int size) {
	if (!IsdCmdSend_StartReadMem(pDev, addr, size)) return 0;

	if (!Isd_NitEmu_WaitMon_CmdComplete(pDev)) return 0;

	if (!IsdCmdSend_GetReadMemResult(pDev, dest, size)) return 0;

	return 1;
}

static int IsdWriteMemChunk(IsdDevice *pDev, uint32_t dest, const void *buf, unsigned int size) {
	return IsdCmdSend_WriteMem(pDev, dest, buf, size);
}



// ----- Monitor routines

static void IsdConvertStatus(IsdMonStatus *pStatus, const IsdMonStatusBlock *pStatusBlock) {
	pStatus->proc = pStatusBlock->proc;
	pStatus->fiqEnter = pStatusBlock->fiqEnter;
	pStatus->cmdComplete = pStatusBlock->cmdComplete;
}

static int Isd_NitEmu_WaitMon_CmdComplete(IsdDevice *pDev) {
	IsdStartTimer(pDev);

	IsdMonStatusBlock monStatus;
	while (1) {
		int status = IsdCmdSend_WaitMonCmdComplete(pDev, &monStatus);
		if (!status) return 0;

		if (monStatus.cmdComplete) return 1;

		//check timeout state
		if (IsdCheckTimerExpired(pDev)) return 0;
	}
}

static int Isd_NitEmu_WaitMon_FiqEntry(IsdDevice *pDev) {
	IsdStartTimer(pDev);

	IsdMonStatusBlock monStatus;
	while (1) {
		int status = IsdCmdSend_WaitMonEntry(pDev, &monStatus);
		if (!status) return 0;

		if (monStatus.fiqEnter) return 1;

		//check timeout state
		if (IsdCheckTimerExpired(pDev)) return 0;
	}
}

static int Isd_NitEmu_MonEnter(IsdDevice *pDev) {
	if (!IsdRaiseFIQ(pDev)) return 0;

	int status = Isd_NitEmu_WaitMon_FiqEntry(pDev);

	if (!IsdLowerFIQ(pDev)) return 0;

	return status;
}

static int Isd_NitEmu_MonLeave(IsdDevice *pDev) {
	if (!Isd_NitEmu_WaitMon_CmdComplete(pDev)) return 0;

	if (!IsdCmdSend_FiqLeave(pDev)) return 0;

	if (!Isd_NitEmu_WaitMon_CmdComplete(pDev)) return 0;

	return 1;
}

static int Isd_NitEmu_MonGetStatus(IsdDevice *pDev, IsdMonStatus *pStatus) {
	IsdMonStatusBlock statusBlock;
	if (!IsdCmdSend_GetMonStatus(pDev, &statusBlock)) return 0;

	IsdConvertStatus(pStatus, &statusBlock);
	return 1;
}



// ----- General routines

static int Isd_NitEmu_GetSerial(IsdDevice *pDev, unsigned char *pSerial) {
	return IsdCmdSend_GetSerial(pDev, pSerial);
}

static int Isd_NitEmu_SelectProc(IsdDevice *pDev, IsdProc proc) {
	if (!IsdCmdSend_SelectProc(pDev, proc)) return 0;

	pDev->curproc = proc;
	return 1;
}



// ----- Memory transfer routines

static int IsdReadMem_Proc(IsdDevice *pDev, uint32_t addr, void *p, size_t size) {
	return IsdReadMemChunk(pDev, addr, p, size);
}

static int Isd_NitEmu_ReadMem(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	return IsdReadByFuncp(pDev, dest, src, size, IsdReadMem_Proc, ISD_READ_MEM_MAX);
}


static int IsdWriteMem_Proc(IsdDevice *pDev, uint32_t addr, const void *p, size_t size) {
	if (!IsdWriteMemChunk(pDev, addr, p, size)) return 0;

	if (!Isd_NitEmu_WaitMon_CmdComplete(pDev)) return 0;

	return 1;
}

static int Isd_NitEmu_WriteMem(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	return IsdWriteByFuncp(pDev, dest, src, size, IsdWriteMem_Proc, ISD_WRITE_MEM_MAX);
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

static int Isd_NitEmu_ReadAGB(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	return IsdReadByFuncp(pDev, dest, src, size, IsdReadAGB_Proc, ISD_READ_AGB_MAX);
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

static int Isd_NitEmu_WriteAGB(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	return IsdWriteByFuncp(pDev, dest, src, size, IsdWriteAGB_Proc, ISD_WRITE_AGB_MAX);
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

static int Isd_NitEmu_ReadEmulation(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	return IsdReadByFuncp(pDev, dest, src, size, IsdReadEmulation_Proc, ISD_READ_SDRAM_MAX);
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

static int Isd_NitEmu_WriteEmulation(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	return IsdWriteByFuncp(pDev, dest, src, size, IsdWriteEmulation_Proc, ISD_WRITE_SDRAM_MAX);
}
