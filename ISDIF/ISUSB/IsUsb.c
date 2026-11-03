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

#include "IsUsb.h"



static wchar_t *PathCat(const wchar_t *str1, const wchar_t *str2) {
	size_t len1 = wcslen(str1), len2 = wcslen(str2);
	wchar_t *buf = (wchar_t *) malloc((len1 + len2 + 1) * sizeof(wchar_t));
	if (buf == NULL) return NULL;

	memcpy(buf, str1, len1 * sizeof(wchar_t));
	memcpy(buf + len1, str2, (len2 + 1) * sizeof(wchar_t));
	return buf;
}

int ISUsbDeviceOpen(ISUsbDevice *pDev, const wchar_t *path) {
	pDev->hDev   = INVALID_HANDLE_VALUE;  // main device handle
	pDev->hWrite = INVALID_HANDLE_VALUE;  // write pipe
	pDev->hRead  = INVALID_HANDLE_VALUE;  // read pipe

	wchar_t *pipe00 = PathCat(path, L"\\Pipe00");
	wchar_t *pipe01 = PathCat(path, L"\\Pipe01");

	if (pipe00 != NULL && pipe01 != NULL) {
		//main device handle
		pDev->hDev = CreateFile(path, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		//write pipe end
		pDev->hWrite = CreateFile(pipe00, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

		//read pipe end
		pDev->hRead = CreateFile(pipe01, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	}
	free(pipe00);
	free(pipe01);

	if (pDev->hDev   == INVALID_HANDLE_VALUE) goto Error;
	if (pDev->hRead  == INVALID_HANDLE_VALUE) goto Error;
	if (pDev->hWrite == INVALID_HANDLE_VALUE) goto Error;

	return 1;

Error:
	ISUsbDeviceClose(pDev);
	return 0;
}

int ISUsbDeviceClose(ISUsbDevice *pDev) {
	if (pDev->hDev   != INVALID_HANDLE_VALUE) CloseHandle(pDev->hDev  );
	if (pDev->hRead  != INVALID_HANDLE_VALUE) CloseHandle(pDev->hRead );
	if (pDev->hWrite != INVALID_HANDLE_VALUE) CloseHandle(pDev->hWrite);

	//clear handle values
	pDev->hDev   = INVALID_HANDLE_VALUE;
	pDev->hRead  = INVALID_HANDLE_VALUE;
	pDev->hWrite = INVALID_HANDLE_VALUE;

	return 1;
}

int ISUsbDeviceWrite(ISUsbDevice *pDev, const void *buf, size_t size) {
	while (size > 0) {
		unsigned int nChunk = size;
		if (nChunk > ISUSB_WRITE_MAX) nChunk = ISUSB_WRITE_MAX;

		//can't use IO timeout, not supported by driver
		DWORD nWritten;
		BOOL res = WriteFile(pDev->hWrite, buf, nChunk, &nWritten, NULL);

		if (!res || nWritten == 0) {
			//transfer fail
			return 0;
		}

		size -= nWritten;
		buf = (const unsigned char *) buf + nWritten;
	}

	return 1;  // success
}

int ISUsbDeviceRead(ISUsbDevice *pDev, void *buf, size_t size) {
	while (size > 0) {
		unsigned int nChunk = size;
		if (nChunk > ISUSB_READ_MAX) nChunk = ISUSB_READ_MAX;

		//can't use IO timeout, not supported by driver
		DWORD nRead;
		BOOL res = ReadFile(pDev->hRead, buf, nChunk, &nRead, NULL);

		if (!res || nRead == 0) {
			//transfer fail
			return 0;
		}

		size -= nRead;
		buf = (unsigned char *) buf + nRead;
	}

	return 1;  // success
}

int ISUsbDeviceGetDeviceDescriptor(ISUsbDevice *pDev, ISUsbDeviceDescriptor *pDesc) {
	ISUsbDeviceDescriptor desc = { 0 };

	DWORD dwBytesReturned;
	BOOL b = DeviceIoControl(pDev->hDev,
		ISUSB_IOCTL_GET_DEVICE_DESCRIPTOR,
		&desc, sizeof(desc),
		&desc, sizeof(desc),
		&dwBytesReturned, NULL);
	if (!b || dwBytesReturned < sizeof(desc)) return 0;

	memcpy(pDesc, &desc, sizeof(desc));
	return 1;
}
