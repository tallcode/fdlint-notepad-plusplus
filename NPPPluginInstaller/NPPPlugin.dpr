program NPPPlugin;

uses
  Forms,
  SysUtils,
  Windows,
  Registry,
  TLHelp32,
  ShlObj,
  ShellAPI;

const
  FileList : array[1..3] of String = (
    'NppFDevXRay.dll',
    'XRay\xray.exe',
    'XRay\NPPPluginUpdater.exe'
  );

var
  I:Integer;
  Path:String;
  NPPPath:String;

function SelectFolderDialog(const Handle:integer;const Caption:string;
              const InitFolder:string;var SelectedFolder:string):boolean;
var
  BInfo: _browseinfoA;
  Buffer: array[0..MAX_PATH] of Char;
  ID: IShellFolder;
  Eaten, Attribute: Cardinal;
  ItemID: PItemidlist;
begin
  with BInfo do
  begin
    HwndOwner := Handle;
    lpfn := nil;
    lpszTitle := Pchar(Caption);
    ulFlags := BIF_RETURNONLYFSDIRS;
    SHGetDesktopFolder(ID);
    ID.ParseDisplayName(0,nil,'\',Eaten,ItemID,Attribute);
    pidlRoot := ItemID;
    GetMem(pszDisplayName, MAX_PATH);
  end;
  if SHGetPathFromIDList(SHBrowseForFolder(BInfo), Buffer) then
  begin
    SelectedFolder := Buffer;
    if Length(SelectedFolder)<>3 then SelectedFolder := SelectedFolder + '\';
    result := True;
  end
  else begin
    SelectedFolder := '\';
    result := False;
  end;
end;

function GetNPPDir():String;
begin
  Result:='';
  with TRegistry.Create() do
  begin
    RootKey:=HKEY_LOCAL_MACHINE;
    OpenKey('SOFTWARE',True);
    OpenKey('Notepad++',True);
    Result:=ReadString('');
    CloseKey;
  end;
  if((DirectoryExists(Result+'\plugins'))and(FileExists(Result+'\notepad++.exe')))then
  begin
    Result:=Result+'\';
    exit;
  end
  else
  begin
    if(SelectFolderDialog(Application.Handle,'请手工选择Notepad++安装目录','C:\',Result))then
    begin
       if((DirectoryExists(Result+'plugins'))and(FileExists(Result+'notepad++.exe')))then
        exit
       else
       begin
        Application.MessageBox('请选择正确的Notepad++安装目录','提示');
        Result:='';
       end;
    end
    else
    begin
      Result:='';
    end;
  end;
end;  

function FindProcess(AFileName: string): boolean;
var
  hSnapshot: THandle;//用于获得进程列表
  lppe: TProcessEntry32;//用于查找进程
  Found: Boolean;//用于判断进程遍历是否完成
begin
  Result :=False;
  hSnapshot := CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);//获得系统进程列表
  lppe.dwSize := SizeOf(TProcessEntry32);//在调用Process32First API之前，需要初始化lppe记录的大小
  Found := Process32First(hSnapshot, lppe);//将进程列表的第一个进程信息读入ppe记录中
  while Found do
  begin
    if ((UpperCase(ExtractFileName(lppe.szExeFile))=UpperCase(AFileName)) or (UpperCase(lppe.szExeFile )=UpperCase(AFileName))) then
    begin
      Result :=True;
      exit;
    end;
    Found := Process32Next(hSnapshot, lppe);//将进程列表的下一个进程信息读入lppe记录中
  end;
end;

begin
  Path:=ExtractFilePath(Application.ExeName);
  for I:=1 to Length(FileList) do
  begin
    if(not FileExists(Path+FileList[I]))then
    begin
     Application.MessageBox('缺少安装文件','提示');
     Exit;
    end;
  end;
  if(FindProcess('notepad++.exe'))then
  begin
    Application.MessageBox('Notepad++正在运行','提示');
    exit;
  end;
  NPPPath:=GetNPPDir;
  if(NPPPath='')then
  begin
    exit;
  end;
  CreateDirectory(PChar(NPPPath+'plugins\XRay'),nil);
  for I:=1 to Length(FileList) do
  begin
    CopyFile(PChar(Path+FileList[I]),PChar(NPPPath+'plugins\'+FileList[I]),false);
  end;
  Application.MessageBox('FDLint for Notepad++安装成功','提示');
end.
