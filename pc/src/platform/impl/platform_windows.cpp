#include "momentum/platform/impl/platform_windows.hpp"

#include "momentum/platform/common/crash_handler.hpp"
#include "momentum/utility/log.hpp"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <csignal>
#include <time.h>
#include <string>
#include <filesystem>

#include <dbghelp.h>
#include <stdarg.h>

#include <SDL3/SDL.h>

#pragma comment(lib, "dbghelp.lib")

FILENAME_LOGGER();

static CRITICAL_SECTION sLogLock;
static char sCrashDumpDir[512] = {};
static wchar_t sCrashDumpDirW[512] = {};
static wchar_t sCfgPathW[512] = {};

namespace momentum::platform::impl {

    void PlatformWindows::Shutdown() {
        ExitProcess(0);
    }

    std::string PlatformWindows::GetName() const {
        return "Windows (SDL)";
    }

    static BOOL WINAPI ConsoleCtrlHandler(DWORD ctrlType) {
        switch (ctrlType) {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            std::_Exit(130);
            return TRUE;
        default:
            return FALSE;
        }
    }

    void PlatformWindows::InstallEmergencyExit() {
        PlatformDesktop::InstallEmergencyExit();
        SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
    }

    void PlatformWindows::InstallCrashHandler() {
        std::string dir = momentum::platform::common::crash_handler::GetCrashDumpDirectory();
        snprintf(sCrashDumpDir, sizeof(sCrashDumpDir), "%s", dir.c_str());
        MultiByteToWideChar(CP_UTF8, 0, sCrashDumpDir, -1, sCrashDumpDirW, _countof(sCrashDumpDirW));

        std::string cfgUtf8 = momentum::platform::common::crash_handler::GetConfigPath();
        MultiByteToWideChar(CP_UTF8, 0, cfgUtf8.c_str(), -1, sCfgPathW, _countof(sCfgPathW));

        InitializeCriticalSection(&sLogLock);
        momentum::platform::common::crash_handler::sLogLockInit = true;
        SetUnhandledExceptionFilter(PlatformWindows::CrashHandler);
        LOG_INFO("Crash handler armed (dumps -> {})\n", sCrashDumpDir);
    }

    // === Private ===
    const char* PlatformWindows::ExceptionCodeToString(DWORD code) {
        switch (code) {
            case EXCEPTION_ACCESS_VIOLATION:      return "ACCESS_VIOLATION";
            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "ARRAY_BOUNDS_EXCEEDED";
            case EXCEPTION_STACK_OVERFLOW:        return "STACK_OVERFLOW";
            case EXCEPTION_ILLEGAL_INSTRUCTION:   return "ILLEGAL_INSTRUCTION";
            case EXCEPTION_INT_DIVIDE_BY_ZERO:    return "INT_DIVIDE_BY_ZERO";
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:    return "FLT_DIVIDE_BY_ZERO";
            default:                              return "UNKNOWN";
        }
    }

    void PlatformWindows::WriteGameState(FILE* log) {
        WriteSdlGameState(log);
    }

