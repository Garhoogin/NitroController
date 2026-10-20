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


// ----- command code

#define ISDCMD_CMDCODE_MASK    0x0000007F  // command code mask
#define ISDCMD_TARGET_MASK     0x00000080  // Command target mask
#define ISDCMD_RDTYPE_MASK     0x000F0000  // operation type mask
#define ISDCMD_LOC_MASK        0xFF000000  // The type of location accessed
#define ISDCMD_TARGET_NTR      0x00000080  // Command target -> NITRO
#define ISDCMD_TARGET_NEC      0x00000000  // Command target -> NEC
#define ISDCMD_TYPE_WR         0x00000000  // Command type -> write
#define ISDCMD_TYPE_RD         0x00010000  // Command type -> read
#define ISDCMD_TYPE_UD         0x00020000  // Command type -> other (?)
#define ISDCMD_FLAG_NOSTAT     0x00100000  // Setting this rejects the command return code
#define ISDCMD_LOC_CMD         0x00000000  // not a real location, used to send commands
#define ISDCMD_LOC_SDRAM       0x01000000  // access emulation memory
#define ISDCMD_LOC_AGB         0x02000000  // access AGB SRAM
#define ISDCMD_LOC_3           0x03000000  // ?
#define ISDCMD_LOC_AGBBUS1     0x04000000  // ?
#define ISDCMD_LOC_AGBBUS2     0x05000000  // ?

#define ISDCMD_READ_SDRAM      (ISDCMD_LOC_SDRAM | ISDCMD_TYPE_RD)                               // read emulation memory
#define ISDCMD_READ_AGB        (ISDCMD_LOC_AGB   | ISDCMD_TYPE_RD)                               // read AGB SRAM

#define ISDCMD_WRITE_SDRAM     (ISDCMD_LOC_SDRAM | ISDCMD_TYPE_WR)                               // write emulation memory
#define ISDCMD_WRITE_AGB       (ISDCMD_LOC_AGB   | ISDCMD_TYPE_WR)                               // write AGB SRAM

#define ISDCMD_GET_SERIAL      (ISDCMD_TARGET_NEC | ISDCMD_TYPE_RD | ISDCMD_FLAG_NOSTAT | 0x13)  // get device serial

#define ISDCMD_HARDWARE_RESET  (ISDCMD_TARGET_NTR | ISDCMD_TYPE_WR | ISDCMD_FLAG_NOSTAT | 0x01)  // do hardware reset
#define ISDCMD_START_READ_MEM  (ISDCMD_TARGET_NTR | ISDCMD_TYPE_WR | ISDCMD_FLAG_NOSTAT | 0x03)  // read memory from target processor
#define ISDCMD_WRITE_MEM       (ISDCMD_TARGET_NTR | ISDCMD_TYPE_WR | ISDCMD_FLAG_NOSTAT | 0x04)  // write memory to target processor
#define ISDCMD_FIQ_LEAVE       (ISDCMD_TARGET_NTR | ISDCMD_TYPE_WR | ISDCMD_FLAG_NOSTAT | 0x05)  // allow FIQ to return
#define ISDCMD_SELECT_PROC     (ISDCMD_TARGET_NTR | ISDCMD_TYPE_WR | ISDCMD_FLAG_NOSTAT | 0x0B)  // set current processor
#define ISDCMD_START_READ_REG  (ISDCMD_TARGET_NTR | ISDCMD_TYPE_WR | ISDCMD_FLAG_NOSTAT | 0x20)  // read register from target processor
#define ISDCMD_SET_FIQ         (ISDCMD_TARGET_NTR | ISDCMD_TYPE_WR | ISDCMD_FLAG_NOSTAT | 0x2A)  // set FIQ raise/lower

