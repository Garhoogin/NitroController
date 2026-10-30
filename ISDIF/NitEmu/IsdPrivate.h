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


// ----- limits

#define ISD_READ_MAX           0x10000  // max size read at once
#define ISD_WRITE_MAX          0x10000  // max size write at once

#define ISD_READ_MEM_MAX       0x08000  // TOOD: verify
#define ISD_WRITE_MEM_MAX      0x02000  // TODO: verify
#define ISD_READ_AGB_MAX         0x200  // TODO: verify
#define ISD_WRITE_AGB_MAX        0x200  // TODO: verify
#define ISD_READ_SDRAM_MAX       0x200  // TODO: verify
#define ISD_WRITE_SDRAM_MAX      0x200  // TODO: verify

#define ISD_DEFAULT_LOCK_TIMEOUT  2000  // default lock timeout
#define ISD_DEFAULT_MON_TIMEOUT   2000  // default monitor communication timeout



// ----- Driver constants

#define ISD_CTL(n) CTL_CODE(FILE_DEVICE_UNKNOWN, n, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define ISD_IOCTL_GET_CONFIG_DESC    ISD_CTL(0x0000)  // Get the USB configuration descriptor
#define ISD_IOCTL_RESET_PORT         ISD_CTL(0x0001)  // Resets the USB port if port connected but not enabled. No in, no out.
#define ISD_IOCTL_RESET_PIPE         ISD_CTL(0x0002)  // Resets the pipe. No in, no out.
#define ISD_IOCTL_GET_DEVICE_DESC    ISD_CTL(0x0003)  // Returns the USB_DEVICE_DESCRIPTOR
#define ISD_IOCTL_GET_WDM_VERSION    ISD_CTL(0x0005)  // Returns WDM version (3: 1.31+. 1: older. 2 and 0 never returned?). No in, ULONG out.
#define ISD_IOCTL_6                  ISD_CTL(0x0006)  // Returns ULONG 0x00020002.
#define ISD_IOCTL_SUBMIT_URB         ISD_CTL(0x0007)  // Submits a URB. URB in, no out.
#define ISD_IOCTL_SUBMIT_URB_EX      ISD_CTL(0x0008)  // Submits a URB. URB in, response out.
#define ISD_IOCTL_GET_PORT_STATUS    ISD_CTL(0x000A)  // Returns USB port status. No in, ULONG out.


#define ISD_PID   0x0404
#define ISD_VID   0x0F6E

DEFINE_GUID(ISD_GUID, 0xB78D7ADA, 0xDDF4, 0x418F, 0x8C, 0x7C, 0x4A, 0x0C8, 0x80, 0x30, 0xF5, 0x42);



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

struct IsdDevice_ {
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
	IsdMonStatus monStatus;         // status of monitor program
};


// ----- private routines on the device

int IsdWrite(IsdDevice *pDev, const void *data, unsigned int size);
int IsdRead(IsdDevice *pDev, void *data, unsigned int size);

int IsdSendWriteCommandEx(IsdDevice *pDev, const void *data, IsdCommandResult *pResult);
int IsdSendReadCommandEx(IsdDevice *pDev, const void *data, void *resp, IsdCommandResult *pResult);
int IsdSendWriteCommand(IsdDevice *pDev, const void *data);
int IsdSendReadCommand(IsdDevice *pDev, const void *data, void *resp);

int IsdGetSerialInternal(IsdDevice *pDev, unsigned char *pSerial);

int IsdCmdSend_ReadAGB(IsdDevice *pDev, uint32_t addr, uint32_t size, void *buf, IsdCommandResult *pResult);
int IsdCmdSend_WriteAGB(IsdDevice *pDev, uint32_t addr, const void *buf, unsigned int size, IsdCommandResult *pResult);
int IsdCmdSend_ReadSDRAM(IsdDevice *pDev, uint32_t addr, uint32_t size, void *buf, IsdCommandResult *pResult);
int IsdCmdSend_WriteSDRAM(IsdDevice *pDev, uint32_t addr, const void *buf, unsigned int size, IsdCommandResult *pResult);
int IsdCmdSend_GetSerial(IsdDevice *pDev, unsigned char *pSerial);
int IsdCmdSend_SelectProc(IsdDevice *pDev, IsdProc proc);
int IsdCmdSend_SetFIQ(IsdDevice *pDev, uint16_t flag);
int IsdCmdSend_GetMonStatus(IsdDevice *pDev, IsdMonStatus *pStatus);
int IsdCmdSend_WaitMonEntry(IsdDevice *pDev, IsdMonStatus *pStatus);
int IsdCmdSend_WaitMonCmdComplete(IsdDevice *pDev, IsdMonStatus *pStatus);
int IsdCmdSend_FiqLeave(IsdDevice *pDev);
int IsdCmdSend_WriteMem(IsdDevice *pDev, uint32_t dest, const void *buf, unsigned int size);
int IsdCmdSend_StartReadMem(IsdDevice *pDev, uint32_t addr, unsigned int size);
int IsdCmdSend_GetReadMemResult(IsdDevice *pDev, void *dest, unsigned int size);
int IsdCmdSend_Configure(IsdDevice *pDev, int idx, const void *data, unsigned int size);

