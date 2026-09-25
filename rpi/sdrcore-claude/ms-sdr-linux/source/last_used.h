#pragma once
#include <math.h>
//#include <windows.h>
#include <stdio.h>
//#include <conio.h>		// gives _cprintf()
//#include <ShlObj.h>
//#include <KnownFolders.h>

struct last_used_stack {
	uint8_t band;
	UINT32 freq;
	char mode;
};