{Coded by JFX <JFX@yandex.ru>}

library HelloWord;
uses
  sysutils,
  Windows;
const
 PluginName='Hello Word!';
{$R *.RES}
function GetPTPluginName:PChar;stdcall;
begin
 Result:=PluginName;
end;

function StartPTPlugin(hDlg:HWND):DWord;stdcall;
begin
 MessageBox(hDlg, PChar('Hello Word !!!'), 'PlugIn for PE Tools', 0);
 Result:=0;
end;

exports GetPTPluginName, StartPTPlugin;
begin
end.