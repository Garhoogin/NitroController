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
#include <Windows.h>

#include "isd.h"
#include "IsdPrivate.h"
#include "TwlDbg.h"


// ----- virtual table

static int Isd_TwlDbg_WaitMon_CmdComplete(IsdDevice *pDev);
static int Isd_TwlDbg_WaitMon_FiqEntry(IsdDevice *pDev);
static int Isd_TwlDbg_MonEnter(IsdDevice *pDev);
static int Isd_TwlDbg_MonLeave(IsdDevice *pDev);
static int Isd_TwlDbg_MonGetStatus(IsdDevice *pDev, IsdMonStatus *pStatus);
static int Isd_TwlDbg_GetSerial(IsdDevice *pDev, unsigned char *pSerial);
static int Isd_TwlDbg_SelectProc(IsdDevice *pDev, IsdProc proc);
static int Isd_TwlDbg_ReadMem(IsdDevice *pDev, void *dest, uint32_t src, size_t size);
static int Isd_TwlDbg_WriteMem(IsdDevice *pDev, uint32_t dest, const void *src, size_t size);
static int Isd_TwlDbg_ReadAGB(IsdDevice *pDev, void *dest, uint32_t src, size_t size);
static int Isd_TwlDbg_WriteAGB(IsdDevice *pDev, uint32_t dest, const void *src, size_t size);
static int Isd_TwlDbg_ReadEmulation(IsdDevice *pDev, void *dest, uint32_t src, size_t size);
static int Isd_TwlDbg_WriteEmulation(IsdDevice *pDev, uint32_t dest, const void *src, size_t size);

static const IsdDeviceVirtual sVtbl = {
	Isd_TwlDbg_WaitMon_CmdComplete,
	Isd_TwlDbg_WaitMon_FiqEntry,
	Isd_TwlDbg_MonEnter,
	Isd_TwlDbg_MonLeave,
	Isd_TwlDbg_MonGetStatus,
	Isd_TwlDbg_GetSerial,
	Isd_TwlDbg_SelectProc,
	Isd_TwlDbg_ReadMem,
	Isd_TwlDbg_WriteMem,
	Isd_TwlDbg_ReadAGB,
	Isd_TwlDbg_WriteAGB,
	Isd_TwlDbg_ReadEmulation,
	Isd_TwlDbg_WriteEmulation
};





static wchar_t *PathCat(const wchar_t *str1, const wchar_t *str2) {
	size_t len1 = wcslen(str1), len2 = wcslen(str2);
	wchar_t *buf = (wchar_t *) malloc((len1 + len2 + 1) * sizeof(wchar_t));
	if (buf == NULL) return NULL;

	memcpy(buf, str1, len1 * sizeof(wchar_t));
	memcpy(buf + len1, str2, (len2 + 1) * sizeof(wchar_t));
	return buf;
}

IsdDevice *IsdOpen_TwlDbgInternal(const wchar_t *devPath, IsdShareMode shareMode) {
	IsdDevice *pDev = (IsdDevice *) calloc(1, sizeof(IsdDevice));
	if (pDev == NULL) goto Error;

	pDev->pVtbl = &sVtbl;
	pDev->status = ISD_STATUS_OK;
	pDev->shareMode = shareMode;
	pDev->hMutex = NULL;

	int status = ISUsbDeviceOpen(&pDev->usb, devPath);
	if (!status) goto Error;

	//shared mutex
	wchar_t *mutexPath = PathCat(L"Global@ISU_", devPath + 4);
	if (mutexPath != NULL) pDev->hMutex = CreateMutex(NULL, FALSE, mutexPath);
	free(mutexPath);

	//checking our device handles opened correctly
	if (pDev->hMutex == NULL) goto Error;

	return pDev;

Error:
	if (pDev != NULL) IsdClose(pDev);
	return NULL;
}

IsdDevice *IsdOpen_TwlDbg(const wchar_t *devPath, IsdShareMode shareMode) {
	if (devPath != NULL) return IsdOpen_TwlDbgInternal(devPath, shareMode);

	//find device
	ISUsbDeviceList list;
	if (!ISUsbFindDevices(&list, ISUSB_VENDOR_INTSYS, ISD_PID_IS_TWL_DEBUGGER)) return NULL;

	IsdDevice *pDev = NULL;

	//use first device in the list
	//TODO: a more sophisticated interface to select the device
	if (list.nDev > 0) {
		pDev = IsdOpen_TwlDbgInternal(list.devs[0].path, shareMode);
	}

	ISUsbDeviceListFree(&list);
	return pDev;
}



// ----- Monitor routines

static int Isd_TwlDbg_WaitMon_CmdComplete(IsdDevice *pDev) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_WaitMon_FiqEntry(IsdDevice *pDev) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_MonEnter(IsdDevice *pDev) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_MonLeave(IsdDevice *pDev) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_MonGetStatus(IsdDevice *pDev, IsdMonStatus *pStatus) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}



// ----- General routines

static int Isd_TwlDbg_GetSerial(IsdDevice *pDev, unsigned char *pSerial) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_SelectProc(IsdDevice *pDev, IsdProc proc) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}



// ----- Memory transfer routines

static int Isd_TwlDbg_ReadMem(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_WriteMem(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_ReadAGB(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_WriteAGB(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_ReadEmulation(IsdDevice *pDev, void *dest, uint32_t src, size_t size) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}

static int Isd_TwlDbg_WriteEmulation(IsdDevice *pDev, uint32_t dest, const void *src, size_t size) {
	//TODO
	pDev->status = ISD_STATUS_UNSUPPORTED;
	return 0;
}
