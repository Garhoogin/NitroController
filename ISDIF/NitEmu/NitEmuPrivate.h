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

#define ISD_READ_MEM_MAX       0x08000  // TOOD: verify
#define ISD_WRITE_MEM_MAX      0x02000  // TODO: verify
#define ISD_READ_AGB_MAX         0x200  // TODO: verify
#define ISD_WRITE_AGB_MAX        0x200  // TODO: verify
#define ISD_READ_SDRAM_MAX       0x200  // TODO: verify
#define ISD_WRITE_SDRAM_MAX      0x200  // TODO: verify



// ----- IO Control Codes

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


// ----- Device info

DEFINE_GUID(ISD_GUID, 0xB78D7ADA, 0xDDF4, 0x418F, 0x8C, 0x7C, 0x4A, 0x0C8, 0x80, 0x30, 0xF5, 0x42);

#define ISD_PID   0x0404
#define ISD_VID   0x0F6E

