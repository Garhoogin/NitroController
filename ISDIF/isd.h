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

#include <stdint.h>
#include <wchar.h>


#define ISD_SERIAL_SIZE   10  // size of the serial number in bytes


#ifdef ISD_DLL
#define ISD_LIB __declspec(dllexport)
#else // ISD_DLL
#define ISD_LIB __declspec(dllimport)
#endif


// -----------------------------------------------------------------------------------------------
// Name: enum IsdStatus
//
// Indicates the last error code on the device.
//
// Boolean values:
//   ISD_STATUS_OK               No error occurred.
//   ISD_STATUS_IRREGULAR        An error occurred that results in the library in an unusable
//                               state. The library should be terminated.
//   ISD_STATUS_BUSY             Exclusive access to the device could not be acquired. The
//                               device may be being used by another process. Try again or use a
//                               lesser access.
//   ISD_STATUS_NOMEM            The operation failed because of insufficient memory on the host
//                               device. This error is non-fatal.
//   ISD_STATUS_COMM_CLOSE       The operation failed because the communication with the device
//                               was closed. No further communication can be successful and the
//                               device should be closed.
//   ISD_STATUS_MON_TIMEOUT      The operation failed because the communication with the monitor
//                               program timed out. Ensure that the monitor program is running.
//                               Otherwise, a hardware reset may be required.
//   ISD_STATUS_FAILED           The operation failed on the NEC executing a request. 
// -----------------------------------------------------------------------------------------------
typedef enum IsdStatus_ {
	ISD_STATUS_OK,             // No error.
	ISD_STATUS_IRREGULAR,      // The state is irregular. The library should be terminated.
	ISD_STATUS_BUSY,           // Exclusive access could not be acquired. Try again or use a lesser access.
	ISD_STATUS_NOMEM,          // Insufficient memory to complete the operation. Recoverable error.
	ISD_STATUS_COMM_CLOSE,     // The communication is closed unexpectedly. Try reconnecting.
	ISD_STATUS_MON_TIMEOUT,    // The debug monitor was unresponsive. Should hardware reset.
	ISD_STATUS_FAILED,         // A command was not executed successfully. Recoverable error.
} IsdStatus;

// -----------------------------------------------------------------------------------------------
// Name: enum IsdProc
//
// The Nitro processor cores.
//
// Boolean values:
//   TS_PROC_MAIN                The DS ARM9 processor.
//   TS_PROC_SUB                 The DS ARM7 processor.
// -----------------------------------------------------------------------------------------------
typedef enum IsdProc_ {
	TS_PROC_MAIN,              // The ARM9 processor
	TS_PROC_SUB                // The ARM7 processor
} IsdProc;

// -----------------------------------------------------------------------------------------------
// Name: enum IsdShareMode
//
// The sharing modes used to access the device.
//
// Boolean values:
//   ISD_SHARE_MODE_EXCLUSIVE    The device is opened with exclusive access for the entire
//                               duration it is open.
//   ISD_SHARE_MODE_SHARED       The access to the device is shared. Exclusive access is still
//                               requested on individual operations (and per user call to the
//                               IsdLockAcquire routine).
//   ISD_SHARE_MODE_PASSIVE      Exclusive access is never acquired. There are no guarantees of
//                               correct operation while other processes use the device.
// -----------------------------------------------------------------------------------------------
typedef enum IsdShareMode_ {
	ISD_SHARE_MODE_EXCLUSIVE,  // has exclusive ownership of the device at all times
	ISD_SHARE_MODE_SHARED,     // only acquires exclusive access for atomic operations
	ISD_SHARE_MODE_PASSIVE     // never acquires exclusive access
} IsdShareMode;

typedef struct IsdDevice_ IsdDevice;



// -----------------------------------------------------------------------------------------------
// Name: IsdOpen
//
// Opens an IS-NITRO-EMULATOR. 
//
// Parameters:
//   devPath       The device path. Obtain this path from enumerating the device list. Leave NULL
//                 to use the default device enumeration.
//   shareMode     The device sharing mode.
//
// Returns:
//   A pointer to an IsdDevice on success, or NULL on failure.
// -----------------------------------------------------------------------------------------------
ISD_LIB IsdDevice *IsdOpen(
	const wchar_t *devPath,
	IsdShareMode   shareMode
);

// -----------------------------------------------------------------------------------------------
// Name: IsdClose
//
// Closes an IS-NITRO-EMULATOR. 
//
// Parameters:
//   pDev          The device handle.
// -----------------------------------------------------------------------------------------------
ISD_LIB void IsdClose(
	IsdDevice *pDev
);

// -----------------------------------------------------------------------------------------------
// Name: IsdClearLastError
//
// Clears the last error on the device. Calling IsdGetLastError will return ISD_STATUS_OK after
// the call until an error occurs.
//
// Parameters:
//   pDev          The device handle.
// -----------------------------------------------------------------------------------------------
ISD_LIB void IsdClearLastError(
	IsdDevice *pDev
);

// -----------------------------------------------------------------------------------------------
// Name: IsdGetLastError
//
// Gets the last error code on the device. Call this only after a failed operation. Successful
// operations do not clear the error code.
//
// Parameters:
//   pDev          The device handle.
//
// Returns:
//   The last error code.
// -----------------------------------------------------------------------------------------------
ISD_LIB IsdStatus IsdGetLastError(
	IsdDevice *pDev
);

// -----------------------------------------------------------------------------------------------
// Name: IsdSetTimeout
//
// Sets the timeout for communication with the monitor program. 
//
// Parameters:
//   pDev          The device handle.
//   ms            The timeout, in milliseconds. This may be zero to indicate no timeout.
// -----------------------------------------------------------------------------------------------
ISD_LIB void IsdSetTimeout(
	IsdDevice *pDev,
	int        ms
);

