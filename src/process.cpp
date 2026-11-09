#include "process.hpp"

#include <iostream>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    using PID_TYPE = DWORD;
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <signal.h>
    using PID_TYPE = pid_t;
#endif

// Fonction portable pour exécuter une commande et obtenir son PID
PID_TYPE launch_command(const std::string& command) {
#if defined(_WIN32) || defined(_WIN64)
    // --- IMPLÉMENTATION WINDOWS ---
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Créer une copie éditable car CreateProcessA peut modifier la chaîne
    std::string cmd_copy = command;

    if (!CreateProcessA(
        NULL,           // Module
        &cmd_copy[0],   // Ligne de commande (modifiable)
        NULL, NULL,     // Attributs de sécurité
        FALSE,          // Héritage des descripteurs
        0,              // Drapeaux de création
        NULL, NULL,     // Environnement et Répertoire
        &si, &pi        // Structures de démarrage et d'information
    )) {
        return (PID_TYPE)-1;
    }

    // Fermer les handles pour éviter les fuites de ressources
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return pi.dwProcessId;

#else
    // --- IMPLÉMENTATION LINUX / MACOS ---
    pid_t pid = fork();

    if (pid < 0) {
        // Échec du fork
        return -1;
    } 
    else if (pid == 0) {
        // --- PROCESSUS ENFANT ---
        // Placer le shell et tous ses descendants dans leur propre groupe.
        if (setpgid(0, 0) != 0) {
            std::cerr << "Impossible de créer le groupe de processus." << std::endl;
            _exit(127);
        }

        // Découper la commande pour execlp / execvp ("/bin/sh", "-c", command)
        // Passer par le shell "sh" permet de gérer les arguments complexes
        execl("/bin/sh", "sh", "-c", command.c_str(), (char*)NULL);
        
        // Si execl échoue
        _exit(127);
    } 
    else {
        // --- PROCESSUS PARENT ---
        // Le parent fait également l'appel afin de réduire la fenêtre de course
        // avec l'exécution du shell dans le processus enfant.
        setpgid(pid, pid);
        return pid;
    }
#endif
}


bool stop_and_wait_process(PID_TYPE pid) {
#if defined(_WIN32) || defined(_WIN64)
    // --- IMPLÉMENTATION WINDOWS ---
    // Ouvrir un handle sur le processus avec les droits requis (SYNCHRONIZE et TERMINATE)
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
    if (hProcess == NULL) {
        std::cerr << "Impossible d'ouvrir le processus avec le PID: " << pid << std::endl;
        return false;
    }

    // Envoyer l'équivalent d'un signal d'arrêt forcé (0 = code de sortie du process)
    if (!TerminateProcess(hProcess, 0)) {
        std::cerr << "Erreur lors de l'envoi du signal d'arret." << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    // Attendre indéfiniment que le processus se termine
    WaitForSingleObject(hProcess, INFINITE);
    
    CloseHandle(hProcess);
    return true;

#else
    // --- IMPLÉMENTATION LINUX / MACOS ---
    // Envoyer le signal à tout le groupe, et pas seulement au shell.
    if (kill(-pid, SIGTERM) != 0) {
        std::cerr << "Erreur lors de l'envoi du signal SIGTERM au groupe du PID: "
                  << pid << std::endl;
        return false;
    }

    std::cout << "Waiting for the process with PID " << pid << " to terminate..." << std::endl;

    // Attendre que le processus enfant change d'état (se termine)
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        std::cerr << "Erreur lors de l'attente du processus." << std::endl;
        return false;
    }

    return true;
#endif
}

bool is_process_running(PID_TYPE pid) {
#if defined(_WIN32) || defined(_WIN64)
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (hProcess == NULL) {
        return false; // Le processus n'existe pas ou ne peut pas être ouvert
    }
    DWORD waitResult = WaitForSingleObject(hProcess, 0);
    CloseHandle(hProcess);
    return waitResult == WAIT_TIMEOUT; // Si le processus est toujours en cours d'exécution
#else
    // Vérifier si le processus existe en envoyant le signal 0
    return (kill(pid, 0) == 0);
#endif
}
