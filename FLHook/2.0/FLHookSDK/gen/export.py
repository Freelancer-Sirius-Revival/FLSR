import os
import re
import sys
import subprocess

dumpBinPath = ""
undnamePath = ""
libPath = ""

def failedPath(p):
    print("Could not find path: " + p)
    exit(1)

def pickNewest(p, pattern=None):
    lst = []
    with os.scandir(p) as it:
        for entry in it:
            if entry.is_dir() and (pattern is not None and re.match(pattern, entry.name) or pattern is None):
                lst.append(entry.name)
    lst = sorted(lst, key=str.lower)
    return lst[-1]

def demangle(name):
    output = subprocess.run([undnamePath, name], stdout=subprocess.PIPE).stdout.decode('utf-8')
    loc = output.find("is :-")
    return output[(loc + 7):-5]

def isData(name):
    demangled = demangle(name)
    if name[0] != "?":
        return name[0] != "_" and name[:3] != "Get"
    return demangled.find("(") == -1 or demangled.find("vftable") != -1


srcPath = " ".join(sys.argv[1:])
srcName = os.path.basename(srcPath)[:-4]
srcExt = os.path.basename(srcPath)[-3:].lower()

print("Using input file " + srcPath)

binaryPath = "C:/Program Files (x86)/Microsoft Visual Studio/"

if not os.path.exists(binaryPath):
    failedPath(binaryPath)

binaryPath += pickNewest(binaryPath, pattern='^[0-9]+$')

binaryPath += "/Community/VC/Tools/MSVC/"

binaryPath += pickNewest(binaryPath, pattern='^[0-9\.]+$')

binaryPath += "/bin/Hostx64/x86/"

print("Using VS Common Tools in " + binaryPath)

dumpBinPath = "\"" + binaryPath + "dumpbin.exe" + "\""
libPath = "\"" + binaryPath + "lib.exe" + "\""
undnamePath = binaryPath + "undname.exe"

print("Dumping exports...")
os.system(dumpBinPath + " /exports /out:__tmp.txt " + srcPath)

with open("__tmp.txt", "r") as f:
    with open(srcName + ".def", "w") as o:
        addedOrdinals = []
        if srcExt == 'dll':
            o.write("LIBRARY " + srcName + "\n")
        else:
            o.write("NAME " + srcName + "\n")
        o.write("EXPORTS\n")
        lines = f.readlines()[16:-8]
        for line in lines:
            elems = line.split()
            ordinal = elems[0]
            mangled = elems[-1]
            if mangled.find("@std@") != -1:
                appended = mangled.replace("@std@", "@st6@") + " @" + ordinal
                if isData(mangled):
                    appended += " DATA"
                addedOrdinals.append(appended)
            if isData(mangled):
                mangled += " DATA"
            o.write("  " + mangled + "\n")
        for ordinal in addedOrdinals:
            o.write("  " + ordinal + "\n")

os.system(libPath + " /def:" + srcName + ".def /out:" + srcName + ".lib /machine:X86")
os.remove("__tmp.txt")