// -----------------------------------------------------------------------------------------------
// Name: IsdLockAcquire
//
// Acquires exclusive access to the device. This call will block until either exclusive access
// can be acquired, or the timeout expires. The lock may be acquired more than once at the same
// time. Only the initial lock acquire will block or can fail. Each call to this routine must
// be paired with a call to IsdLockRelease.
//
// Parameters:
//   pDev          The device handle.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdLockAcquire(
	IsdDevice *pDev
);

// -----------------------------------------------------------------------------------------------
// Name: IsdLockRelease
//
// Releases exclusive access to the device. If the lock was acquired multiple times, this routine
// only releases exclusive access once the lock count is decremented to zero.
//
// Parameters:
//   pDev          The device handle.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdLockRelease(
	IsdDevice *pDev
);

// -----------------------------------------------------------------------------------------------
// Name: IsdSetLockTimeout
//
// Sets the timeout for acquiring exclusive access. This may be zero for no timeout.
//
// Parameters:
//   pDev          The device handle.
//   ms            The timeout, in milliseconds.
// -----------------------------------------------------------------------------------------------
ISD_LIB void IsdSetLockTimeout(
	IsdDevice   *pDev,
	unsigned int ms
);

// -----------------------------------------------------------------------------------------------
// Name: IsdMonEnter
//
// Trigger the monitor program to enter. This must be successfully called before running any
// functions that communicate with the monitor program. This routine may fail if the monitor
// program is not running, or the processor could otherwise not be interrupted (e.g. the FIQ is
// disabled).
//
// This routine will halt the selected processor until a later call to IsdMonLeave.
//
// Parameters:
//   pDev          The device handle.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdMonEnter(
	IsdDevice *pDev
);

// -----------------------------------------------------------------------------------------------
// Name: IsdMonLeave
//
// Triggers the monitor program to exit. This resumes the normal execution of the processor.
//
// Parameters:
//   pDev          The device handle.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdMonLeave(
	IsdDevice *pDev
);

// -----------------------------------------------------------------------------------------------
// Name: IsdMonRunning
//
// Checks if the monitor program was started. This will return zero if the running program was
// not loaded with the monitor program. This function may return 1 in cases where the monitor was
// not started. 
//
// Parameters:
//   pDev          The device handle.
//
// Returns:
//   1 if the monitor program was started, 0 otherwise.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdMonRunning(
	IsdDevice *pDev
);

// -----------------------------------------------------------------------------------------------
// Name: IsdGetSerial
//
// Gets the device serial number. 
//
// Parameters:
//   pDev          The device handle.
//   buf           The buffer receiving the serial number. This should be large enough to fit a
//                 char array of ISD_SERIAL_SIZE elements.
// -----------------------------------------------------------------------------------------------
ISD_LIB void IsdGetSerial(
	IsdDevice *pDev,
	char      *buf
);

// -----------------------------------------------------------------------------------------------
// Name: IsdSelectProc
//
// Sets the active processor. Operations on a processor will happen on the specificed processor
// following the successful call to this function.
//
// Parameters:
//   pDev          The device handle.
//   proc          The selected processor.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdSelectProc(
	IsdDevice *pDev,
	IsdProc    proc
);

// -----------------------------------------------------------------------------------------------
// Name: IsdReadMem
//
// Reads memory from the address space of the selected processor. The monitor must be entered
// before executing this function.
//
// Parameters:
//   pDev          The device handle.
//   dest          The destination buffer.
//   src           The source address.
//   size          The size of memory requested.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdReadMem(
	IsdDevice *pDev,
	void      *dest,
	uint32_t   src,
	size_t     size
);

// -----------------------------------------------------------------------------------------------
// Name: IsdWriteMem
//
// Writes to memory in the address space of the selected processor. The monitor must be entered
// before executing this function.
//
// Parameters:
//   pDev          The device handle.
//   dest          The destination address.
//   src           The source buffer.
//   size          The size of memory written.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdWriteMem(
	IsdDevice  *pDev,
	uint32_t    dest,
	const void *src,
	size_t      size
);

// -----------------------------------------------------------------------------------------------
// Name: IsdReadAGB
//
// Reads memory from the AGB shared area.
//
// Parameters:
//   pDev          The device handle.
//   dest          The destination buffer.
//   src           The source address.
//   size          The size of memory requested.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdReadAGB(
	IsdDevice *pDev,
	void      *dest,
	uint32_t   src,
	size_t     size
);

// -----------------------------------------------------------------------------------------------
// Name: IsdWriteAGB
//
// Writes memory to the shared AGB area.
//
// Parameters:
//   pDev          The device handle.
//   dest          The destination address.
//   src           The source buffer.
//   size          The size of memory written.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdWriteAGB(
	IsdDevice  *pDev,
	uint32_t    dest,
	const void *src,
	size_t      size
);

// -----------------------------------------------------------------------------------------------
// Name: IsdReadEmulation
//
// Reads from the emulation memory.
//
// Parameters:
//   pDev          The device handle.
//   dest          The destination buffer.
//   src           The source address.
//   size          The size of memory requested.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdReadEmulation(
	IsdDevice *pDev,
	void      *dest,
	uint32_t   src,
	size_t     size
);

// -----------------------------------------------------------------------------------------------
// Name: IsdWriteEmulation
//
// Writes to the emulation memory.
//
// Parameters:
//   pDev          The device handle.
//   dest          The destination address.
//   src           The source buffer.
//   size          The size of memory written.
//
// Returns:
//   Zero on failure, or nonzero on success.
// -----------------------------------------------------------------------------------------------
ISD_LIB int IsdWriteEmulation(
	IsdDevice  *pDev,
	uint32_t    dest,
	const void *src,
	size_t      size
);
