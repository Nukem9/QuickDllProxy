#include <type_traits>
#include <windows.h>

#if !__cpp_constexpr || !__cpp_constinit || !__cpp_concepts
#error This file requires a compiler with with C++20 support or newer for constexpr, constinit, and concept features.
#endif

#if !defined(_M_IX86) && !defined(_M_X64)
#error Unsupported architecture.
#endif

#if !defined(DLL_PROXY_DECLARE_IMPLEMENTATION)
#error Do not include this file directly. Include DllProxy.h instead.
#endif

#if !defined(DLL_PROXY_EXPORT_LISTING_FILE)
#error DLL_PROXY_EXPORT_LISTING_FILE must be defined before using DLL_PROXY_DECLARE_IMPLEMENTATION.
#endif

#if !defined(DLL_PROXY_LIBRARY_RESOLVER_CALLBACK)
#define DLL_PROXY_LIBRARY_RESOLVER_CALLBACK DllProxy::DefaultLibraryResolverCallback
#endif

#if !defined(DLL_PROXY_EXPORT_RESOLVER_CALLBACK)
#define DLL_PROXY_EXPORT_RESOLVER_CALLBACK DllProxy::DefaultExportResolverCallback
#endif

#if !defined(DLL_PROXY_EXCEPTION_CALLBACK)
#define DLL_PROXY_EXCEPTION_CALLBACK DllProxy::DefaultExceptionCallback
#endif

#define DLL_PROXY_STRINGIFY(X)		#X
#define DLL_PROXY_CONCAT_IMPL(X, Y) X##Y
#define DLL_PROXY_CONCAT(X, Y)		DLL_PROXY_CONCAT_IMPL(X, Y)

#if defined(_MSC_VER)
#define DLL_PROXY_MAKE_SECTION(Name)				__pragma(section(Name, read))
#define DLL_PROXY_USE_SECTION(Name)					__declspec(allocate(Name))
#define DLL_PROXY_ALIAS_SYMBOL(From, To)			__pragma(comment(linker, "/ALTERNATENAME:" To "=" From))
#define DLL_PROXY_EXPORT_SYMBOL(From, To)			__pragma(comment(linker, "/EXPORT:" To "=" From))
#define DLL_PROXY_EXPORT_ORDINAL(From, To, Ordinal) __pragma(comment(linker, "/EXPORT:" To "=" From ",@" #Ordinal))
#elif defined(__GNUC__) // _MSC_VER
#define DLL_PROXY_MAKE_SECTION(Name)				__asm__(".pushsection \"" Name "\", \"xr\"\n.balign 4096\n.popsection\n");
#define DLL_PROXY_USE_SECTION(Name)					__attribute__((section(Name), used))
#define DLL_PROXY_ALIAS_SYMBOL(From, To)			__asm__(".global \"" To "\"\n.set \"" To "\", " From);
#if defined(_M_IX86)
#define DLL_PROXY_SYMBOL_PREFIX "_"
#else
#define DLL_PROXY_SYMBOL_PREFIX ""
#endif
#define DLL_PROXY_EXPORT_SYMBOL(From, To)			DLL_PROXY_ALIAS_SYMBOL(From, DLL_PROXY_SYMBOL_PREFIX To)
#define DLL_PROXY_EXPORT_ORDINAL(From, To, Ordinal) DLL_PROXY_ALIAS_SYMBOL(From, DLL_PROXY_SYMBOL_PREFIX To)
#else // __GNUC__
#error Unsupported compiler.
#endif

namespace DllProxy::Internal
{
	void UnresolvedExportCallback();
	void WINAPI TLSInitCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved);
}

//
// ----------------------------------------------------------------
// -             Custom export section implementation             -
// ----------------------------------------------------------------
//
namespace DllProxy::Section
{
	constexpr size_t DefaultPageAlign = 4096;
	constexpr size_t DefaultFuncAlign = 16;

	//
	// .dllprox$a - Beginning of the segment
	//
	DLL_PROXY_MAKE_SECTION(".dllprox$a")
	alignas(DefaultPageAlign) DLL_PROXY_USE_SECTION(".dllprox$a") constinit uint8_t ExportBoundaryStart = 1;

