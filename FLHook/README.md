# FL:SR Hook

This Hook is based on FL-Hook 2.1.

## For Developers

1. Download [Visual Studio Community](https://visualstudio.microsoft.com/de/vs/community/)
2. Select *Desktop Development with C++* in the main "Workloads" tab
3. In the tab "Individual Components", select
	- *Windows Universal C-Runtime*
	- *MSVC v142 x64/x86 Buildtools*
	- *C++ MFC for v142 Buildtools* (not the Spectre version)
4. Open the project file `project/FLHook.sln`
5. Tell Visual Studio to NOT upgrade the project
6. In Visual Studio, open the integrated terminal and run `vcpkg integrate install` to enable vcpkg for dependency download.

The build output is located in `bin/`.