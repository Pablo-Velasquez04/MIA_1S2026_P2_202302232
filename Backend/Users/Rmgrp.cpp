//Users/Rmgrp.cpp
#include "Users.h"
#include <sstream>

extern ActiveSession currentSession;

std::string cmdRmgrp(std::map<std::string, std::string> params) {

    if (!currentSession.active)
        return "Error: No hay sesión activa.\n";

    if (currentSession.user != "root")
        return "Error: Solo root puede ejecutar rmgrp.\n";

    std::string name = params.count("name") ? params["name"] : "";
    if (name.empty())
        return "Error: Falta parámetro -name.\n";

    std::fstream disk = openSessionDisk();
    if (!disk.is_open())
        return "Error: No se pudo abrir el disco.\n";

    SuperBloque sb{};
    if (!readSuperblock(disk, sb)) {
        disk.close();
        return "Error: No se pudo leer el superbloque.\n";
    }

    std::string content = readUsersFile(disk, sb);
    auto lines = parseLines(content);

    bool found = false;
    for (auto& line : lines) {
        if (line.empty()) continue;
        std::vector<std::string> fields;
        std::istringstream ls(line);
        std::string f;
        while (std::getline(ls, f, ',')) {
            f.erase(0, f.find_first_not_of(" "));
            f.erase(f.find_last_not_of(" ") + 1);
            fields.push_back(f);
        }
        if (fields.size() >= 3 && fields[1] == "G" &&
            fields[2] == name   && fields[0] != "0") {
            line  = "0,G," + name;
            found = true;
            break;
        }
    }

    if (!found) {
        disk.close();
        return "Error: El grupo '" + name + "' no existe.\n";
    }

    if (!writeUsersFile(disk, sb, buildContent(lines))) {
        disk.close();
        return "Error: No se pudo escribir en users.txt.\n";
    }

    disk.close();
    return "Grupo '" + name + "' eliminado exitosamente.\n";
}