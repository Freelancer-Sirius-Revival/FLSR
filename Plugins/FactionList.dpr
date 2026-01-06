// Original created by Might_Impress, modified by Skotty for FL:SR

library FactionList;

//{$R *.res}

type
  PUINT = ^UINT;
  UINT = Cardinal;

const
  NUM_DISPLAY = 8;

{$REGION 'Utils'}
function VirtualProtect(lpAddress:Pointer;dwSize:NativeUInt;flNewProtect:UINT;var OldProtect:UINT):LongBool;stdcall;external 'kernel32.dll';

function Patch1(const Code:UINT;const Value:Byte):Boolean;overload;
var
OldProtect : UINT;
begin
Result := VirtualProtect(Pointer(Code),1,$40{PAGE_EXECUTE_READWRITE},OldProtect);
if Result then PByte(Code)^ := Value;
end;

function Patch1(const Code:UINT;const Value:ShortInt):Boolean;overload;
begin Result := Patch1(Code,Byte(Value));end;

function Patch4(const Code:UINT;const Value:UINT):Boolean;
var
OldProtect : UINT;
begin
Result := VirtualProtect(Pointer(Code),4,$40{PAGE_EXECUTE_READWRITE},OldProtect);
if Result then PUINT(Code)^ := Value;
end;

function PatchF(const Code:UINT;const Value:Single):Boolean;
var
OldProtect : UINT;
begin
Result := VirtualProtect(Pointer(Code),4,$40{PAGE_EXECUTE_READWRITE},OldProtect);
if Result then PSingle(Code)^ := Value;
end;
{$ENDREGION}

procedure Patch;
const
StepY  : Single = 0.05;
var
NEW_OFFSET : UINT;  //$34C produces $334 $37C
begin
NEW_OFFSET := PUINT($4A911B)^ + NUM_DISPLAY * 4;
Patch4($4A911B,PUINT($4A911B)^ + NUM_DISPLAY * 16); // wire[] text[] gradienthate[] gradientlove[]
Patch4($4A7B57,NEW_OFFSET);
Patch1($4A7DFC,NUM_DISPLAY);
Patch1($4A7E0B,NUM_DISPLAY);
Patch1($4A7E0D,NUM_DISPLAY-1);
Patch1($4A7BE4,-NUM_DISPLAY * 4);
Patch1($4A7D01,+NUM_DISPLAY * 4);
Patch1($4A7DE9,NUM_DISPLAY * 8);
Patch4($4A825A,NEW_OFFSET);
Patch4($4A81CA,NEW_OFFSET - NUM_DISPLAY * 4);
Patch1($4A81DA,NUM_DISPLAY);
Patch1($4A8282,-NUM_DISPLAY * 4);
Patch1($4A8288,+NUM_DISPLAY * 4);
Patch1($4A828E,NUM_DISPLAY * 8);
Patch1($4A832E,-NUM_DISPLAY * 4);
Patch1($4A834D,-NUM_DISPLAY * 4);
Patch1($4A8353,+NUM_DISPLAY * 4);
Patch1($4A8359,NUM_DISPLAY * 8);
Patch1($4A8363,NUM_DISPLAY);
Patch4($4A8370,NEW_OFFSET - NUM_DISPLAY * 4); //$334
Patch1($4A8375,NUM_DISPLAY);
Patch4($4A838A,NEW_OFFSET + NUM_DISPLAY * 8); //$37C
Patch1($4A83CE,-NUM_DISPLAY * 4);
Patch1($4A86D8,NUM_DISPLAY);
PatchF($5D4954,0.0);          //Faction Controls Offset Y
Patch4($4A7B7B,UINT(@StepY));   //Faction Controls Step Y
PatchF($4A7BC0,0.025);         //Faction Frame Height
PatchF($4A7BB8,0.13);           //Faction Frame Width
//Patch1($4A7C06,0);            //Faction Text Left Align
PatchF($5D4938,0.15);         //Faction Text Y
PatchF($5D4918,0.14);          //Faction Bars Y
PatchF($4A7E10,0.432);          //Faction Scroll Height
PatchF($4A7E31,0.188);          //Faction Scroll Y
PatchF($4A79DD,0.22);           //"REPUTATION" Y
PatchF($4A7A05,0.22);           //"STATS" Y
PatchF($4A7A45,0.455);          //Info Card Width
PatchF($4A7A40,0.35);           //Info Card Height
PatchF($4A7A62,0.182);          //Info Card Y
//InfoCard (RichEditWin Control)
PatchF($474CE2,0.455);
PatchF($474CFC,-0.374);

//NEW_OFFSET := PUINT($4A911B)^;
//FAddrWire := NEW_OFFSET - NUM_DISPLAY * 16;   //OK
//FAddrText := NEW_OFFSET - NUM_DISPLAY * 8;    //??? untested
//FAddrGrad := NEW_OFFSET - NUM_DISPLAY * 4;    //??? untested
//FQuantity := NUM_DISPLAY;
end;

begin
IsMultiThread := True;
Patch;
end.
