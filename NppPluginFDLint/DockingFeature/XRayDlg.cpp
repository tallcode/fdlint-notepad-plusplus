//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
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

#include "XRayDlg.h"
#include "PluginDefinition.h"

extern NppData nppData;

//建立新的线程，在线程中扫描
DWORD WINAPI ThreadScan (LPVOID lpParam)
{
	//强制保存文件
	//防止插件崩溃而导致的文件丢失
	::SendMessage(nppData._nppHandle, NPPM_SAVECURRENTFILE, 0, 0);
	
	//H是扫描窗口的句柄
	HWND H = (HWND)lpParam;
	//设置状态文本
	::SetWindowText(::GetDlgItem(H,IDC_STATUS),TEXT("正在扫描请稍候......"));
	//清空ListBox
	::SendMessage(::GetDlgItem(H,IDC_RESULTLIST),LB_RESETCONTENT,0,0);

	TCHAR FilePath[MAX_PATH];
	TCHAR PluginPath[MAX_PATH];
	TCHAR EXEPath[MAX_PATH] =TEXT("");
	TCHAR TempFilePath[MAX_PATH] =TEXT("");
	TCHAR cmdLine[1024] =TEXT("");
	//得到NPP插件目录
	::SendMessage(nppData._nppHandle, NPPM_GETNPPDIRECTORY, 0, (LPARAM)PluginPath);
	lstrcat(PluginPath,TEXT("\\plugins\\XRay\\"));
	//得到当前文件路径
	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 0, (LPARAM)FilePath);
	//组装可执行文件路径
	lstrcat(EXEPath,PluginPath);
	lstrcat(EXEPath,TEXT("xray.exe"));
	//组装临时文件路径
	lstrcat(TempFilePath,PluginPath);
	lstrcat(TempFilePath,TEXT("output.txt"));
	//组装命令行
	lstrcat(cmdLine,EXEPath);
	//指定xray输出格式
	lstrcat(cmdLine,TEXT(" --format=vim \""));
	lstrcat(cmdLine,FilePath);
	lstrcat(cmdLine,TEXT("\""));
	//测试用的命令行
	//TCHAR cmdLine[MAX_PATH] = TEXT("ipconfig /all");
	::OutputDebugString(cmdLine);
	//设置SA
	SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

	//设置si和pi
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	//创建临时文件
	HANDLE hFile = ::CreateFile(TempFilePath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		//文件创建失败
		::SetWindowText(::GetDlgItem(H,IDC_STATUS),TEXT("创建临时文件失败"));	
		return 0;
	}

	//si分配空间，更改输出句柄
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdError = hFile;
	si.hStdOutput = hFile;
	si.wShowWindow = SW_HIDE;  

	//创建进程
	BOOL ret = ::CreateProcess(NULL, cmdLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
	if(ret){
		//等待60秒或者进程结束
		if(::WaitForSingleObject(pi.hProcess,600000)==WAIT_OBJECT_0){
				//取得临时文件尺寸
				DWORD dwFileSize=::GetFileSize(hFile, NULL); 
				//申请空间
				char* Output = new char[dwFileSize+1];
				DWORD bytesRead;
				//移动读写指针到文件头部
				::SetFilePointer(hFile,0,0,FILE_BEGIN);
				//读取文件全部内容
				::ReadFile(hFile,Output,dwFileSize,&bytesRead,NULL);
				Output[bytesRead]='\0';
				//关闭文件
				CloseHandle(hFile);
				//UTF-8转Unicode
				int wcsLen = ::MultiByteToWideChar(CP_UTF8,NULL,Output,bytesRead,NULL,0);
				TCHAR* T = new TCHAR[wcsLen+1];
				::MultiByteToWideChar(CP_UTF8,NULL,Output,bytesRead,T,wcsLen);
				T[wcsLen+1] = '\0';
				//按行分割
				TCHAR * lpch = T;
				TCHAR * pch = wcschr(T,'\n');
				int len;
				while(pch!=NULL){
					len = pch - lpch +1;
					TCHAR* Line = new TCHAR[len+1];
					lstrcpyn(Line,lpch,len);
					Line[len] = '\0';
					//插入Listbox
					::SendMessage(::GetDlgItem(H,IDC_RESULTLIST),LB_INSERTSTRING,-1,(LPARAM)Line);
					delete[] Line;
					lpch = pch;
					pch = wcschr(pch+1,'\n');
				}
	
				delete[] Output;
				delete[] T;
				//设置状态文本
				::SetWindowText(::GetDlgItem(H,IDC_STATUS),TEXT("扫描完成"));
		}
		else{
			//终止进程
			::TerminateProcess(pi.hProcess,0);
			//关闭文件
			CloseHandle(hFile);
			::SetWindowText(::GetDlgItem(H,IDC_STATUS),TEXT("等待超时"));
		}
	}

	return 0;
}

