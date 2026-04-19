//FileSystem/Reset.cpp
#include "FileSystem.h"
#include "../Mounting/Mounting.h"
#include <iostream>

std::string cmdReset() {
    resetMounts();
    resetSession();
    return "Estado global reiniciado exitosamente. Todas las monturas y sesiones han sido limpiadas.\n";
}
