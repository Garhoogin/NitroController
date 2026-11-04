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


#define ISUSB_READ_MAX           0x10000  // max size read at once
#define ISUSB_WRITE_MAX          0x10000  // max size write at once



// ----- driver interface GUID

DEFINE_GUID(ISUSB_INTERFACE_GUID, 0xB78D7ADA, 0xDDF4, 0x418F, 0x8C, 0x7C, 0x4A, 0xC8, 0x80, 0x30, 0xF5, 0x42);



// ----- IO Control Codes

#define ISUSB_CTL(n) CTL_CODE(FILE_DEVICE_UNKNOWN, n, METHOD_BUFFERED, FILE_ANY_ACCESS)


// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_GET_CONFIG_DESCRIPTOR
// 
// Get the USB configuration descriptor.
//
// Input:
//   None
//
// Output:
//   The USB configuration descriptor.
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_GET_CONFIG_DESCRIPTOR                                        ISUSB_CTL(0x0000)

// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_RESET_DEVICE
// 
// Resets the USB port if port connected but not enabled.
//
// Input:
//   None
// 
// Output: 
//   None
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_RESET_DEVICE                                                 ISUSB_CTL(0x0001)

// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_RESET_PIPE
// 
// Resets the pipe.
//
// Input:
//   None
//
// Output:
//   None
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_RESET_PIPE                                                   ISUSB_CTL(0x0002)

// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_GET_DEVICE_DESCRIPTOR
// 
// Gets the USB device descriptor.
//
// Input:
//   None
//
// Output:
//   The USB device descriptor.
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_GET_DEVICE_DESCRIPTOR                                        ISUSB_CTL(0x0003)

// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_GET_WDM_VERSION
//
// Returns the driver supported WDM version.
//
// Input:
//   None
//
// Output:
//   ULONG indicating the supported WDM version.
//     3: 1.31 and above
//     1: older than 1.31
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_GET_WDM_VERSION                                              ISUSB_CTL(0x0005)

// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_GET_DRIVER_VERSION
// 
// Get the driver version? Not available for all versions.
//
// Input:
//   None
// 
// Output:
//   ULONG indicating the driver version?
//     00010002: older driver versions?
//     00020002: driver version 2.2 and newer?
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_GET_DRIVER_VERSION                                           ISUSB_CTL(0x0006)

// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_SUBMIT_URB
// 
// Submits a URB. Not available for all versions.
//
// Input:
//   URB request
//
// Output:
//   None
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_SUBMIT_URB                                                   ISUSB_CTL(0x0007)

// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_SUBMIT_URB_EX
// 
// Submits a URB. Not available for all versions.
//
// Input:
//   URB request.
//
// Output:
//   URB response.
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_SUBMIT_URB_EX                                                ISUSB_CTL(0x0008)

// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_9
// 
// ? Not available for all versions.
//
// Input:
//   ?
//
// Output:
//   ?
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_9                                                            ISUSB_CTL(0x0009)

// -----------------------------------------------------------------------------------------------
// Name: ISUSB_IOCTL_GET_PORT_STATUS
// 
// Returns USB port status. Not available for all versions.
//
// Input:
//   None
//
// Output:
//   USB port status, as ULONG.
// -----------------------------------------------------------------------------------------------
#define ISUSB_IOCTL_GET_PORT_STATUS                                              ISUSB_CTL(0x000A)
