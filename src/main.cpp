#include <iostream>
#include <string>
#include <set>
#include <filesystem>
#include <fstream>
#include <thread>

#include <csignal>

#include "process.hpp"
#include "scanner.hpp"

namespace fs = std::filesystem;
namespace ch = std::chrono;

#define CACHE_DIR ".auto-restart/"
#define PID_FILE CACHE_DIR"pid"

#define DELAY 1000 // in milliseconds

typedef std::filesystem::file_time_type file_time;
    
// return the pid in the file, or -1 if the file does not exist or cannot be read
PID_TYPE read_pid() {
    std::ifstream pidFile(PID_FILE);
    if (!pidFile.is_open()) {
        return (PID_TYPE)-1;
    }

    PID_TYPE pid;
    pidFile >> pid;
    if (pidFile.fail()) {
        return (PID_TYPE)-1;
    }

    return pid;
}

void write_pid(PID_TYPE pid) {
    fs::create_directories(CACHE_DIR); // Ensure the cache directory exists
    std::ofstream pidFile(PID_FILE);
    if (!pidFile.is_open()) {
        std::cerr << "Impossible d'ouvrir le fichier PID pour l'écriture." << std::endl;
        return;
    }

    pidFile << pid;
    if (pidFile.fail()) {
        std::cerr << "Erreur lors de l'écriture du PID dans le fichier." << std::endl;
    }
}

file_time get_pid_file_time() {
    try {
        return fs::last_write_time(PID_FILE);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Erreur lors de la récupération de la date de modification du fichier PID: " << e.what() << std::endl;
        return file_time::min();
    }
}

// return most recent edit date of files
file_time get_edit_date(const std::set<std::string>& sources) {
    file_time mostRecentTime = file_time::min();

    for (const auto& source : sources) {
        try {
            file_time ftime = fs::last_write_time(source);
            if (mostRecentTime < ftime) {
                mostRecentTime = ftime;
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Erreur lors de la récupération de la date de modification pour le fichier: " << source << ". " << e.what() << std::endl;
        }
    }
    return mostRecentTime;
}



bool loop(const std::string& command, const std::set<std::string>& source_patterns) {
    std::set<std::string> sources;
    for (const std::string& pattern : source_patterns) {
        auto scanned_sources = scanSources(pattern);
        sources.insert(scanned_sources.begin(), scanned_sources.end());
    }

    if(sources.empty()) {
        std::cerr << "No source files found for the given patterns." << std::endl;
        return false;
    }

    // check if at least one of the sources files is modified (last edit time is after last command start time)
    file_time pid_edit_time = get_pid_file_time();
    file_time mostRecentEditTime = get_edit_date(sources);
    if (mostRecentEditTime > pid_edit_time) {
        std::cout << "Source files have been modified. Restarting the command..." << std::endl;
        // Stop the previous process if it is running
        PID_TYPE pid = read_pid();
        if (pid != (PID_TYPE)-1 && is_process_running(pid)) {
            stop_and_wait_process(pid);
        }
        std::this_thread::sleep_for(ch::milliseconds(DELAY)); // Wait for a short delay before restarting

        // Launch the command and write its PID to the file
        PID_TYPE new_pid = launch_command(command);
        if (new_pid != (PID_TYPE)-1) {
            write_pid(new_pid);
        } else {
            std::cerr << "Failed to launch the command." << std::endl;
            return false;
        }
    }
    return true;
}



bool run = true;

void signal_handler(int signal) {
    run = false;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <command> <source_patterns>" << std::endl;
        return 1;
    }
    fs::create_directories(CACHE_DIR);
    std::ofstream gitignore(CACHE_DIR".gitignore");
    if (gitignore.is_open()) {
        gitignore << "*\n";
        gitignore.close();
    } else {
        std::cerr << "Impossible de créer le fichier .gitignore dans le répertoire de cache." << std::endl;
    }

    std::string command = argv[1];
    std::set<std::string> source_patterns;
    for (int i = 2; i < argc; ++i) {
        source_patterns.insert(argv[i]);
    }

    // Register signal handlers for graceful termination
    std::signal(SIGINT, signal_handler);
    std::signal(SIGQUIT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    PID_TYPE new_pid = launch_command(command);
    if (new_pid != (PID_TYPE)-1) {
        write_pid(new_pid);
    } else {
        std::cerr << "Failed to launch the command." << std::endl;
        return false;
    }

    do {
        run &= loop(command, source_patterns);
        // run = false; // debug
        if (run) {
            std::this_thread::sleep_for(ch::milliseconds(1000));
        }
    } while (run);

    std::cout << "Stopping the command..." << std::endl;
    stop_and_wait_process(read_pid());
    return 0;
}