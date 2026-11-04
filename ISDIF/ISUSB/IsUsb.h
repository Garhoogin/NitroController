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
#pragma once
#include <Windows.h>
#include <stdint.h>


// -----------------------------------------------------------------------------------------------
// The common definitions for the Intelligent Systems USB driver.
//
// Driver services the devices:
//   0100 : IS-CGB-EMULATOR
//   0201 : FLASH GANG WRITER
//   0202 : IS-AGB-CAPTURE
//   0300 : IS-DOL-VIEWER
//   0400 : IS-NITRO-EMULATOR
//   0401 : IS-NITRO-UIC
//   0402 : IS-NITRO-WRITER
//   0403 : IS-NITRO-CAPTURE
//   0404 : IS-NITRO-EMULATOR
//   0500 : IS-TWL-DEBUGGER
//   0502 : IS-TWL-CAPTURE
//   0600 : IS-CTR-BOX
//   0601 : IS-SPR-BOX (CAPTURE)
//   0602 : IS-SPR-BOX (DEBUGGER)
//   0700 : IS-SNAKE-BOX (DEBUGGER)
//   0701 : IS-SNAKE-BOX (CAPTURE/TESTER)
// -----------------------------------------------------------------------------------------------

#define ISUSB_VENDOR_INTSYS 0x0F6E



typedef struct ISUsbDevice_ {
	HANDLE hDev;
	HANDLE hRead;
	HANDLE hWrite;
} ISUsbDevice;



typedef struct ISUsbDeviceListEntry_ {
	wchar_t *path;
	uint16_t vid;
	uint16_t pid;
} ISUsbDeviceListEntry;

typedef struct ISUsbDeviceList_ {
	ISUsbDeviceListEntry *devs;
	unsigned int          nDev;
} ISUsbDeviceList;




// ----- Driver structures

typedef struct ISUsbDeviceDescriptor_ {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t bcdUSB;
	uint8_t  bDeviceClass;
	uint8_t  bDeviceSubClass;
	uint8_t  bDeviceProtocol;
	uint8_t  bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t  iManufacturer;
	uint8_t  iProduct;
	uint8_t  iSerialNumber;
	uint8_t  bNumConfigurations;
} ISUsbDeviceDescriptor;



int ISUsbDeviceOpen(ISUsbDevice *pDev, const wchar_t *path);
int ISUsbDeviceOpenDrvOnly(ISUsbDevice *pDev, const wchar_t *path);
int ISUsbDeviceClose(ISUsbDevice *pDev);
int ISUsbDeviceWrite(ISUsbDevice *pDev, const void *buf, unsigned int size);
int ISUsbDeviceRead(ISUsbDevice *pDev, void *buf, unsigned int size);
int ISUsbDeviceGetDeviceDescriptor(ISUsbDevice *pDev, ISUsbDeviceDescriptor *pDesc);

int ISUsbFindDevices(ISUsbDeviceList *pList, uint16_t vid, uint16_t pid);
void ISUsbDeviceListFree(ISUsbDeviceList *pList);
