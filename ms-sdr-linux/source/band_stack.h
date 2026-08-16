#pragma once
#include <math.h>
#include <stdio.h>
#include "extern.h"

struct band_stack {
	UINT32 freq[3];
	char mode[3];
};