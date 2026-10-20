#include <Windows.h>
#include <SetupAPI.h>
#include <initguid.h>

#include <stdio.h>
#include <stdint.h>

#include "isd.h"


int main(void) {
	__debugbreak();
	IsdDevice *pDev = IsdOpen(NULL, ISD_SHARE_MODE_EXCLUSIVE);
	if (pDev == NULL) __debugbreak();

	IsdSelectProc(pDev, TS_PROC_MAIN);
	IsdMonEnter(pDev);
	if (IsdGetLastError(pDev) != ISD_STATUS_OK) __debugbreak();
	IsdClearLastError(pDev);

	//testing
	unsigned char page[0x200];
	IsdReadEmulation(pDev, page, 0, sizeof(page));

	memcpy(page, "TESTING", 7);
	IsdWriteEmulation(pDev, 0, page, sizeof(page));
	__debugbreak();

	IsdMonLeave(pDev);
	IsdClose(pDev);
	return 0;
}