	//
	// .dllprox$b - Executable assembly code
	//
	DLL_PROXY_MAKE_SECTION(".dllprox$b")

#if defined(_M_IX86)
	
	#pragma pack(push, 1)
	struct X86StubPlaceholderCode
	{
		uint8_t JmpDwordOpcode[2]; // 0xFF 0x25
		void *JmpDwordEipAddress;  // 0xDEADBEEF
		void *JmpDwordDestination; // 0xDEADC0DE
		uint8_t Padding[6];		   // 0xCC 0xCC 0xCC 0xCC 0xCC 0xCC
	};
	static_assert(sizeof(X86StubPlaceholderCode) == 16, "Opcodes are expected to be 16 bytes");
#pragma pack(pop)

	#define MAKE_PROXY_EXPORT_IMPL(PlaceholderName)	\
		extern "C"									\
		alignas(DefaultFuncAlign)					\
		DLL_PROXY_USE_SECTION(".dllprox$b")			\
		constinit									\
		X86StubPlaceholderCode PlaceholderName { 0xFF, 0x25, &PlaceholderName.JmpDwordDestination, (void *)&Internal::UnresolvedExportCallback, 0xCC, 0xCC , 0xCC, 0xCC , 0xCC, 0xCC }; \
		DLL_PROXY_ALIAS_SYMBOL(DLL_PROXY_STRINGIFY(_##PlaceholderName), #PlaceholderName)

#elif defined(_M_X64) // _M_IX86
	
	#pragma pack(push, 1)
	struct Amd64StubPlaceholderCode
	{
		uint8_t JmpQwordOpcode[2]; // 0xFF 0x25
		int32_t JmpQwordRipOffset; // 0x00000000
		void *JmpQwordDestination; // 0xDEADC0DEDEADC0DE
		uint8_t Padding[2];		   // 0xCC 0xCC
	};
	static_assert(sizeof(Amd64StubPlaceholderCode) == 16, "Opcodes are expected to be 16 bytes");
#pragma pack(pop)

	#define MAKE_PROXY_EXPORT_IMPL(PlaceholderName)	\
		extern "C"									\
		alignas(DefaultFuncAlign)					\
		DLL_PROXY_USE_SECTION(".dllprox$b")			\
		constinit									\
		Amd64StubPlaceholderCode PlaceholderName { 0xFF, 0x25, 0, (void *)&Internal::UnresolvedExportCallback, 0xCC, 0xCC };

#endif // _M_X64

#define MAKE_PROXY_API_IMPL(FuncName, VariableAlias) \
	MAKE_PROXY_EXPORT_IMPL(VariableAlias)            \
	DLL_PROXY_EXPORT_SYMBOL(DLL_PROXY_STRINGIFY(VariableAlias), FuncName)

#define MAKE_PROXY_API_ORDINAL_IMPL(FuncName, Ordinal, VariableAlias) \
	MAKE_PROXY_EXPORT_IMPL(VariableAlias)                             \
	DLL_PROXY_EXPORT_ORDINAL(DLL_PROXY_STRINGIFY(VariableAlias), FuncName, Ordinal)

#define DECLARE_PROXIED_API(FuncName) \
	MAKE_PROXY_API_IMPL(FuncName, DLL_PROXY_CONCAT(XPROXY_, __COUNTER__))

#define DECLARE_PROXIED_API_ORDINAL(FuncName, Ordinal) \
	MAKE_PROXY_API_ORDINAL_IMPL(FuncName, Ordinal, DLL_PROXY_CONCAT(XPROXY_, __COUNTER__))

#define DECLARE_PROXIED_LIBRARY(LibraryName) \
	constexpr auto OriginalLibraryName = L##LibraryName;

#include DLL_PROXY_EXPORT_LISTING_FILE

#undef DECLARE_PROXIED_LIBRARY
#undef DECLARE_PROXIED_API_ORDINAL
#undef DECLARE_PROXIED_API
#undef MAKE_PROXY_API_ORDINAL_IMPL
#undef MAKE_PROXY_API_IMPL
#undef MAKE_PROXY_EXPORT_IMPL

	//
	// .dllprox$z - End of the segment
	//
	DLL_PROXY_MAKE_SECTION(".dllprox$z")
	alignas(DefaultPageAlign) DLL_PROXY_USE_SECTION(".dllprox$z") constinit uint8_t ExportBoundaryEnd = 1;

	//
	// Merge ".dllprox" into ".text" with identical attributes
	//
#if defined(_MSC_VER)
	#pragma comment(linker, "/MERGE:.dllprox=.text")
#endif
}

//
// ----------------------------------------------------------------
// -         Thread local storage callback implementation         -
// ----------------------------------------------------------------
//
namespace DllProxy::TLS
{
#if defined(DLL_PROXY_TLS_CALLBACK_AUTOINIT)
#if defined(_MSC_VER)
#if defined(_M_IX86)

	#pragma data_seg(push, proxdata, ".CRT$XLB")
	extern "C" PIMAGE_TLS_CALLBACK DllProxyTLSCallback = Internal::TLSInitCallback;
	#pragma data_seg(pop, proxdata)
	#pragma comment(linker, "/INCLUDE:__tls_used")
	#pragma comment(linker, "/INCLUDE:_DllProxyTLSCallback")

#elif defined(_M_X64) // _M_IX86

	#pragma const_seg(push, proxconst, ".CRT$XLB")
	extern "C" constexpr PIMAGE_TLS_CALLBACK DllProxyTLSCallback = Internal::TLSInitCallback;
	#pragma const_seg(pop, proxconst)
	#pragma comment(linker, "/INCLUDE:_tls_used")
	#pragma comment(linker, "/INCLUDE:DllProxyTLSCallback")

#endif // _M_X64
#elif defined(__GNUC__) // _MSC_VER
#if defined(_M_IX86)

	DLL_PROXY_USE_SECTION(".CRT$XLB") extern "C" PIMAGE_TLS_CALLBACK DllProxyTLSCallback = Internal::TLSInitCallback;

#elif defined(_M_X64) // _M_IX86

	DLL_PROXY_USE_SECTION(".CRT$XLB") extern "C" constexpr PIMAGE_TLS_CALLBACK DllProxyTLSCallback = Internal::TLSInitCallback;

#endif // _M_X64
#endif // __GNUC__
#endif // DLL_PROXY_TLS_CALLBACK_AUTOINIT
}

//
// ----------------------------------------------------------------
// -            Runtime export resolver implementation            -
// ----------------------------------------------------------------
//
namespace DllProxy::Internal
{
	constexpr PfnRealLibraryResolver RealLibraryResolver = ::DLL_PROXY_LIBRARY_RESOLVER_CALLBACK;
	constexpr PfnRealExportResolver RealExportResolver = ::DLL_PROXY_EXPORT_RESOLVER_CALLBACK;
	constexpr PfnExceptionCallback ExceptionCallback = ::DLL_PROXY_EXCEPTION_CALLBACK;

	void InitializeImpl();
	
	__declspec(noinline) void UnrecoverableError(ErrorCode Code)
	{
		ExceptionCallback(Code);

		__debugbreak();
#if defined(_MSC_VER)
		__assume(0);
#elif defined(__GNUC__)
		__builtin_unreachable();
#endif
	}

	void UnresolvedExportCallback()
	{
		UnrecoverableError(ErrorCode::ExportNotResolved);
	}

	void WINAPI TLSInitCallback(PVOID DllHandle, DWORD Reason, PVOID Reserved)
	{
		if (Reason == DLL_PROCESS_ATTACH)
			InitializeImpl();
	}

	__declspec(noinline) void *GetLocalModuleHandle()
	{
		constinit static void *moduleHandle = nullptr;

		if (!moduleHandle)
		{
			uint32_t flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;
			HMODULE handle = nullptr;

			if (!GetModuleHandleExW(flags, reinterpret_cast<LPCWSTR>(&GetLocalModuleHandle), &handle))
				UnrecoverableError(ErrorCode::InvalidModuleBase);

			InterlockedExchangePointer(&moduleHandle, handle);
		}

		return moduleHandle;
	}

	template<typename T>
	T ConvertModuleRva(uintptr_t Rva)
	{
		return (T)(reinterpret_cast<uintptr_t>(GetLocalModuleHandle()) + Rva);
	}

	// Pass the callback as a templated type to avoid the need for std::function's allocations
	template<typename T>
	requires(std::is_invocable_v<T, uint32_t /* Ordinal */, const char */* Name */, uintptr_t /* Address */>)
	void EnumerateLocalModuleExports(T&& Callback)
	{
		auto dosHeader = ConvertModuleRva<const IMAGE_DOS_HEADER *>(0);

		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			UnrecoverableError(ErrorCode::InvalidModuleHeaders);

		auto ntHeaders = ConvertModuleRva<const IMAGE_NT_HEADERS *>(dosHeader->e_lfanew);

		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE ||
			ntHeaders->FileHeader.SizeOfOptionalHeader < sizeof(IMAGE_OPTIONAL_HEADER))
			UnrecoverableError(ErrorCode::InvalidModuleHeaders);

		auto exportDataDirectory = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		auto exportDirectory = ConvertModuleRva<const IMAGE_EXPORT_DIRECTORY *>(exportDataDirectory->VirtualAddress);

		if (exportDataDirectory->VirtualAddress == 0 || exportDataDirectory->Size <= 0)
			UnrecoverableError(ErrorCode::InvalidModuleHeaders);

		auto exportAddressTable = ConvertModuleRva<const uint32_t *>(exportDirectory->AddressOfFunctions);
		auto exportNameTable = ConvertModuleRva<const uint32_t *>(exportDirectory->AddressOfNames);
		auto exportNameOrdinalTable = ConvertModuleRva<const uint16_t *>(exportDirectory->AddressOfNameOrdinals);

		// Ordinals are just an index. Names might not be present.
		for (uint32_t i = 0; i < exportDirectory->NumberOfFunctions; i++)
		{
			const uint32_t exportOrdinal = exportDirectory->Base + i;
			const auto exportName = [&]() -> const char *
			{
				for (uint32_t j = 0; j < exportDirectory->NumberOfNames; j++)
				{
					if (exportNameOrdinalTable[j] == i)
						return ConvertModuleRva<const char *>(exportNameTable[j]);
				}

				return nullptr;
			}();

			Callback(exportOrdinal, exportName, ConvertModuleRva<uintptr_t>(exportAddressTable[i]));
		}
	}

	void InitializeImpl()
	{
		// Load the original DLL. If it's not available, we can't do anything.
		const auto originalModule = RealLibraryResolver();

		if (!originalModule)
			UnrecoverableError(ErrorCode::LibraryNotFound);

		// Then unprotect the entire proxy segment for writing
		const auto sectionStart = reinterpret_cast<uintptr_t>(&Section::ExportBoundaryStart);
		const auto sectionEnd = reinterpret_cast<uintptr_t>(&Section::ExportBoundaryEnd);
		const auto sectionLength = sectionEnd - sectionStart;
		DWORD oldProtection = 0;

		if (!VirtualProtect(reinterpret_cast<void *>(sectionStart), sectionLength, PAGE_READWRITE, &oldProtection))
			UnrecoverableError(ErrorCode::VirtualProtectFailed);

		// Iterate over all exports from this DLL. If the RVA points into the dllprox section, patch the jump to
		// point back to the original library.
		EnumerateLocalModuleExports([&](uint32_t Ordinal, const char *Name, uintptr_t Address)
		{
			if (Address > sectionStart && Address < sectionEnd)
			{
				void *functionPointer = nullptr;
				const bool status = RealExportResolver(originalModule, Ordinal, Name, &functionPointer);

#if defined(DLL_PROXY_CHECK_MISSING_EXPORTS)
				if (!status)
					UnrecoverableError(ErrorCode::ExportNotFound);
#endif

				// functionPointer is allowed to be null as long as success is reported. Users can hurt themselves as they please.
				if (status)
				{
#if defined(_M_IX86)
					auto ptr = &reinterpret_cast<Section::X86StubPlaceholderCode *>(Address)->JmpDwordDestination;

					static_assert(sizeof(*ptr) == sizeof(LONG), "Expected pointer to be 32 bits");
					InterlockedExchange(reinterpret_cast<LONG *>(ptr), reinterpret_cast<LONG>(functionPointer));
#elif defined(_M_X64) // _M_IX86
					auto __unaligned ptr = &reinterpret_cast<Section::Amd64StubPlaceholderCode *>(Address)->JmpQwordDestination;

					static_assert(sizeof(*ptr) == sizeof(LONGLONG), "Expected pointer to be 64 bits");
					InterlockedExchange64(reinterpret_cast<LONGLONG *>(ptr), reinterpret_cast<LONGLONG>(functionPointer));
#endif // _M_X64
				}
			}
		});

		// Reprotect. Done.
		if (!VirtualProtect(reinterpret_cast<void *>(sectionStart), sectionLength, oldProtection, &oldProtection))
			UnrecoverableError(ErrorCode::VirtualProtectFailed);

		FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void *>(sectionStart), sectionLength);
	}
}

