#include "Unknwn.h"
#include "MinHook.h"
#include <windows.h>


#if _WIN64
#pragma comment(lib, "libMinHook.x64.lib")

#else
#pragma comment(lib, "libMinHook.x86.lib")
#endif

// --- Constants ---
static constexpr DWORD MAX_SUPPORTED_CORES = 12;
static HMODULE hOriginalDInput8 = NULL;

// --- Original function pointers ---
typedef void(WINAPI* RealGetSystemInfo)(LPSYSTEM_INFO lpSystemInfo);
typedef BOOL(WINAPI* RealGetLogicalProcessorInformation)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength);
using DirectInput8CreateFunctionType = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);


RealGetSystemInfo fpRealGetSystemInfo = nullptr;
RealGetLogicalProcessorInformation fpRealGetLogicalProcessorInformation = nullptr;
DirectInput8CreateFunctionType fpOriginalDirectInput8Create = nullptr;

// --- hooks ---
void WINAPI GetSystemInfoDetour(LPSYSTEM_INFO info)
{
    fpRealGetSystemInfo(info);
    if (info->dwNumberOfProcessors > MAX_SUPPORTED_CORES)
        info->dwNumberOfProcessors = MAX_SUPPORTED_CORES;
}

BOOL WINAPI GetLogicalProcessorInformationDetour(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength)
{
    const auto result = fpRealGetLogicalProcessorInformation(Buffer, ReturnedLength);
    if (result == TRUE && *ReturnedLength > MAX_SUPPORTED_CORES)
    {
        *ReturnedLength = MAX_SUPPORTED_CORES;
    }
    return result;
}

// --- MinHook initialization ---
BOOL Init(HINSTANCE hModule)
{
    if (MH_Initialize() != MH_OK) {
        return FALSE;
    }

    if (MH_CreateHook(&GetSystemInfo, &GetSystemInfoDetour, reinterpret_cast<LPVOID*>(&fpRealGetSystemInfo)) != MH_OK) {
        return FALSE;
    }

    if (MH_CreateHook(&GetLogicalProcessorInformation, &GetLogicalProcessorInformationDetour, reinterpret_cast<LPVOID*>(&fpRealGetLogicalProcessorInformation)) != MH_OK) {
        return FALSE;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize(); // Cleanup on failure.
        return FALSE;
    }

    return TRUE;
}
// --- DLL entry point ---
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        return Init(hModule);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) {
            return FALSE;
        }

        if (MH_Uninitialize() != MH_OK) {
            return FALSE;
        }
        break;
    }
    return TRUE;
}

// Function to check if the OS is 64-bit.
static BOOL Is64BitOS()
{
    using IsWow64ProcessFunctionType = BOOL(WINAPI*)(HANDLE, PBOOL);

    HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32"));

    if (hKernel32 == NULL)
    {
        RaiseException(EXCEPTION_INVALID_HANDLE, EXCEPTION_NONCONTINUABLE, 0, NULL);
        return FALSE;
    }

    const auto isWow64Process = reinterpret_cast<IsWow64ProcessFunctionType>(GetProcAddress(hKernel32, "IsWow64Process"));

    BOOL is64Bit = FALSE;
    if (isWow64Process != NULL)
    {
        isWow64Process(GetCurrentProcess(), &is64Bit);
    }
    return is64Bit;
}


// This function will proxy the call to the original DINPUT8.dll.
extern "C" HRESULT WINAPI DirectInput8Create(
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID* ppvOut,
    LPUNKNOWN punkOuter)
{
    // Check if the original DLL has been loaded.
    if (hOriginalDInput8 == NULL)
    {
        const WCHAR* originalDllPath;
        if (Is64BitOS())
        {
            // Path for 32-bit processes on a 64-bit OS.
            originalDllPath = L"C:\\Windows\\SysWOW64\\DINPUT8.dll";
        }
        else
        {
            // Path for 32-bit processes on a 32-bit OS.
            originalDllPath = L"C:\\Windows\\System32\\DINPUT8.dll";
        }

        // Load the original DINPUT8.dll.
        hOriginalDInput8 = LoadLibraryW(originalDllPath);
        if (hOriginalDInput8 == NULL)
        {
            return E_FAIL;
        }

        // Get the address of the original DirectInput8Create function.
        fpOriginalDirectInput8Create = reinterpret_cast<DirectInput8CreateFunctionType>(
            GetProcAddress(hOriginalDInput8, "DirectInput8Create"));
        if (fpOriginalDirectInput8Create == NULL)
        {
            FreeLibrary(hOriginalDInput8);
            return E_FAIL;
        }
    }

    // Call the original function.
    return fpOriginalDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}