    void PlatformWindows::WriteSystemInfo(FILE* log) {
        fprintf(log, "\nSystem:\n");

        MEMORYSTATUSEX mem = {};
        mem.dwLength = sizeof(mem);
        if (GlobalMemoryStatusEx(&mem)) {
            fprintf(log, "  RAM: %llu MB total, %llu MB available\n",
                    mem.ullTotalPhys / (1024*1024), mem.ullAvailPhys / (1024*1024));
        }

        OSVERSIONINFOEXA osvi = {};
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        fprintf(log, "  OS: Windows %d.%d build %d\n",
                osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

        SYSTEM_INFO si;
        GetSystemInfo(&si);
        fprintf(log, "  Processors: %u\n", si.dwNumberOfProcessors);
        fprintf(log, "  Architecture: %s\n",
                si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" :
                si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ? "x86" : "other");
    }

    void WriteConfig(FILE* log) {
        FILE* cfg = sCfgPathW[0] ? _wfopen(sCfgPathW, L"r") : nullptr;
        if (!cfg) {
            fprintf(log, "\nConfig: (no config.ini found)\n");
            return;
        }
        fprintf(log, "\nConfig (config.ini):\n");
        char line[512];
        while (fgets(line, sizeof(line), cfg)) {
            char* nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            fprintf(log, "  %s\n", line);
        }
        fclose(cfg);
    }

    LONG WINAPI PlatformWindows::CrashHandler(EXCEPTION_POINTERS* ep) {
        wchar_t wLogPath[600];
        wchar_t wDmpPath[600];
        const wchar_t* base = sCrashDumpDirW[0] ? sCrashDumpDirW : L".";

        time_t now = time(NULL);
        struct tm t;
        localtime_s(&t, &now);

        _snwprintf_s(wLogPath, _countof(wLogPath), _TRUNCATE,
                     L"%s\\crash_%04d%02d%02d_%02d%02d%02d.txt",
                     base, t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        _snwprintf_s(wDmpPath, _countof(wDmpPath), _TRUNCATE,
                     L"%s\\crash_%04d%02d%02d_%02d%02d%02d.dmp",
                     base, t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

        char logPath[600];
        WideCharToMultiByte(CP_UTF8, 0, wLogPath, -1, logPath, sizeof(logPath), nullptr, nullptr);
        char dmpPath[600];
        WideCharToMultiByte(CP_UTF8, 0, wDmpPath, -1, dmpPath, sizeof(dmpPath), nullptr, nullptr);

        HANDLE dmpFile = CreateFileW(wDmpPath, GENERIC_WRITE, 0, NULL,
                                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (dmpFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION mei;
            mei.ThreadId          = GetCurrentThreadId();
            mei.ExceptionPointers = ep;
            mei.ClientPointers    = FALSE;
            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dmpFile,
                              MiniDumpWithDataSegs, &mei, NULL, NULL);
            CloseHandle(dmpFile);
        }

        FILE* log = _wfopen(wLogPath, L"w");
        if (log) {
            fprintf(log, "=== CRASH REPORT ===\n");
            fprintf(log, "Version: %s\n", APP_VERSION);
            fprintf(log, "Build:   %s %s\n", __DATE__, __TIME__);

            char timeBuf[64];
            ctime_s(timeBuf, sizeof(timeBuf), &now);
            fprintf(log, "Time:    %s", timeBuf);
            fprintf(log, "==========================================\n");

            DWORD code = ep->ExceptionRecord->ExceptionCode;
            fprintf(log, "\nException: 0x%08X (%s)\n", code, ExceptionCodeToString(code));
            fprintf(log, "Address:   %p\n", ep->ExceptionRecord->ExceptionAddress);

            if (code == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2) {
                const char* op = ep->ExceptionRecord->ExceptionInformation[0] == 0 ? "reading" : "writing";
                fprintf(log, "Fault:     %s address 0x%016llX\n", op,
                        (unsigned long long)ep->ExceptionRecord->ExceptionInformation[1]);
            }

            CONTEXT* ctx = ep->ContextRecord;
            fprintf(log, "\nRegisters:\n");
    #if defined(_M_X64) || defined(_WIN64)
            fprintf(log, "  RAX=%016llX  RBX=%016llX  RCX=%016llX  RDX=%016llX\n",
                    (unsigned long long)ctx->Rax, (unsigned long long)ctx->Rbx,
                    (unsigned long long)ctx->Rcx, (unsigned long long)ctx->Rdx);
            fprintf(log, "  RSI=%016llX  RDI=%016llX  RBP=%016llX  RSP=%016llX\n",
                    (unsigned long long)ctx->Rsi, (unsigned long long)ctx->Rdi,
                    (unsigned long long)ctx->Rbp, (unsigned long long)ctx->Rsp);
            fprintf(log, "  R8 =%016llX  R9 =%016llX  R10=%016llX  R11=%016llX\n",
                    (unsigned long long)ctx->R8,  (unsigned long long)ctx->R9,
                    (unsigned long long)ctx->R10, (unsigned long long)ctx->R11);
            fprintf(log, "  R12=%016llX  R13=%016llX  R14=%016llX  R15=%016llX\n",
                    (unsigned long long)ctx->R12, (unsigned long long)ctx->R13,
                    (unsigned long long)ctx->R14, (unsigned long long)ctx->R15);
            fprintf(log, "  RIP=%016llX  EFLAGS=%08X\n",
                    (unsigned long long)ctx->Rip, (unsigned)ctx->EFlags);
    #else
            fprintf(log, "  EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X\n",
                    ctx->Eax, ctx->Ebx, ctx->Ecx, ctx->Edx);
            fprintf(log, "  ESI=%08X  EDI=%08X  EBP=%08X  ESP=%08X\n",
                    ctx->Esi, ctx->Edi, ctx->Ebp, ctx->Esp);
            fprintf(log, "  EIP=%08X  EFLAGS=%08X\n", ctx->Eip, ctx->EFlags);
    #endif

            fprintf(log, "\nStack Trace:\n");
            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
            HANDLE proc = GetCurrentProcess();
            SymInitialize(proc, NULL, TRUE);

            STACKFRAME64 frame = {};
    #if defined(_M_X64) || defined(_WIN64)
            frame.AddrPC.Offset    = ctx->Rip;    frame.AddrPC.Mode    = AddrModeFlat;
            frame.AddrFrame.Offset = ctx->Rbp;    frame.AddrFrame.Mode = AddrModeFlat;
            frame.AddrStack.Offset = ctx->Rsp;    frame.AddrStack.Mode = AddrModeFlat;
            const DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
    #else
            frame.AddrPC.Offset    = ctx->Eip;    frame.AddrPC.Mode    = AddrModeFlat;
            frame.AddrFrame.Offset = ctx->Ebp;    frame.AddrFrame.Mode = AddrModeFlat;
            frame.AddrStack.Offset = ctx->Esp;    frame.AddrStack.Mode = AddrModeFlat;
            const DWORD machineType = IMAGE_FILE_MACHINE_I386;
    #endif

            char symBuf[sizeof(SYMBOL_INFO) + 256];
            SYMBOL_INFO* sym = (SYMBOL_INFO*)symBuf;
            sym->SizeOfStruct = sizeof(SYMBOL_INFO);
            sym->MaxNameLen   = 255;

            IMAGEHLP_LINE64 srcLine = {};
            srcLine.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

            for (int i = 0; i < 64; i++) {
                if (!StackWalk64(machineType, proc, GetCurrentThread(),
                                 &frame, ctx, NULL,
                                 SymFunctionTableAccess64, SymGetModuleBase64, NULL))
                    break;
                if (frame.AddrPC.Offset == 0) break;

                DWORD64 addr   = frame.AddrPC.Offset;
                DWORD   disp   = 0;
                DWORD64 disp64 = 0;
                fprintf(log, "  [%02d] 0x%016llX", i, (unsigned long long)addr);
                if (SymFromAddr(proc, addr, &disp64, sym))
                    fprintf(log, "  %s+0x%X", sym->Name, (unsigned)disp64);
                if (SymGetLineFromAddr64(proc, addr, &disp, &srcLine))
                    fprintf(log, "  (%s:%d)", srcLine.FileName, srcLine.LineNumber);
                fprintf(log, "\n");
            }
            SymCleanup(proc);

            fprintf(log, "\nStack Memory (SP, 256 bytes):\n ");
    #if defined(_M_X64) || defined(_WIN64)
            unsigned char* sp = (unsigned char*)(uintptr_t)ctx->Rsp;
    #else
            unsigned char* sp = (unsigned char*)(uintptr_t)ctx->Esp;
    #endif
            for (int i = 0; i < 256; i++) {
                if (!IsBadReadPtr(sp + i, 1))
                    fprintf(log, " %02X", sp[i]);
                else
                    fprintf(log, " ??");
                if ((i + 1) % 16 == 0 && i < 255) fprintf(log, "\n ");
            }
            fprintf(log, "\n");

            WriteGameState(log);
            WriteConfig(log);
            WriteSystemInfo(log);
            momentum::platform::common::crash_handler::WriteLogRing(log);

            fprintf(log, "\n=== END CRASH REPORT ===\n");
            fclose(log);
        }

        char msg[1024];
        snprintf(msg, sizeof(msg),
                 "The game has crashed.\n\n"
                 "Exception: 0x%08X (%s)\n"
                 "Address: %p\n\n"
                 "Crash log: %s\n"
                 "Minidump: %s\n\n"
                 "Please share the crash log when reporting.",
                 ep->ExceptionRecord->ExceptionCode,
                 ExceptionCodeToString(ep->ExceptionRecord->ExceptionCode),
                 ep->ExceptionRecord->ExceptionAddress,
                 logPath, dmpPath);

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Courage-Reborn - Program crash", msg, NULL);
        return EXCEPTION_EXECUTE_HANDLER;
    }
}