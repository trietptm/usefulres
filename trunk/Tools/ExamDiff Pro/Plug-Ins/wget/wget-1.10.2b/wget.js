nSlashes = CountChars(WScript.Arguments(1), '/', true) - 2;
cmdLine = "\"" + WScript.Arguments(0) + "\"" + " -r -nH --cut-dirs=" + nSlashes + " -P " +
    "\"" + WScript.Arguments (2) + "\"" + " " + WScript.Arguments(1);
//WScript.Echo('*** Command:' + cmdLine);
var WshShell = new ActiveXObject("WScript.Shell");
var oExec = WshShell.Exec(cmdLine);

var out = "";
var err = "";
var tryCount = 0;

// Wait for the completion
while (true)
{
	var input = ReadAllFromAny(oExec);
	if (-1 == input)
	{
		if (tryCount++ > 10 && oExec.Status == 1)
		   break;
		WScript.Sleep(100);
	}
	else
	{
		tryCount = 0;
	}
}

WScript.StdOut.Write(out);
WScript.StdErr.Write(err);
WScript.Quit(oExec.ExitCode);

function ReadAllFromAny(oExec)
{
	if (!oExec.StdErr.AtEndOfStream)
	{
		err += oExec.StdErr.ReadAll();
		return 1;
	}

	if (!oExec.StdOut.AtEndOfStream)
	{
		out += oExec.StdOut.ReadAll();
		return 1;
	}

	return -1;
}

function CountChars(sString, cChar, bIgnoreLast)
{
	nCount = 0, nLen = sString.length;
	if (bIgnoreLast && cChar == sString.charAt(nLen - 1)) nLen--;
	for (i = 0; i < nLen; i++) if (cChar == sString.charAt(i)) nCount++;
	return nCount;
}
