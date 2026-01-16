# FLHook Plugin SDK

**N.B.: This project is a subproject of https://github.com/Friendly0Fire/FLHook; `FLHook.lib` must be added from a compiled version of FLHook in order for the plugin SDK to be functional.**

## Introduction

Every plugin is a DLL that exports certain functions. Those functions are called by FLHook. For example, if someone sends a chat message, SubmitChat is called withing FLHook. FLHook will then check the plugins and if a plugin exports the same function, that function within that plugin is called. So, basically, FLHook hooks the flserver and then calls the plugins.

PLEASE NOTE: The source files for the advanced connection and tempban plugins are included in https://github.com/Friendly0Fire/FLHook. They are commented and should give you a good idea of how to write plugins.

For development help, please refer to https://the-starport.net (check out the FLHook development forums)!

## Plugin Development

### Plugin info

Your plugin DLL needs to be located directly in the `.\EXE\flhook_plugins` folder. It needs to export `Get_PluginInfo` which replaces the old plugin INI file. Normally the function `Get_PluginInfo` should look something like this (using `tempban.dll` as example):

```cpp
EXPORT PLUGIN_INFO* Get_PluginInfo()
{
	PLUGIN_INFO* p_PI = new PLUGIN_INFO();
	p_PI->sName = "TempBan Plugin by w0dk4";
	p_PI->sShortName = "tempban";
	p_PI->bMayPause = true;
	p_PI->bMayUnload = true;
	p_PI->ePluginReturnCode = &returncode;
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkTimerCheckKick, PLUGIN_HkTimerCheckKick, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&HkIServerImpl::Login, PLUGIN_HkIServerImpl_Login, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&Plugin_Communication_CallBack, PLUGIN_Plugin_Communication, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&ExecuteCommandString_Callback, PLUGIN_ExecuteCommandString_Callback, 0));
	p_PI->lstHooks.push_back(PLUGIN_HOOKINFO((FARPROC*)&CmdHelp_Callback, PLUGIN_CmdHelp_Callback, 0));
	return p_PI;
}
```

You should check the example plugins in order to get used to the plugin info function.

The `p_PI->lstHooks` part needs further explanation: This is where you tell FLHook which functions you want to have hooked into your plugin. You must export all the functions you list here. You can see a complete list of all available hooks along with a short description of what they do (if available) in `plugin.h`. The last parameter is a priority setting. This defines the order in which multiple plugins are called on the same function. For example, if an anticheat plugin hooks a function with a priority of `3` and a simple logging plugin hooks the same function with a priority of `0`, the anticheat plugin will receive the callback before the logging plugin.

The `ePluginReturnCode` is also important. It is a pointer to a global variable of type `PLUGIN_RETURNCODE`, an enum defined in the `plugin.h` header file, declared in your plugin. Here's an example of how it could be used:
```cpp
PLUGIN_RETURNCODE returncode;

EXPORT int __stdcall A_Hooked_Function(unsigned int iSomething)
{
	// do something here
	// now we don't want FLHook to call the original function and we don't want other plugins to be called either, so we change the return code
	returncode = SKIPPLUGINS_NOFUNCTIONCALL;
	return 0;
}
```

In the example above, FLHook will skip any plugin and return the current hooked function. If the return value is not void, it will return what you return (in this example, it's `0`). FLHook queries the value of your return code directly after your hooked function. You should now see the use of the priority setting in the plugin config: this way, high priority plugins can force FLHook to skip other plugins, as well as Freelancer's own function calls.

If you ever change the return code, you must **always** set the return code in every hooked function. It is *not* reset after a call. It's generally a good idea to always set it.

### Hooking functions

So, this is where the fun starts. First the very basics: Taking the example from above, the plugin defines a hook on `HkIServerImpl::Login`. Taking a look at the documentation about that hook gives us `void __stdcall Login(struct SLoginInfo const &li, unsigned int iClientID)`. This is what the function prototype must look like. You now need to define that function with a dll export, like this:

```cpp
namespace HkIServerImpl
{
	EXPORT void __stdcall Login(struct SLoginInfo const &li, unsigned int iClientID)
	{
		// do something here
		return;
	}
}
```

Some words about
```cpp
extern IMPORT bool g_bPlugin_nofunctioncall;
```
(in `plugin.h`): `g_bPlugin_nofunctioncall == true` indicates to your plugin that the original function should not be called because either it has been called by a previous plugin, or a previous plugin wants to prevent the function from being called. This adds a layer of compatability for plugin functions that need to be executed after certain server methods, like `IServerImpl::PlayerLaunch`.

### Plugin intercommunication

There are several header and library files provided with the SDK. They form the interface with which you can call original FLServer functions or functions exported by FLHook. Don't forget to include the library files to your linker dependencies. The `plugin.h` header file defines the return code enum and the inter-plugin communication interface. You can "talk" with other plugins by using the plugin communication function which is exported by FLHook. Here is an example:

```cpp
EXPORT void Plugin_Communication_CallBack(PLUGIN_MESSAGE msg, void* data)
{
	// this is the hooked plugin communication function

	// we now check, if the message is for us
	if(msg == DO_SOMETHING) {
		// the message is for us, now we know what the actual data is, so we do a reinterpret cast
		CUSTOM_STRUCT* incoming_data = reinterpret_cast<CUSTOM_STRUCT*>(data);

		// do something here with the received data & instruction
	}
	return;
}
```

In the example, `CUSTOM_STRUCT` can be any struct of any size. Please note: The
`Plugin_Communication_CallBack` function must be defined in `p_PI->mapHooks`, otherwise the function will not be called. The calling plugin (origin of the message) would call the communication function like this:

```cpp
CUSTOM_STRUCT outgoing_data;
Plugin_Communication(DO_SOMETHING, &outgoing_data);
```

### Debugging

To spot bugs in your plugins, you should always have a look at the `flhook.log` in `.\EXE\flhook_logs\`. If one of your callback functions throws an exception to FLHook, it is logged in this log file. Feel free to implement your own exception handling for more detailed error reporting.

**IMPORTANT NOTE:** If you want to debug your plugins and you are using STL classes that are being shared with FLHook (i.e. FLHook allocates a string and your plugin deallocates it) you *need* to make sure you set the compiler to use the "Multi-threaded DLL (/MD)" runtime library (in VC C++ project settings under "Code Generation"), otherwise you will get debug assertion crashes! You can also compile FLHook in debug mode and then also compile your plugin with "Multi-threaded Debug DLL (/MDd)". Just make sure, whatever runtime library setting you use, it *must* match the one of FLHook.

A special feature is the performance timer. The performance timer measures the time that is being spent in the different plugins. If the time is longer than a specified setting, it will be logged. You can alter that setting in `flhook.ini` as `TimerThreshold`.

## Reference

The SDK is an ever-evolving project, hence the best way to know what the current available features and functions are is to take a peek at the source. The `include` directory contains all available definitions:
 * `FLCore*.h` files contain their respective DLL/EXE's exports.
 * `plugin.h` contains plugin-specific defines, such as `PLUGIN_RETURNCODE`.
 * `st6.h` is the minimal reimplementation of the VC6 STL features used by Freelancer to ensure interoperability with more recent compilers.
 * `FLHook.h` contains all FLHook exports and defines and also automatically imports the other files for you.