/*
UVNet Universal Decompiler (uvudec)
Copyright 2010 John McMaster <JohnDMcMaster@gmail.com>
Licensed under the terms of the LGPL V3 or later, see COPYING for details
*/

#ifndef UVQT_UTIL_H
#define UVQT_UTIL_H

#include <QWidget>
#include "uvd/util/util.h"

//Are these defined somewhere?
#define UVQT_KEY_LEFT		0x01000012
#define UVQT_KEY_UP			0x01000013
#define UVQT_KEY_RIGHT		0x01000014
#define UVQT_KEY_DOWN		0x01000015
#define UVQT_KEY_PAGEUP		0x01000016
#define UVQT_KEY_PAGEDOWN	0x01000017

void UVDQtPrintRect(QRect rect);

#define UV_ASSERT_VOID(x)	if( !x ) { printf("ERROR %s@%s:%d: assertion failed\n", __FUNCTION__, __FILE__, __LINE__); fflush(stdout); UVD_PRINT_STACK(); exit(1); }

#endif

