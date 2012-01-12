program NPPPluginUpdater;

uses
  Forms,
  SysUtils,
  Classes, 
  Windows,
  Registry,
  TLHelp32,
  ShellAPI,
  IdHTTP;

procedure AdjustToken();
var
TPPrev, TP: TTokenPrivileges;
TokenHandle: THandle;
dwRetLen: DWORD;
lpLuid: TLargeInteger;
begin
OpenProcessToken(GetCurrentProcess, TOKEN_ALL_ACCESS, TokenHandle);
if (LookupPrivilegeValue(nil, 'SeDebugPrivilege', lpLuid)) then
begin
    TP.PrivilegeCount := 1;
    TP.Privileges[0].Attributes := SE_PRIVILEGE_ENABLED;
    TP.Privileges[0].Luid := lpLuid;
    AdjustTokenPrivileges(TokenHandle, False, TP, SizeOf(TPPrev), TPPrev, dwRetLen);
end;
CloseHandle(TokenHandle);
end;

function DownloadVersion():Integer;
var
  IdHTTP:TIdHTTP;
  Temp:String;
begin
  IdHTTP:=TIdHTTP.Create(nil);
  IdHTTP.HandleRedirects:=True;
  IdHTTP.Request.UserAgent:='Mozilla/3.0(FDLint Updater)';
  try
    Temp:=Trim(IdHTTP.Get('http://wd.alibaba-inc.com/tools/fdlint/version.txt'));
  except
    Temp:='0';
  end;
  try
    Result:=StrToInt(Temp);
  except
    Result:=0;
  end;
  FreeAndNil(IdHTTP);
end;

function DownloadFile():Boolean;
var
  IdHTTP:TIdHTTP;
  NewFile:TFileStream;
begin
  IdHTTP:=TIdHTTP.Create(nil);
  NewFile:=TFileStream.Create('new_xray.exe', fmCreate);
  IdHTTP.HandleRedirects:=True;
  IdHTTP.Request.UserAgent:='Mozilla/3.0(FDLint Updater)';
  try
    IdHTTP.Get('http://wd.alibaba-inc.com/tools/fdlint/xray.exe',NewFile);
    FreeAndNil(NewFile);
    Result := True
  except
    Result := False;
  end;
  FreeAndNil(IdHTTP);
end;

function ReadCurrentVersion():Integer;
var
  Temp:String;
begin
  with TRegistry.Create() do
  begin
    RootKey:=HKEY_LOCAL_MACHINE;
    OpenKey('SOFTWARE',True);
    OpenKey('Notepad++',True);
    Temp:=ReadString('FDLint');
    CloseKey;
    Free;
  end;
  try
    Result:=StrToInt(Temp);
  except
    Result:=0;
  end;
end;

function WriteCurrentVersion(Version:Integer):Boolean;
begin
  with TRegistry.Create() do
  begin
    RootKey:=HKEY_LOCAL_MACHINE;
    OpenKey('SOFTWARE',True);
    OpenKey('Notepad++',True);
    WriteString('FDLint',IntToStr(Version));
    CloseKey;
    Free;
  end;
  Result:=True;
end;

function FindProcess(AFileName: string): DWORD;
var
  hSnapshot: THandle;//用于获得进程列表
  lppe: TProcessEntry32;//用于查找进程
  Found: Boolean;//用于判断进程遍历是否完成
begin
  Result :=0;
  hSnapshot := CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);//获得系统进程列表
  lppe.dwSize := SizeOf(TProcessEntry32);//在调用Process32First API之前，需要初始化lppe记录的大小
  Found := Process32First(hSnapshot, lppe);//将进程列表的第一个进程信息读入ppe记录中
  while Found do
  begin
    if ((UpperCase(ExtractFileName(lppe.szExeFile))=UpperCase(AFileName)) or (UpperCase(lppe.szExeFile )=UpperCase(AFileName))) then
    begin
      Result := lppe.th32ProcessID;
      exit;
    end;
    Found := Process32Next(hSnapshot, lppe);//将进程列表的下一个进程信息读入lppe记录中
  end;
end;

function UpdateFile():Boolean;
var
  PID:DWORD;
  hProcess:THandle;
begin
  Result:=False;
  AdjustToken();
  PID:=FindProcess('xray.exe');
  if(PID>0)then
  begin
    hProcess:=OpenProcess(PROCESS_ALL_ACCESS,False,PID);
    if((hProcess>0)and(WaitForSingleObject(hProcess,600000)=WAIT_OBJECT_0))then
    begin
      try
        CopyFile('new_xray.exe','xray.exe',False);
        DeleteFile('new_xray.exe');
        Result:=True;
      except
        Result:=False;
      end;
    end;
  end
  else
  begin
    CopyFile('new_xray.exe','xray.exe',False);
    DeleteFile('new_xray.exe');
    Result:=True;
  end;
end;

procedure Main();
var
  CurrentVersion:Integer;
  OnlineVersion:Integer;
begin
  OnlineVersion := DownloadVersion();
  if(OnlineVersion = 0) then exit;
  CurrentVersion := ReadCurrentVersion();
  if((OnlineVersion>CurrentVersion)and(DownloadFile()))then
  begin
    if(UpdateFile())then
    begin
      WriteCurrentVersion(OnlineVersion);
      Application.MessageBox('FDLint更新更新成功,请重新扫描文件','提示');
    end
    else
    begin
      Application.MessageBox('FDLint有更新，但是更新失败','提示');
    end;
  end;
end;

begin
  Main();
end.
