#include "momentum/platform/impl/platform_posix.hpp"
#include "momentum/platform/impl/platform_desktop.hpp"

#include "momentum/platform/common/crash_handler.hpp"
#include "momentum/utility/log.hpp"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <filesystem>

#include <signal.h>
#include <execinfo.h>
#include <pthread.h>
#include <sys/utsname.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/sysinfo.h>
#include <ucontext.h>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif

#include <SDL3/SDL.h>
#include <SDL3/SDL_filesystem.h>

FILENAME_LOGGER();

alignas(16) static char sAltStack[65536];
static char sCrashDumpDir[512] = "";
static pthread_t sMainThread;

namespace momentum::platform::impl {

    void PlatformPosix::Shutdown() {
        _Exit(0);
    }

    std::string PlatformPosix::GetName() const {
        return "POSIX (SDL)";
    }

    void PlatformPosix::OnInitialized() {
        if (const char* base = SDL_GetBasePath()) {
            chdir(base);
        }
    }

    void PlatformPosix::InstallCrashHandler() {
        sMainThread = pthread_self();
        std::string dir = momentum::platform::common::crash_handler::GetCrashDumpDirectory();
        snprintf(sCrashDumpDir, sizeof(sCrashDumpDir), "%s", dir.c_str());

        momentum::platform::common::crash_handler::sLogLockInit = true;

        stack_t ss = {};
        ss.ss_sp    = sAltStack;
        ss.ss_size  = sizeof(sAltStack);
        ss.ss_flags = 0;
        sigaltstack(&ss, NULL);

        struct sigaction sa = {};
        sa.sa_sigaction = CrashHandlerPosix;
        sa.sa_flags     = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
        sigemptyset(&sa.sa_mask);

        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGABRT, &sa, NULL);
        sigaction(SIGFPE,  &sa, NULL);
        sigaction(SIGILL,  &sa, NULL);
        sigaction(SIGBUS,  &sa, NULL);
        LOG_INFO("Crash handler armed (dumps -> {})\n", sCrashDumpDir);
    }

    // === Private ===
    const char* PlatformPosix::SignalToString(int sig) {
        switch (sig) {
            case SIGSEGV: return "SIGSEGV (Segmentation Fault)";
            case SIGABRT: return "SIGABRT (Abort)";
            case SIGFPE:  return "SIGFPE (Floating Point Exception)";
            case SIGILL:  return "SIGILL (Illegal Instruction)";
            case SIGBUS:  return "SIGBUS (Bus Error)";
            default:      return "UNKNOWN";
        }
    }

    void PlatformPosix::WriteGameState(FILE* log) {
        WriteSdlGameState(log);
    }

    void PlatformPosix::WriteSystemInfo(FILE* log) {
        fprintf(log, "\nSystem:\n");

        struct utsname uts;
        if (uname(&uts) == 0) {
            fprintf(log, "  OS: %s %s %s\n", uts.sysname, uts.release, uts.machine);
        }

#ifdef __linux__
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            unsigned long long total = (unsigned long long)si.totalram * si.mem_unit / (1024*1024);
            unsigned long long free_ = (unsigned long long)si.freeram  * si.mem_unit / (1024*1024);
            fprintf(log, "  RAM: %llu MB total, %llu MB available\n", total, free_);
        }
#endif
#ifdef __APPLE__
        {
            int64_t memBytes = 0;
            size_t  len      = sizeof(memBytes);
            if (sysctlbyname("hw.memsize", &memBytes, &len, NULL, 0) == 0)
                fprintf(log, "  RAM: %lld MB total\n", memBytes / (1024*1024));
        }
#endif

#ifdef __APPLE__
        {
            int32_t ncpu = 0;
            size_t  ncpuLen = sizeof(ncpu);
            if (sysctlbyname("hw.logicalcpu", &ncpu, &ncpuLen, NULL, 0) == 0 && ncpu > 0)
                fprintf(log, "  Processors: %d\n", ncpu);
        }
#else
        {
            long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
            if (nprocs > 0)
                fprintf(log, "  Processors: %ld\n", nprocs);
        }
