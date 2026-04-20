// FileSystem/FileSystem.h
#pragma once
#include <string>
#include <map>

// Estructura de sesión activa — accesible desde otros módulos
struct ActiveSession {
    bool        active;
    std::string user;
    std::string partitionId;
    std::string diskPath;
    int         uid;
    int         gid;
    long        sbStart;
};

// Comandos
std::string cmdMkfs  (std::map<std::string, std::string> params);
std::string cmdLogin (std::map<std::string, std::string> params);
std::string cmdLogout();
std::string cmdLoss  (std::map<std::string, std::string> params);
std::string cmdJournaling(std::map<std::string, std::string> params);
void resetSession();

// Acceso global a la sesión — lo usan MKDIR, MKFILE, CAT, MKGRP, etc.
bool               hasActiveSession();
const ActiveSession* getActiveSession();