/*
UVNet Universal Decompiler (uvudec)
Copyright 2008 John McMaster <JohnDMcMaster@gmail.com>
Licensed under terms of the three clause BSD license, see LICENSE for details
*/

#include <QApplication>
#include "uvd_GUI.h"

uv_err_t uvmain(int argc, char **argv, int *retOut)
{
	QApplication *application = NULL;
	UVDMainWindow *mainWindow = NULL;

	application = new QApplication(argc, argv);
	uv_assert_ret(application);
	mainWindow = new UVDMainWindow();
	uv_assert_ret(mainWindow);

	uv_assert_err_ret(mainWindow->init());
	
	mainWindow->show();
	*retOut = application->exec();
	
	delete mainWindow;
	delete application;
	
	return UV_ERR_OK;
}

int main(int argc, char **argv)
{
	int ret = 1;

	UV_DEBUG(uvmain(argc, argv, &ret));
	return ret;
}
