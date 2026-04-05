//Users/Chgrp.cpp
#include "Users.h"
#include <sstream>

extern ActiveSession currentSession;

std::string cmdChgrp(std::map<std::string, std::string> params) {

    if (!currentSession.active)
        return "Error: No hay sesión activa.\n";

    if (currentSession.user != "root")
        return "Error: Solo root puede ejecutar chgrp.\n";

    std::string user = params.count("user") ? params["user"] : "";
    std::string grp  = params.count("grp")  ? params["grp"]  : "";

    if (user.empty()) return "Error: Falta parámetro -user.\n";
    if (grp.empty())  return "Error: Falta parámetro -grp.\n";

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

    if (!groupExists(lines, grp)) {
        disk.close();
        return "Error: El grupo '" + grp + "' no existe.\n";
    }

    if (!userExists(lines, user)) {
        disk.close();
        return "Error: El usuario '" + user + "' no existe.\n";
    }

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
        if (fields.size() >= 5 && fields[1] == "U" &&
            fields[3] == user   && fields[0] != "0") {
            line  = fields[0] + ",U," + grp + "," + user + "," + fields[4];
            found = true;
            break;
        }
    }

    if (!found) {
        disk.close();
        return "Error: No se pudo encontrar al usuario '" + user + "'.\n";
    }

    if (!writeUsersFile(disk, sb, buildContent(lines))) {
        disk.close();
        return "Error: No se pudo escribir en users.txt.\n";
    }

    disk.close();
    return "Usuario '" + user + "' cambiado al grupo '" + grp + "' exitosamente.\n";
}