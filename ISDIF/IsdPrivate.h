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


#ifdef _DEBUG
#define ISD_ASSERT(x)   do { if (!(x)) __debugbreak(); } while (0)
#else
#define ISD_ASSERT(x)   __assume(x)
#endif


#define ISD_READ_MAX           0x10000  // max size read at once
#define ISD_WRITE_MAX          0x10000  // max size write at once


// ----- Defaults

#define ISD_DEFAULT_LOCK_TIMEOUT  2000  // default lock timeout
#define ISD_DEFAULT_MON_TIMEOUT   2000  // default monitor communication timeout



// ----- Driver structures

typedef struct UsbDeviceDescriptor_ {
	UCHAR bLength;
	UCHAR bDescriptorType;
	USHORT bcdUSB;
	UCHAR bDeviceClass;
	UCHAR bDeviceSubClass;
	UCHAR bDeviceProtocol;
	UCHAR bMaxPacketSize0;
	USHORT idVendor;
	USHORT idProduct;
	USHORT bcdDevice;
	UCHAR iManufacturer;
	UCHAR iProduct;
	UCHAR iSerialNumber;
	UCHAR bNumConfigurations;
} UsbDeviceDescriptor;

typedef struct IsdDeviceVirtual_ {
	int (*WaitMon_CmdComplete) (IsdDevice *pDev);
	int (*WaitMon_FiqEntry) (IsdDevice *pDev);
	int (*MonEnter) (IsdDevice *pDev);
	int (*MonLeave) (IsdDevice *pDev);
	int (*MonGetStatus) (IsdDevice *pDev, IsdMonStatus *pStatus);

	int (*GetSerial) (IsdDevice *pDev, unsigned char *pSerial);
	int (*SelectProc) (IsdDevice *pDev, IsdProc proc);

	int (*ReadMem) (IsdDevice *pDev, void *dest, uint32_t src, size_t size);
	int (*WriteMem) (IsdDevice *pDev, uint32_t dest, const void *src, size_t size);
	int (*ReadAGB) (IsdDevice *pDev, void *dest, uint32_t src, size_t size);
	int (*WriteAGB) (IsdDevice *pDev, uint32_t dest, const void *src, size_t size);
	int (*ReadEmulation) (IsdDevice *pDev, void *dest, uint32_t src, size_t size);
	int (*WriteEmulation) (IsdDevice *pDev, uint32_t dest, const void *src, size_t size);
} IsdDeviceVirtual;

struct IsdDevice_ {
	const IsdDeviceVirtual *pVtbl;  // virtual table

	HANDLE hDev;                    // handle to the device
	HANDLE hReadPipe;               // handle to the read pipe
	HANDLE hWritePipe;              // handle to the write pipe
	char serial[ISD_SERIAL_SIZE];   // device serial

	IsdShareMode shareMode;         // the device sharing mode
	HANDLE hMutex;                  // the mutex used as the basis for the lock
	unsigned int mutOwned;          // the mutex owned status
	unsigned int lockCount;         // the lock count

	unsigned int lockTimeout;       // lock acquire timeout
	LARGE_INTEGER timerStart;       // timeout timer start
	LARGE_INTEGER timerFreq;        // timer frequency
	LARGE_INTEGER timerTimeout;     // timeout duration

	IsdStatus status;               // current status
	IsdProc curproc;                // current processor

	int monStart;                   // if the monitor started
};


// ----- private routines on the device

int IsdGetSerialInternal(IsdDevice *pDev, unsigned char *pSerial);
void IsdStartTimer(IsdDevice *pDev);
int IsdCheckTimerExpired(IsdDevice *pDev);

int IsdWrite(IsdDevice *pDev, const void *data, unsigned int size);
int IsdRead(IsdDevice *pDev, void *data, unsigned int size);


// ----- Chunked transfer routines

typedef int (*IsdWriteRoutine) (IsdDevice *pDev, uint32_t addr, const void *p, size_t size);
typedef int (*IsdReadRoutine) (IsdDevice *pDev, uint32_t addr, void *p, size_t size);

int IsdWriteByFuncp(IsdDevice *pDev, uint32_t dest, const void *src, size_t size, IsdWriteRoutine funcp, size_t maxBlock);
int IsdReadByFuncp(IsdDevice *pDev, void *dest, uint32_t src, size_t size, IsdReadRoutine funcp, size_t maxBlock);

