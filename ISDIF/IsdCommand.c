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
#include "isd.h"
#include "isd_cmd.h"
#include "IsdPrivate.h"

#include <Windows.h>



static int IsdReadCommandResult(IsdDevice *pDev, IsdCommandResult *pResult) {
	//if the null pointer is passed, we redirect to a dummy buffer
	IsdCommandResult dummy;
	if (pResult == NULL) pResult = &dummy;

	return IsdRead(pDev, pResult, sizeof(IsdCommandResult));
}

int IsdSendWriteCommandEx(IsdDevice *pDev, const void *data, IsdCommandResult *pResult) {
	const IsdCommandHeader *hdr = (const IsdCommandHeader *) data;

	//write commands include the size of data in the header
	if (!IsdWrite(pDev, data, sizeof(IsdCommandHeader) + hdr->size)) return 0;

	//if the status is requested, read it.
	if (!(hdr->header & ISDCMD_FLAG_NOSTAT)) {
		if (!IsdReadCommandResult(pDev, pResult)) return 0;
	}

	return 1;
}

int IsdSendReadCommandEx(IsdDevice *pDev, const void *data, void *resp, IsdCommandResult *pResult) {
	const IsdCommandHeader *hdr = (const IsdCommandHeader *) data;

	//read commands are fixed-length, do not include more than the header size
	if (!IsdWrite(pDev, data, sizeof(IsdCommandHeader))) return 0;
	
	//the header size indicates response length to receive
	if (!IsdRead(pDev, resp, hdr->size)) return 0;

	//if the status is requested, read it.
	if (!(hdr->header & ISDCMD_FLAG_NOSTAT)) {
		if (!IsdReadCommandResult(pDev, pResult)) return 0;
	}

	return 1;
}

int IsdSendWriteCommand(IsdDevice *pDev, const void *data) {
	return IsdSendWriteCommandEx(pDev, data, NULL);
}

int IsdSendReadCommand(IsdDevice *pDev, const void *data, void *resp) {
	return IsdSendReadCommandEx(pDev, data, resp, NULL);
}



// ----- command procedures

int IsdCmdSend_ReadAGB(IsdDevice *pDev, uint32_t addr, uint32_t size, void *buf, IsdCommandResult *pResult) {
	IsdCommandHeader cmd = { 0 };
	ISD_INIT_CMD_HDR(cmd, ISDCMD_READ_AGB);

	cmd.addr = addr;
	cmd.size = size;
	return IsdSendReadCommandEx(pDev, &cmd, buf, pResult);
}

int IsdCmdSend_WriteAGB(IsdDevice *pDev, uint32_t addr, const void *buf, unsigned int size, IsdCommandResult *pResult) {
	IsdCmdWrite *cmd = (IsdCmdWrite *) calloc(1, sizeof(IsdCmdWrite) + size);
	if (cmd == NULL) {
		pDev->status = ISD_STATUS_NOMEM;
		return 0;
	}

	ISD_INIT_CMD_HDR(cmd->header, ISDCMD_WRITE_AGB);

	cmd->header.addr = addr;
	cmd->header.size = size;
	memcpy(cmd->data, buf, size);
	int status = IsdSendWriteCommandEx(pDev, cmd, pResult);

	free(cmd);
	return status;
}

int IsdCmdSend_ReadSDRAM(IsdDevice *pDev, uint32_t addr, uint32_t size, void *buf, IsdCommandResult *pResult) {
	IsdCommandHeader cmd = { 0 };
	ISD_INIT_CMD_HDR(cmd, ISDCMD_READ_SDRAM);

	cmd.addr = addr;
	cmd.size = size;
	return IsdSendReadCommandEx(pDev, &cmd, buf, pResult);
}

