#pragma once

#include <string>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    using PID_TYPE = DWORD;
#else
    #include <unistd.h>
    using PID_TYPE = pid_t;
#endif

PID_TYPE launch_command(const std::string& command);

bool stop_and_wait_process(PID_TYPE pid);

bool is_process_running(PID_TYPE pid);