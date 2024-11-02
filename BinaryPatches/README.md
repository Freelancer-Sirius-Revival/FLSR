# Binary Patches for FL:SR

Plenty of changes to behaviour in Freelancer is done by binary patching.

## Structure

- `bwpatchw` contains the binaries of adoxa’s bwpatch tool
- `originals` contains vanilla Freelancer 1.1 files (with version-modified EXE files for FL:SR)
- the other directories mimick FL’s directory structure containing binary files

## Patching

To apply all patches and have them automatically be copied into the `Freelancer` directory of this repository, just execute `patch.bat`.

## Adding Patches

### To New File

1. Copy a vanilla 1.1-patched binary to `originals`, keeping FL’s directory structure there.
2. Create a new `BINARYNAME.patch` file with the equivalent naming of your binary file in the same directory structure in the main directory here.
3. Add `File: PATH\TO\BINARY.dll` file path as first line into the new `.patch` file.
4. Add a new line in there with the format: `OFFSET: NEW VALUE [ ORIGINAL VALUE ]`
    - Offset is hexa-decimal without leading `0x`
    - Values can be anything from integers, floats, or pure hexa-decimal byte representations
5. Put a comment above this line describing what this patch does and who made it: `# Does foo bar. - Skotty`
6. Open `patch.bat` and add the new `.patch` file to be processed by `bwpatchw.exe`.
7. In case the directory structure was newly added, add an respective `xcopy` line to make the patched file be transferred over to the actual `Freelancer` directory.

### To Existing File

1. Find the `.patch` file of the binary the patches should be applied to.
2. Add a new line in there with the format: `OFFSET: NEW VALUE [ ORIGINAL VALUE ]`
    - Offset is hexa-decimal without leading `0x`
    - Values can be anything from integers, floats, or pure hexa-decimal byte representations
3. Put a comment above this line describing what this patch does and who made it: `# Does foo bar. - Skotty`

## `bwpatchw` Usage

Execute `bwpatchw.exe --help` in a command window to get usage information about it.