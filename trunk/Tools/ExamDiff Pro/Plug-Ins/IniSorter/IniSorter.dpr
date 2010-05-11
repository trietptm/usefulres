program IniSorter;

{$APPTYPE CONSOLE}
{ $DEFINE DEBUG}

{$R *.res}

uses
  Classes, SysUtils, IniFiles, Windows;

const
  IniFileExt = '.ini';
  SortedMarker = '.sorted';
  LineFeed = ^M^J;
  EOS = #0;

//---------------------------------------------------------------------------
function IsDigit(C: char): boolean;
begin
  IsDigit:=C in ['0'..'9'];
end;

//---------------------------------------------------------------------------
function NaturalNumberOrder(Item1, Item2: string): integer;

  function SpanNumbers(const S: string): integer;
  var
    I: integer;
  begin
    I:=1;
    while (I <= Length(S)) and IsDigit(S[I]) do Inc(I);
    SpanNumbers:=I - 1;
  end;

  function SpanNotNumbers(const S: string): integer;
  var
    I: integer;
  begin
    I:=1;
    while (I <= Length(S)) and not IsDigit(S[I]) do Inc(I);
    SpanNotNumbers:=I - 1;
  end;

var
{$IFDEF DEBUG}
  Temp1, Temp2: string;
{$ENDIF DEBUG}
  N1, N2: integer;
  Span1, Span2: integer;
begin
{$IFDEF DEBUG}
  Temp1:=Item1; Temp2:=Item2;
{$ENDIF DEBUG}
  if Item1 = '' then
  begin
    if Item2 = '' then
    begin
      Result:=0;
    {$IFDEF DEBUG}
      OutputDebugString(PChar('"' + Item1 + '" = "' + Item2 + '"'));
    {$ENDIF DEBUG}
    end
    else
    begin
      Result:=-1;
    {$IFDEF DEBUG}
      OutputDebugString(PChar('"' + Item1 + '" < "' + Item2 + '"'));
    {$ENDIF DEBUG}
    end;
    exit;
  end;
  if Item2 = '' then
  begin
    Result:=1;
  {$IFDEF DEBUG}
    OutputDebugString(PChar('"' + Item1 + '" > "' + Item2 + '"'));
  {$ENDIF DEBUG}
    exit;
  end;

  Result:=0;
  repeat
    // Get next "chunk"
    // A chunk is either a group of letters or a group of numbers
    if IsDigit(Item1[1]) and IsDigit(Item2[1]) then
    begin
      // They both contain numbers
      Span1:=SpanNumbers(Item1);
      Span2:=SpanNumbers(Item2);
      // Convert the numeric strings to integers
      Val(Copy(Item1, 1, Span1), N1, Result);
      Val(Copy(Item2, 1, Span2), N2, Result);
      // Remove the extracted chunks from the strings
      Delete(Item1, 1, Span1);
      Delete(Item2, 1, Span2);
      // Numerical comparison
      Result:=0;
      if N1 > N2 then
	Result:=1
      else
	if N1 < N2 then
	  Result:=-1
    end
    else
      if not IsDigit(Item1[1]) and not IsDigit(Item2[1]) then
      begin
	// They both contain letters
	Span1:=SpanNotNumbers(Item1);
	Span2:=SpanNotNumbers(Item2);
	Result:=CompareStr(Copy(Item1, 1, Span1), Copy(Item2, 1, Span2));
	// Remove the extracted chunks from the strings
	Delete(Item1, 1, Span1);
	Delete(Item2, 1, Span2);
      end
      else
	// A letter and a number: surely different, use a standard compare
	Result:=CompareStr(Item1, Item2);
  until (Result <> 0) or (Item1 = '') or (Item2 = '');

  if Result = 0 then
    if (Item1 = '') <> (Item2 = '') then
      // If only one string is now empty they can not be equal!
      Result:=CompareStr(Item1, Item2);

{$IFDEF DEBUG}
  if Result = 0 then
    OutputDebugString(PChar('"' + Temp1 + '" = "' + Temp2 + '"'))
  else
    if Result > 0 then
      OutputDebugString(PChar('"' + Temp1 + '" > "' + Temp2 + '"'))
    else
      OutputDebugString(PChar('"' + Temp1 + '" < "' + Temp2 + '"'));
{$ENDIF DEBUG}
end;

//---------------------------------------------------------------------------
function KeyNaturalNumberOrder(List: TStringList; Index1, Index2: integer): integer;
begin
  if List.CaseSensitive then
    KeyNaturalNumberOrder:=NaturalNumberOrder(List.Names[Index1],
					      List.Names[Index2])
  else
    // Case-insensitive comparison
    KeyNaturalNumberOrder:=NaturalNumberOrder(LowerCase(List.Names[Index1]),
					      LowerCase(List.Names[Index2]));
end;

