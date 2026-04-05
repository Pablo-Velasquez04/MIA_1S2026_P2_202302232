//FileSystem/Logout.cpp
#include "FileSystem.h"

extern ActiveSession currentSession;

//LOGOUT
std::string cmdLogout() {

    if (!currentSession.active)
        return "Error: No hay ninguna sesión activa.\n";

    std::string closedUser = currentSession.user;
    std::string closedId   = currentSession.partitionId;

    //Limpiar sesión
    currentSession.active      = false;
    currentSession.user        = "";
    currentSession.partitionId = "";
    currentSession.diskPath    = "";
    currentSession.uid         = -1;
    currentSession.gid         = -1;
    currentSession.sbStart     = -1;

    return "Sesión cerrada exitosamente.\n"
           "  Usuario   : " + closedUser + "\n" +
           "  Partición : " + closedId   + "\n";
}