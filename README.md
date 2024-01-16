# QuickDllProxy

A header-only DLL proxy stub generation library built with C++20. No more copy-pasting 100s of DLL exports. No more guessing function parameters.

For a somewhat detailed breakdown please [read DllProxy.h's header comments](include/QuickDllProxy/DllProxy.h) or [check out the implementation](include/QuickDllProxy/DllProxyImpl.inl).

## Configuration

```cpp
#define DLL_PROXY_EXPORT_LISTING_FILE "ExportListing.inc"
// Required setting to define the list of proxied exports for this library.

#define DLL_PROXY_TLS_CALLBACK_AUTOINIT
// Optional setting to create a TLS callback that runs DllProxy::Initialize.

#define DLL_PROXY_CHECK_MISSING_EXPORTS
// Optional setting to enforce all real exports are found during DllProxy::Initialize. If disabled, unresolved
// export errors will be deferred until proxy functions are invoked.

#define DLL_PROXY_LIBRARY_RESOLVER_CALLBACK MyExampleLibraryResolver
// Optional setting to define a custom function that resolves a DLL's HMODULE. Signature identical to
// DllProxy::DefaultLibraryResolverCallback.

#define DLL_PROXY_EXPORT_RESOLVER_CALLBACK MyExampleExportResolver
// Optional setting to define a custom function that resolves a DLL's exported function. Signature identical to 
// DllProxy::DefaultExportResolverCallback.

#define DLL_PROXY_EXCEPTION_CALLBACK MyExampleExceptionCallback
// Optional setting to define a custom function that's called on error. Signature identical to
// DllProxy::DefaultExceptionCallback.
```

## Example Usage

```cpp
// MySpecialDllFile.cpp
#define DLL_PROXY_EXPORT_LISTING_FILE "ExportListing.inc"   // List of exported functions
#define DLL_PROXY_TLS_CALLBACK_AUTOINIT                     // Enable automatic initialization TLS callback
#define DLL_PROXY_DECLARE_IMPLEMENTATION                    // Define the whole implementation
#include <QuickDllProxy/DllProxy.h>
```

```cpp
// ExportListing.inc
DECLARE_PROXIED_LIBRARY("winhttp.dll")
DECLARE_PROXIED_API_ORDINAL("WinHttpAddRequestHeaders", 6)
DECLARE_PROXIED_API_ORDINAL("WinHttpSetSecureLegacyServersAppCompat", 1)
DECLARE_PROXIED_API("TestFunction_1110")
```

## Demo Projects
- [WinHttp](example/demo_winhttp/dllmain.cpp)
- [DbgHelp](example/demo_dbghelp/proxy.cpp)

## Known Limitations
- Exported global variables aren't supported.
- Original exports are resolved at runtime within a TLS callback or DllMain(). In rare cases this is too late to be useful.
- Control flow guard targets aren't valid until after original export resolution.

## License

- [LGPLv3](LICENSE.md)