//
// ----------------------------------------------------------------
// -                  Public API implementation                   -
// ----------------------------------------------------------------
//
namespace DllProxy
{
#if !defined(DLL_PROXY_TLS_CALLBACK_AUTOINIT)
	void Initialize()
	{
		Internal::InitializeImpl();
	}
#endif

	void *DefaultLibraryResolverCallback()
	{
		// Try to load the DLL with the user-specified path. In the event it's not found, there's nothing left to
		// do here.
		HMODULE libraryHandle = LoadLibraryW(Section::OriginalLibraryName);

		if (!libraryHandle)
			return nullptr;

		// Prevent infinite recursion when module names collide. Make sure this DLL doesn't grab a reference to itself.
		if (libraryHandle == Internal::GetLocalModuleHandle())
		{
			FreeLibrary(libraryHandle);

			// Try system32 instead
			const size_t bufferLength = 4096;
			auto buffer = reinterpret_cast<wchar_t *>(VirtualAlloc(nullptr, bufferLength * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

			if (!buffer)
				return nullptr;

			if (GetSystemDirectoryW(buffer, bufferLength - 1) <= 0)
			{
				VirtualFree(buffer, 0, MEM_RELEASE);
				return nullptr;
			}

			wcscat_s(buffer, bufferLength, L"\\");
			wcscat_s(buffer, bufferLength, Section::OriginalLibraryName);

			libraryHandle = LoadLibraryW(buffer);
			VirtualFree(buffer, 0, MEM_RELEASE);
		}

		return libraryHandle;
	}

	bool DefaultExportResolverCallback(void *Module, uint32_t Ordinal, const char *Name, void **FunctionPointer)
	{
		void *pointer = nullptr;

		if (Name)
			pointer = reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(Module), Name));

		if (!pointer)
			pointer = reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(Module), MAKEINTRESOURCEA(Ordinal)));

		if (!pointer)
			return false;

		*FunctionPointer = pointer;
		return true;
	}

	void DefaultExceptionCallback(ErrorCode Code)
	{
		// Avoid a hard dependency on user32.dll
		if (const HMODULE user32 = LoadLibraryW(L"user32.dll"))
		{
			const auto messageBox = reinterpret_cast<decltype(&MessageBoxW)>(GetProcAddress(user32, "MessageBoxW"));

			if (messageBox)
			{
				wchar_t reason[] = L"Proxy error code during initialization: 0";
				reason[ARRAYSIZE(reason) - 2] = L'0' + static_cast<uint8_t>(Code);

				messageBox(nullptr, reason, L"Proxy DLL Fatal Error", MB_ICONERROR);
			}
		}

		TerminateProcess(GetCurrentProcess(), 0xBADC0DE0 | static_cast<uint8_t>(Code));
	}
}