#define ISDCMD_READ_MEM_RESULT (ISDCMD_TARGET_NTR | ISDCMD_TYPE_RD | ISDCMD_FLAG_NOSTAT | 0x01)  // get read memory result
#define ISDCMD_GET_PRINT_MAINP (ISDCMD_TARGET_NTR | ISDCMD_TYPE_RD | ISDCMD_FLAG_NOSTAT | 0x08)  // get print data ARM9
#define ISDCMD_GET_PRINT_SUBP  (ISDCMD_TARGET_NTR | ISDCMD_TYPE_RD | ISDCMD_FLAG_NOSTAT | 0x09)  // get print data ARM7
#define NECCDM_READ_REG_RESULT (ISDCMD_TARGET_NTR | ISDCMD_TYPE_RD | ISDCMD_FLAG_NOSTAT | 0x0C)  // read regs read result
#define ISDCMD_WAITMON_FIQ     (ISDCMD_TARGET_NTR | ISDCMD_TYPE_RD | ISDCMD_FLAG_NOSTAT | 0x27)  // wait for FIQ entry
#define ISDCMD_WAITMON_CMD     (ISDCMD_TARGET_NTR | ISDCMD_TYPE_RD | ISDCMD_FLAG_NOSTAT | 0x28)  // wait for command complete




// ----- command struct

typedef struct IsdCommandHeader_ {
	uint32_t header;  // this is really a u16 ; u8 ; u8, treating as u32 for convenience
	uint32_t addr;
	uint32_t size;
	uint32_t fieldC;
} IsdCommandHeader;

typedef struct IsdCommandResult_ {
	uint32_t retVal;                // return value of the NEC command
	uint32_t field4;                // filled zero
	uint32_t field8;                // filled zero
	uint32_t fieldC;                // filled zero
} IsdCommandResult;

typedef struct IsdMonStatus_ {
	unsigned char proc;             // indicates which processor (0=main, 1=sub)
	unsigned char field1;
	unsigned char cmdComplete;      // indicates the command is complete
	unsigned char fiqEnter;         // indicates the FIQ handler is ready
	uint32_t monStatus;             // ? (0=in process, 1=complete, 2=unknown?)
} IsdMonStatus;



typedef struct IsdCmdSetProcBody_ {
	uint8_t cmdid;
	uint16_t proc;
} IsdCmdSetProcBody;

typedef struct IsdCmdSetFiqBody_ {
	uint8_t cmdid;
	uint16_t flag;
} IsdCmdSetFiqBody;

typedef struct IsdCmdReadMemBody_ {
	uint8_t cmdid;
	uint32_t addr;
	uint32_t size;
} IsdCmdReadMemBody;

typedef struct IsdCmdWriteMemBody_ {
	uint8_t cmdid;
	uint32_t addr;
	uint16_t length;
	uint16_t field1A;
	unsigned char data[];
} IsdCmdWriteMemBody;



typedef struct IsdCmdWrite_ {
	IsdCommandHeader header;
	unsigned char    data[];
} IsdCmdWrite;

typedef struct IsdCmdSetProc_ {
	IsdCommandHeader   header;
	IsdCmdSetProcBody  body;
} IsdCmdSetProc;

typedef struct IsdCmdSetFiq_ {
	IsdCommandHeader   header;
	IsdCmdSetFiqBody   body;
} IsdCmdSetFiq;

typedef struct IsdCmdReadMem_ {
	IsdCommandHeader   header;
	IsdCmdReadMemBody  body;
} IsdCmdReadMem;

typedef struct IsdCmdWriteMem_ {
	IsdCommandHeader   header;
	IsdCmdWriteMemBody body;
} IsdCmdWriteMem;




#define ISD_INIT_CMD_HDR(hdr, id) do { \
	(hdr).header = (id);               \
	(hdr).addr   = 0;                  \
	(hdr).size   = 0;                  \
	(hdr).fieldC = 0;                  \
} while (0)

#define ISD_INIT_CMD(cmd, id) do {             \
	(cmd).header.header = (id);                \
	(cmd).header.addr   = 0;                   \
	(cmd).header.size   = sizeof((cmd).body);  \
	(cmd).header.fieldC = 0;                   \
	(cmd).body.cmdid    = (uint8_t) (id);      \
} while (0)



