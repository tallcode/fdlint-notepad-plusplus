//this file is part of notepad++
//Copyright (C)2003 Don HO <donho@altern.org>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"

//
// put the headers you need here
//
#include <stdlib.h>
#include <shlwapi.h>
#include "XRayDlg.h"

XRayDlg _xrayDlg;

#ifdef UNICODE 
	#define generic_itoa _itow
#else
	#define generic_itoa itoa
#endif

FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

#define DOCKABLE_DLG_INDEX 0

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
	// Initialize dockable demo dialog
	_xrayDlg.init((HINSTANCE)hModule, NULL);
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
	//CleanUp();
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
	//
	// Firstly we get the parameters from your plugin config file (if any)
	//

	// get path of plugin configuration
	//::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)iniFilePath);

	// if config path doesn't exist, we create it
	/*if (PathFileExists(iniFilePath) == FALSE)
	{
		::CreateDirectory(iniFilePath, NULL);
	}*/

	// make your plugin config file full file path name
	//PathAppend(iniFilePath, configFileName);

	// get the parameter value from plugin config
	//doCloseTag = (::GetPrivateProfileInt(sectionName, keyName, 0, iniFilePath) != 0);


    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
	// Shortcut :
	// Following code makes the first command
	// 绑定快捷键 Ctrl+Shift+X
	ShortcutKey *shKey = new ShortcutKey;
	shKey->_isAlt = false;
	shKey->_isCtrl = true;
	shKey->_isShift = true;
	shKey->_key = 0x58; //相当于VK_X
	setCommand(DOCKABLE_DLG_INDEX, TEXT("FDLint"), ShowDockableDlg, shKey, false);
	setCommand(1, TEXT("---"), NULL, NULL, false);
	setCommand(2, TEXT("About FDLint"), AboutXRay, NULL, false);
}


//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
	delete funcItem[0]._pShKey;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

// Dockable Dialog Demo
// 
// This demonstration shows you how to do a dockable dialog.
// You can create your own non dockable dialog - in this case you don't nedd this demonstration.
// You have to create your dialog by inherented DockingDlgInterface class in order to make your dialog dockable
// 显示扫描窗口
void ShowDockableDlg()
{
	_xrayDlg.setParent(nppData._nppHandle);
	tTbData	data = {0};

	//如果窗口没有创建，则创建窗口
	if (!_xrayDlg.isCreated()){
		_xrayDlg.create(&data);
		// define the default docking behaviour
		// 窗口位于编辑器底部
		data.uMask = DWS_DF_CONT_BOTTOM;
		data.pszModuleName = _xrayDlg.getPluginFileName();
		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DLG_INDEX
		data.dlgID = DOCKABLE_DLG_INDEX;
		::SendMessage(nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
		::OutputDebugString(TEXT("Create Window"));
		TCHAR UpdaterPath[MAX_PATH];
		::SendMessage(nppData._nppHandle, NPPM_GETNPPDIRECTORY, 0, (LPARAM)UpdaterPath);
		lstrcat(UpdaterPath,TEXT("\\plugins\\XRay\\NPPPluginUpdater.exe"));
		::ShellExecute(NULL,TEXT("open"),UpdaterPath,NULL,NULL,SW_SHOWNORMAL);
	}
	// 显示窗口
	_xrayDlg.display();
	// 自动执行扫描
	_xrayDlg.Scan();
}
//隐藏窗口
void HideDockableDlg()
{
	// 如果窗口已经被创建，则隐藏窗口
	if (_xrayDlg.isCreated()){
		_xrayDlg.display(false);
	}
}
//关于XRay
void AboutXRay()
{
	//打开网页
	::ShellExecute(NULL,TEXT("open"),TEXT("http://wd.alibaba-inc.com/doc/page/tools/fdlint"),NULL,NULL,SW_SHOWNORMAL);
}