void XRayDlg::Scan(){		
	//获得Notepad++识别的文件类型
	int LANGTYPE = ::SendMessage(nppData._nppHandle, NPPM_GETBUFFERLANGTYPE,::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0), 0);
	if((LANGTYPE==-1)||((LANGTYPE!=LangType(L_HTML))&&(LANGTYPE!=LangType(L_JS))&&(LANGTYPE!=LangType(L_CSS)))){
		::MessageBox(_hSelf,TEXT("只能扫描HTML、JS或者CSS文件"),TEXT("文件类型"),MB_OK);
		return;
	}
	//TCHAR EXT[MAX_PATH];
	//得到扩展名
	//SendMessage(nppData._nppHandle, NPPM_GETEXTPART, 0, (LPARAM)EXT);
	//创建扫描线程
	DWORD dwThreadId;
	::CreateThread(NULL,0,ThreadScan,_hSelf,0,&dwThreadId);
	::OutputDebugString(TEXT("Create Thread"));
	return;
}

BOOL CALLBACK XRayDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDC_RESULTLIST:
				{
					//双击ListBox触发
					if(HIWORD(wParam)==LBN_DBLCLK){
						int index;
						int len;
						//取得选中条目的index
						index = ::SendDlgItemMessage(_hSelf,IDC_RESULTLIST,LB_GETCARETINDEX,0,0);
						//取得选中条目的文本长度
						len = ::SendDlgItemMessage(_hSelf,IDC_RESULTLIST,LB_GETTEXTLEN,(WPARAM)index,0);
						if(len == LB_ERR){
							return FALSE;
						}
						//新建可以保存选中文本的缓冲区
						TCHAR* T = new TCHAR[len+1];
						//取得选中条目的文本
						if(::SendDlgItemMessage(_hSelf,IDC_RESULTLIST,LB_GETTEXT,(WPARAM)index,(LPARAM)T) == LB_ERR){
							delete[] T;
							return FALSE;
						}
						//文本结尾
						T[len] = '\0';
						//取得文本中的行号，即“]:”和“,”中间的文本
						TCHAR* start = wcschr(T,']');
						
						TCHAR* end = wcschr(T,',');
						if((start>T)&&(end>start)){
							len = end-start-1;
							TCHAR* line = lstrcpyn(T,start+2,len);
							__try{
								//转为整形
								int lineNum = _wtoi(line);
								if (lineNum >0 )
								{
									// Get the current scintilla
									int which = -1;
									::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
									if (which == -1)
										return FALSE;
									HWND curScintilla = (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
									//跳到行
									::SendMessage(curScintilla, SCI_ENSUREVISIBLE, lineNum-1, 0);
									::SendMessage(curScintilla, SCI_GOTOLINE, lineNum-1, 0);
								}
							}
							__except(EXCEPTION_EXECUTE_HANDLER){
								//输出错误信息
								//OutputDebugString(str);
							}
						}
						//释放空间
						delete[] T;
						return TRUE;
					}
				}
			}
			return FALSE;
		}
		//窗口尺寸变化时触发
		case WM_SIZE:
		{
			//取得宽高
			static UINT cx,cy;
			cx = LOWORD(lParam);
			cy = HIWORD(lParam);
			if((cx>30)&&(cy>100)){
				//调整控件位置和宽高
				::SetWindowPos(::GetDlgItem(_hSelf,IDC_RESULTLIST),0,0,30,cx,cy-30,SWP_NOOWNERZORDER);
				//SetWindowPos(GetDlgItem(_hSelf,IDOK),0,cx-75,3,70,24,SWP_NOOWNERZORDER);
			}
			return TRUE;
		}

		default :
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}
}

