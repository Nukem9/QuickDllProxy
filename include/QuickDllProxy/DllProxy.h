//
// ----------------------------------------------------------------
// -        Automatic DLL proxy generation and resolution         -
// -    (Emulation of behavior similar to delayed import DLLs)    -
// -                                                              -
// -           https://github.com/Nukem9/QuickDllProxy            -
// ----------------------------------------------------------------
//
//
// Example usage in a single .cpp file:
//
//   #define DLL_PROXY_EXPORT_LISTING_FILE "ExportListing.inc"   // List of exported functions
//   #define DLL_PROXY_TLS_CALLBACK_AUTOINIT                     // Enable automatic initialization
//   #define DLL_PROXY_DECLARE_IMPLEMENTATION                    // Define the whole implementation
//   #include <QuickDllProxy/DllProxy.h>
//
//
// User-configurable settings:
//
//   #define DLL_PROXY_EXPORT_LISTING_FILE "ExportListing.inc"
//     Required setting to define the list of proxied exports for this library.
//
//   #define DLL_PROXY_TLS_CALLBACK_AUTOINIT
//     Optional setting to create a TLS callback that runs DllProxy::Initialize.
//
//   #define DLL_PROXY_CHECK_MISSING_EXPORTS
//     Optional setting to enforce all real exports are found during DllProxy::Initialize. If disabled, unresolved
//     export errors will be deferred until proxy functions are invoked.
//
//   #define DLL_PROXY_LIBRARY_RESOLVER_CALLBACK MyExampleLibraryResolver
//     Optional setting to define a custom function that resolves a DLL's HMODULE. Signature identical to
//     DllProxy::DefaultLibraryResolverCallback.
//
//   #define DLL_PROXY_EXPORT_RESOLVER_CALLBACK MyExampleExportResolver
//     Optional setting to define a custom function that resolves a DLL's exported function. Signature identical to
//     DllProxy::DefaultExportResolverCallback.
//
//   #define DLL_PROXY_EXCEPTION_CALLBACK MyExampleExceptionCallback
//     Optional setting to define a custom function that's called on error. Signature identical to
//     DllProxy::DefaultExceptionCallback.
//
//
// Overview of the implementation:
//
//   1. Create a custom readable+executable PE section named ".dllprox". Its boundaries are defined by the virtual
//      addresses of DllProxy::Section::ExportBoundaryStart and DllProxy::Section::ExportBoundaryEnd, respectively.
//
//   2. Declare AMD64 assembly stubs that will later point to the original ("real") functions. These proxy functions
//      are placed in the custom section at compile time.
//
//   3. Manually tell the linker to export these stubs. Exports are aliased in order to avoid name collisions with
//      actual functions. For example, "PROXY_SymAllocDiaString" is aliased to "SymAllocDiaString" along with an
//      ordinal. This is done by using __pragma(linker) macros instead of a traditional module.def file.
//
//   4. Tell the linker to merge the ".dllprox" section into ".text".
//
//   5. At runtime DllProxy::Initialize patches a proxy stub's assembly opcodes such that it'll jump to the function
//      back in the original DLL. By default, if the original function cannot be located, the proxy stub will raise an
//      error only when invoked. If the developer defines DLL_PROXY_CHECK_MISSING_EXPORTS, the initialization routine
//      fails up front instead.
//
//   6. "At runtime" is determined by the developer. Ideally one would use a static constructor that's executed before
//      DllMain, but there's no hard requirement on where this happens. This implementation also provides an optional
//      TLS callback to run DllProxy::Initialize early on in the loader process via DLL_PROXY_TLS_CALLBACK_AUTOINIT.
//
//
// Developer notes:
//
//   Exported global variable forwarding isn't suppported.
//
//   DO NOT use CRT functions in callbacks. That includes printf, malloc, etc. The CRT may not be initialized. Certain
//   C++ STL types work as long as they don't allocate memory.
//
//   DO NOT read/write data in the ".dllprox" section through static C++ variables. The linker (LTCG) might optimize
//   constant loads away.
//
//   Control flow guard integrity is maintained only after initialization. "The VirtualProtect and VirtualAlloc
//   functions will by default treat a specified region of executable and committed pages as valid indirect call
//   targets."
//
//   Several atomic operations are used in the code. These are a byproduct of paranoia rather than thread safety.
//
//   "Using linker segments and __declspec(allocate(...)) to arrange data in a specific order"
//   https://devblogs.microsoft.com/oldnewthing/20181107-00/?p=100155
//
//   "Accessing the current module's HINSTANCE from a static library"
//   https://devblogs.microsoft.com/oldnewthing/20041025-00/?p=37483
//
//   "/EXPORT (Exports a Function)"
//   https://learn.microsoft.com/en-us/cpp/build/reference/export-exports-a-function?view=msvc-170
//
//
// Reference module.def format:
//
//   LIBRARY dbghelp
//   EXPORTS
//     Ordinal_1101 = PROXY_Ordinal_1101 @1101
//     SymAllocDiaString = PROXY_SymAllocDiaString @1111
//
#if !defined(DLLPROXY_H_DEDUP)
#define DLLPROXY_H_DEDUP

#include <cstdint>

namespace DllProxy
{
	enum class ErrorCode : std::uint8_t
	{
		/// <summary>
		/// Unable to query base address of this DLL.
		/// </summary>
		InvalidModuleBase = 0,

		/// <summary>
		/// Invalid DOS, NT, or export directory headers.
		/// </summary>
		InvalidModuleHeaders = 1,

		/// <summary>
		/// A call to VirtualProtect failed.
		/// </summary>
		VirtualProtectFailed = 2,

		/// <summary>
		/// Failed to load original module for proxying.
		/// </summary>
		LibraryNotFound = 3,

		/// <summary>
		/// Failed to locate an exported function in the original module.
		/// </summary>
		ExportNotFound = 4,

		/// <summary>
		/// A proxy export was called, but a real function pointer wasn't resolved yet.
		/// </summary>
		ExportNotResolved = 5,
	};

	using PfnRealLibraryResolver = void *(*)();
	using PfnRealExportResolver = bool(*)(void *Module, std::uint32_t Ordinal, const char *Name, void **FunctionPointer);
	using PfnExceptionCallback = void(*)(ErrorCode Code);

#if !defined(DLL_PROXY_TLS_CALLBACK_AUTOINIT)
	void Initialize();
#endif // !DLL_PROXY_TLS_CALLBACK_AUTOINIT
	
	void *DefaultLibraryResolverCallback();
	bool DefaultExportResolverCallback(void *Module, std::uint32_t Ordinal, const char *Name, void **FunctionPointer);
	void DefaultExceptionCallback(ErrorCode Code);
}

#endif // !DLLPROXY_H_DEDUP

#if defined(DLL_PROXY_DECLARE_IMPLEMENTATION)
#include "DllProxyImpl.inl"
#endif // DLL_PROXY_DECLARE_IMPLEMENTATION