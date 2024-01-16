#include <stdio.h>
#include <QuickDllProxy/DllProxy.h>                                    // Define the header only (forward declarations)

void *ResolveModule()
{
	return LoadLibraryA("C:\\windows\\system32\\winhttp.dll");
}

void ExampleHookedFunction()
{
}

bool ResolveFunction(void *Module, uint32_t Ordinal, const char *Name, void **Pointer)
{
    // Intercept a specific ordinal
    if (Ordinal == 23)
    {
		*Pointer = &ExampleHookedFunction;
		return true;
    }

    // Otherwise resolve it as normal through GetProcAddress
	auto ptr = GetProcAddress((HMODULE)Module, Name);

    if (!ptr)
        return false;

	*Pointer = ptr;
    return true;
}

void HandleException(DllProxy::ErrorCode Code)
{
	char reason[32];
	sprintf_s(reason, "Error code: %u", Code);

    MessageBoxA(nullptr, reason, "PROXY ERROR", MB_ICONERROR);
    __debugbreak();
}

#define DLL_PROXY_EXPORT_LISTING_FILE "winhttp_exports.inc"     // List of exported functions
//#define DLL_PROXY_TLS_CALLBACK_AUTOINIT
#define DLL_PROXY_CHECK_MISSING_EXPORTS                         // Strict validation
#define DLL_PROXY_LIBRARY_RESOLVER_CALLBACK ResolveModule       // Custom module resolver
#define DLL_PROXY_EXPORT_RESOLVER_CALLBACK ResolveFunction      // Custom export function resolver
#define DLL_PROXY_EXCEPTION_CALLBACK HandleException            // Custom error handler
#define DLL_PROXY_DECLARE_IMPLEMENTATION                        // Then define the whole implementation
#include <QuickDllProxy/DllProxy.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // Not using TLS autoinit, so call it manually
        DllProxy::Initialize();

        MessageBoxA(nullptr, "Hello world from proxy winhttp!", "DLL_PROCESS_ATTACH", MB_OK);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}
