xcopy originals\*.* .\ /s /y

bwpatchw.exe -f DLLS/BIN/Content.dll.patch

bwpatchw.exe -f EXE/common.dll.patch
bwpatchw.exe -f EXE/ebueula.dll.patch
bwpatchw.exe -f EXE/FLServer.exe.patch
bwpatchw.exe -f EXE/Freelancer.exe.patch
bwpatchw.exe -f EXE/rendcomp.dll.patch
bwpatchw.exe -f EXE/rp8.dll.patch
bwpatchw.exe -f EXE/server.dll.patch

xcopy .\DLLS\*.dll ..\DLLS /s /y
xcopy .\EXE\*.dll ..\EXE /s /y
xcopy .\EXE\*.exe ..\EXE /s /y