#ifndef F3DS2UNR_H
#define F3DS2UNR_H

#define WIN32_LEAN_AND_MEAN
#define STRICT  1

#pragma warning (disable:4786) // ignore "debug information truncated..."

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
using std::string;

#include <vector>
#include <map>
using std::vector;
using std::map;

#include <windows.h>
#include <shlobj.h>

#include "File3DS.h"
#include "Registry.h"
#include "UnrealModel.h"

const char Version[] = "1.00 25-May-01";

#endif // F3DS2UNR_H