//---------------------------------------------------------------------------
function GenericNaturalNumberOrder(List: TStringList; Index1, Index2: integer): integer;
begin
  if List.CaseSensitive then
    GenericNaturalNumberOrder:=NaturalNumberOrder(List.Strings[Index1],
						  List.Strings[Index2])
  else
    // Case-insensitive comparison
    GenericNaturalNumberOrder:=NaturalNumberOrder(LowerCase(List.Strings[Index1]),
						  LowerCase(List.Strings[Index2]));
end;

//---------------------------------------------------------------------------
var
  UseStandardOutput: boolean;
  FileCount: integer;
  Error: boolean;
  OutputFile: TMemoryStream;
  Sections: TStringList;
  SectionData: TStringList;
  IniFile: TMemIniFile;
  I: integer;
  SectionIndex: integer;
  Filename: string;
  Heading: string;
begin
  UseStandardOutput:=false;
  Error:=false;
  FileCount:=0;

  OutputFile:=TMemoryStream.Create;

  Sections:=TStringList.Create;
  Sections.CaseSensitive:=false;
  Sections.Sorted:=false;

  SectionData:=TStringList.Create;
  SectionData.CaseSensitive:=false;
  SectionData.Sorted:=false;

  if ParamCount = 0 then
  begin
    writeln(ChangeFileExt(ExtractFileName(ParamStr(0)), ''),
	    ':  ' + IniFileExt + ' file sorter');
    writeln('Sorts ' + IniFileExt +
	    ' files for better reading and to ease comparisons');
    writeln('Syntax:');
    writeln('    ' + ChangeFileExt(ExtractFileName(ParamStr(0)), ''),
	    ' [/f][/o] filename[' + IniFileExt +
	    '] [filename[' + IniFileExt + ']...]');
    writeln('    /f  Results sent to a file ' + SortedMarker + IniFileExt +
	    ' (default)');
    writeln('    /o  Results sent to standard output');
    Halt(1);
  end;

  for I:=1 to ParamCount() do
  begin
    Filename:=ParamStr(I);

    // Switch
    if Filename[1] = '/' then
    begin
      if Length(Filename) > 1 then
	case UpCase(Filename[2]) of
	  'O': UseStandardOutput:=true;
	  'F': UseStandardOutput:=false;
	end;
      continue;
    end;

    Inc(FileCount);

    // If there is no explicit extension use the default
    if ExtractFileExt(Filename) = '' then
      Filename:=ChangeFileExt(Filename, IniFileExt);

    if (AnsiPos(Filename, '?') <> 0) or (AnsiPos(Filename, '*') <> 0) then
    begin
      writeln(ErrOutput, 'Wildcards not allowed: ', Filename);
      Error:=true;
      continue;
    end;

    if not FileExists(Filename) then
    begin
      writeln(ErrOutput, 'File not found: ', Filename);
      Error:=true;
      continue;
    end;
    Filename:=ExpandUNCFileName(Filename);

    if not UseStandardOutput then writeln('Sorting ', Filename);

    IniFile:=nil;
    try
      IniFile:=TMemIniFile.Create(Filename);
      OutputFile.Clear;
      Sections.Clear;

      // Read the names of the sections
      IniFile.ReadSections(Sections);
      if Sections.Count = 0 then
      begin
	Error:=true;
	if not UseStandardOutput then
	  writeln('Invalid or empty ',IniFileExt,' file');
      end
      else
      begin
	// Natural sort
	Sections.CustomSort(GenericNaturalNumberOrder);

	// Elaborate each section
	for SectionIndex:=0 to pred(Sections.Count) do
	begin
	  SectionData.Clear;
	  // Read keys and values
	  IniFile.ReadSectionValues(Sections.Strings[SectionIndex], SectionData);
	  // Natural sort
	  SectionData.CustomSort(KeyNaturalNumberOrder);

	  // Write on the output stream
	  // Section heading
	  Heading:='[' + Sections.Strings[SectionIndex] + ']' + LineFeed;
	  OutputFile.Write(Heading[1], Length(Heading));
	  // Section data
	  SectionData.SaveToStream(OutputFile);
	  // Separator
	  OutputFile.Write(LineFeed, Length(LineFeed));
	end;

	// Write the results
	Filename:=ChangeFileExt(Filename, SortedMarker + IniFileExt);
	if not UseStandardOutput then writeln('Writing ', Filename);

	if UseStandardOutput then
	  writeln(PChar(OutputFile.Memory))
	else
	  try
	    OutputFile.SaveToFile(Filename);
	  except
	    writeln(ErrOutput, 'Error writing file ', Filename);
	    Error:=true;
	  end;
      end;
    except
      writeln(ErrOutput, 'Error handling file ', Filename);
      Error:=true;
    end;

    IniFile.Free;
  end;

  if not UseStandardOutput then writeln('Finished');

  SectionData.Free;
  Sections.Free;
  OutputFile.Free;

  if (FileCount = 1) and Error then Halt(1);
end.