int IsdCmdSend_WriteSDRAM(IsdDevice *pDev, uint32_t addr, const void *buf, unsigned int size, IsdCommandResult *pResult) {
	IsdCmdWrite *cmd = (IsdCmdWrite *) calloc(1, sizeof(IsdCmdWrite) + size);
	if (cmd == NULL) {
		pDev->status = ISD_STATUS_NOMEM;
		return 0;
	}

	ISD_INIT_CMD_HDR(cmd->header, ISDCMD_WRITE_SDRAM);

	cmd->header.addr = addr;
	cmd->header.size = size;
	memcpy(cmd->data, buf, size);
	int status = IsdSendWriteCommandEx(pDev, cmd, pResult);

	free(cmd);
	return status;
}

int IsdCmdSend_GetSerial(IsdDevice *pDev, unsigned char *pSerial) {
	IsdCommandHeader cmd = { 0 };
	ISD_INIT_CMD_HDR(cmd, ISDCMD_GET_SERIAL);

	cmd.size = ISD_SERIAL_SIZE;
	return IsdSendReadCommand(pDev, &cmd, pSerial);
}

int IsdCmdSend_SelectProc(IsdDevice *pDev, IsdProc proc) {
	IsdCmdSetProc cmd = { 0 };
	ISD_INIT_CMD(cmd, ISDCMD_SELECT_PROC);

	cmd.body.proc = (uint16_t) proc;
	return IsdSendWriteCommand(pDev, &cmd);
}

int IsdCmdSend_SetFIQ(IsdDevice *pDev, uint16_t flag) {
	IsdCmdSetFiq cmd = { 0 };
	ISD_INIT_CMD(cmd, ISDCMD_SET_FIQ);

	cmd.body.flag = flag;
	return IsdSendWriteCommand(pDev, &cmd);
}

int IsdCmdSend_WaitMonEntry(IsdDevice *pDev, IsdMonStatus *pStatus) {
	IsdCommandHeader cmd = { 0 };
	ISD_INIT_CMD_HDR(cmd, ISDCMD_WAITMON_FIQ);

	cmd.size = sizeof(IsdMonStatus);
	return IsdSendReadCommand(pDev, &cmd, pStatus);
}

int IsdCmdSend_WaitMonCmdComplete(IsdDevice *pDev, IsdMonStatus *pStatus) {
	IsdCommandHeader cmd = { 0 };
	ISD_INIT_CMD_HDR(cmd, ISDCMD_WAITMON_CMD);

	cmd.size = sizeof(IsdMonStatus);
	return IsdSendReadCommand(pDev, &cmd, pStatus);
}

int IsdCmdSend_FiqLeave(IsdDevice *pDev) {
	IsdCommandHeader cmd = { 0 };
	ISD_INIT_CMD_HDR(cmd, ISDCMD_FIQ_LEAVE);

	return IsdSendWriteCommand(pDev, &cmd);
}

int IsdCmdSend_WriteMem(IsdDevice *pDev, uint32_t dest, const void *buf, unsigned int size) {
	IsdCmdWriteMem *cmd = calloc(1, sizeof(IsdCmdWriteMem) + size);
	if (cmd == NULL) {
		pDev->status = ISD_STATUS_NOMEM;
		return 0;
	}

	ISD_INIT_CMD(*cmd, ISDCMD_WRITE_MEM);

	cmd->header.size += size;
	cmd->body.addr    = dest;
	cmd->body.length  = (uint16_t) size;
	memcpy(cmd->body.data, buf, size);

	int res = IsdSendWriteCommand(pDev, cmd);
	free(cmd);

	return res;
}

int IsdCmdSend_StartReadMem(IsdDevice *pDev, uint32_t addr, unsigned int size) {
	IsdCmdReadMem cmd = { 0 };
	ISD_INIT_CMD(cmd, ISDCMD_START_READ_MEM);

	cmd.body.addr = addr;
	cmd.body.size = size;
	return IsdSendWriteCommand(pDev, &cmd);
}

int IsdCmdSend_GetReadMemResult(IsdDevice *pDev, void *dest, unsigned int size) {
	IsdCommandHeader cmd = { 0 };
	ISD_INIT_CMD_HDR(cmd, ISDCMD_READ_MEM_RESULT);

	cmd.size = size;
	return IsdSendReadCommand(pDev, &cmd, dest);
}