#endif
    }

    void PlatformPosix::WriteConfig(FILE* log) {
        const std::string path = momentum::platform::common::crash_handler::GetConfigPath();
        FILE* cfg = fopen(path.c_str(), "r");
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

    void PlatformPosix::WriteRegisters(FILE* log, void* ctx) {
        if (!ctx) return;
        ucontext_t* uc = (ucontext_t*)ctx;

#if defined(__linux__) && defined(__x86_64__)
        mcontext_t* mc = &uc->uc_mcontext;
        fprintf(log, "\nRegisters:\n");
        fprintf(log, "  RAX=%016llX  RBX=%016llX  RCX=%016llX  RDX=%016llX\n",
                (unsigned long long)mc->gregs[REG_RAX], (unsigned long long)mc->gregs[REG_RBX],
                (unsigned long long)mc->gregs[REG_RCX], (unsigned long long)mc->gregs[REG_RDX]);
        fprintf(log, "  RSI=%016llX  RDI=%016llX  RBP=%016llX  RSP=%016llX\n",
                (unsigned long long)mc->gregs[REG_RSI], (unsigned long long)mc->gregs[REG_RDI],
                (unsigned long long)mc->gregs[REG_RBP], (unsigned long long)mc->gregs[REG_RSP]);
        fprintf(log, "  R8 =%016llX  R9 =%016llX  R10=%016llX  R11=%016llX\n",
                (unsigned long long)mc->gregs[REG_R8],  (unsigned long long)mc->gregs[REG_R9],
                (unsigned long long)mc->gregs[REG_R10], (unsigned long long)mc->gregs[REG_R11]);
        fprintf(log, "  R12=%016llX  R13=%016llX  R14=%016llX  R15=%016llX\n",
                (unsigned long long)mc->gregs[REG_R12], (unsigned long long)mc->gregs[REG_R13],
                (unsigned long long)mc->gregs[REG_R14], (unsigned long long)mc->gregs[REG_R15]);
        fprintf(log, "  RIP=%016llX  EFLAGS=%08llX\n",
                (unsigned long long)mc->gregs[REG_RIP], (unsigned long long)mc->gregs[REG_EFL]);

#elif defined(__linux__) && defined(__i386__)
        mcontext_t* mc = &uc->uc_mcontext;
        fprintf(log, "\nRegisters:\n");
        fprintf(log, "  EAX=%08lX  EBX=%08lX  ECX=%08lX  EDX=%08lX\n",
                (unsigned long)mc->gregs[REG_EAX], (unsigned long)mc->gregs[REG_EBX],
                (unsigned long)mc->gregs[REG_ECX], (unsigned long)mc->gregs[REG_EDX]);
        fprintf(log, "  ESI=%08lX  EDI=%08lX  EBP=%08lX  ESP=%08lX\n",
                (unsigned long)mc->gregs[REG_ESI], (unsigned long)mc->gregs[REG_EDI],
                (unsigned long)mc->gregs[REG_EBP], (unsigned long)mc->gregs[REG_ESP]);
        fprintf(log, "  EIP=%08lX  EFLAGS=%08lX\n",
                (unsigned long)mc->gregs[REG_EIP], (unsigned long)mc->gregs[REG_EFL]);

#elif defined(__APPLE__) && defined(__x86_64__)
        fprintf(log, "\nRegisters:\n");
        fprintf(log, "  RAX=%016llX  RBX=%016llX  RCX=%016llX  RDX=%016llX\n",
                (unsigned long long)uc->uc_mcontext->__ss.__rax,
                (unsigned long long)uc->uc_mcontext->__ss.__rbx,
                (unsigned long long)uc->uc_mcontext->__ss.__rcx,
                (unsigned long long)uc->uc_mcontext->__ss.__rdx);
        fprintf(log, "  RSI=%016llX  RDI=%016llX  RBP=%016llX  RSP=%016llX\n",
                (unsigned long long)uc->uc_mcontext->__ss.__rsi,
                (unsigned long long)uc->uc_mcontext->__ss.__rdi,
                (unsigned long long)uc->uc_mcontext->__ss.__rbp,
                (unsigned long long)uc->uc_mcontext->__ss.__rsp);
        fprintf(log, "  R8 =%016llX  R9 =%016llX  R10=%016llX  R11=%016llX\n",
                (unsigned long long)uc->uc_mcontext->__ss.__r8,
                (unsigned long long)uc->uc_mcontext->__ss.__r9,
                (unsigned long long)uc->uc_mcontext->__ss.__r10,
                (unsigned long long)uc->uc_mcontext->__ss.__r11);
        fprintf(log, "  R12=%016llX  R13=%016llX  R14=%016llX  R15=%016llX\n",
                (unsigned long long)uc->uc_mcontext->__ss.__r12,
                (unsigned long long)uc->uc_mcontext->__ss.__r13,
                (unsigned long long)uc->uc_mcontext->__ss.__r14,
                (unsigned long long)uc->uc_mcontext->__ss.__r15);
        fprintf(log, "  RIP=%016llX  RFLAGS=%016llX\n",
                (unsigned long long)uc->uc_mcontext->__ss.__rip,
                (unsigned long long)uc->uc_mcontext->__ss.__rflags);

#elif defined(__APPLE__) && defined(__arm64__)
        fprintf(log, "\nRegisters:\n");
        for (int i = 0; i < 29; i++)
            fprintf(log, "  x%-2d=%016llX\n", i, (unsigned long long)uc->uc_mcontext->__ss.__x[i]);
        fprintf(log, "  fp =%016llX  lr =%016llX\n",
                (unsigned long long)uc->uc_mcontext->__ss.__fp,
                (unsigned long long)uc->uc_mcontext->__ss.__lr);
        fprintf(log, "  sp =%016llX  pc =%016llX  cpsr=%08X\n",
                (unsigned long long)uc->uc_mcontext->__ss.__sp,
                (unsigned long long)uc->uc_mcontext->__ss.__pc,
                (unsigned)uc->uc_mcontext->__ss.__cpsr);
#else
        (void)log; (void)uc;
#endif
    }

    void PlatformPosix::CrashHandlerPosix(int sig, siginfo_t* info, void* ctx) {
        char logPath[600];

        time_t now = time(NULL);
        struct tm t;
        localtime_r(&now, &t);

        snprintf(logPath, sizeof(logPath), "%s/crash_%04d%02d%02d_%02d%02d%02d.txt",
                sCrashDumpDir[0] ? sCrashDumpDir : ".",
                t.tm_year+1900, t.tm_mon+1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);

        FILE* log = fopen(logPath, "w");
        if (log) {
            fprintf(log, "=== CRASH REPORT ===\n");
            fprintf(log, "Version: %s\n", APP_VERSION);
            fprintf(log, "Build:   %s %s\n", __DATE__, __TIME__);

            char timeBuf[64];
            ctime_r(&now, timeBuf);
            fprintf(log, "Time:    %s", timeBuf);
            fprintf(log, "==========================================\n");

            fprintf(log, "\nSignal: %d (%s)\n", sig, SignalToString(sig));
            if (info)
                fprintf(log, "Fault Address: %p\n", info->si_addr);

            WriteRegisters(log, ctx);

            fprintf(log, "\nStack Trace:\n");
            void* frames[64];
            int   nframes = backtrace(frames, 64);
            char** syms   = backtrace_symbols(frames, nframes);
            if (syms) {
                for (int i = 0; i < nframes; i++)
                    fprintf(log, "  [%02d] %s\n", i, syms[i]);
                free(syms);
            }

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
                 "Signal: %d (%s)\n\n"
                 "Crash log: %s\n\n"
                 "Please share the crash log when reporting.",
                 sig, SignalToString(sig), logPath);

        if (pthread_equal(pthread_self(), sMainThread)) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Courage-Reborn - Program crash", msg, NULL);
        } else {
            fprintf(stderr, "%s\n", msg);
        }

        signal(sig, SIG_DFL);
        raise(sig);
    }
}
