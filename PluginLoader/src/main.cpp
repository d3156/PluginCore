#include <PluginCore/Core.hpp>

#include <csignal>
#include <cerrno>
#include <cstring>
#include <unistd.h>  

static volatile sig_atomic_t g_stop = 0;

static void on_term(int /*signum*/) {
    g_stop = 1;
}

int main(int argc, char* argv[]) {
    struct sigaction sa = {};
    sa.sa_handler = on_term;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    d3156::PluginCore::Core core(argc, argv);
    while (!g_stop) pause(); 
    return 0;
}
