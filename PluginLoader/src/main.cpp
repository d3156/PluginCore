#include <PluginCore/Core.hpp>

#include <csignal>
#include <cerrno>
#include <cstring>
#include <unistd.h>  

static volatile sig_atomic_t g_stop = 0;

//////////////////////////////////Backtrace
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void sig_handler(int num)
{
    static int count = 0;
    count++;
    if (count < 2) g_stop = 1;
    else exit(0);    
}

void printBacktrace(int signal)
{
    (void)signal;
    void *backAddresses[30];
    int backtraceSize;
    char **backtraceFunctionNames;
    backtraceSize = backtrace(backAddresses, 30);
    backtraceFunctionNames = backtrace_symbols(backAddresses, backtraceSize);
    printf("\033[31m[Segfault backtrace]\033[0m\n");
    for (int i = 0; i < backtraceSize; i++) { printf("%s\n", backtraceFunctionNames[i]); }
    printf("\033[31m[End backtrace]\033[0m\n");
    printf("PluginLoader down.\n");
    exit(SIGSEGV);
}
//////////////////////////////////Backtrace


int main(int argc, char* argv[]) {
    struct sigaction sig;
    sig.sa_handler  = sig_handler;
    sig.sa_flags    = SA_NOCLDWAIT;
    sig.sa_restorer = NULL;
    sigaction(SIGINT, &sig, NULL);
    sigaction(SIGTERM, &sig, NULL);
    signal(SIGSEGV, printBacktrace);
    d3156::PluginCore::Core core(argc, argv);
    while (!g_stop) pause(); 
    return 0;
}
