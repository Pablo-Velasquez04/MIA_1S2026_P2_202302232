// Users/Mkgrp.cpp
#include "Users.h"
#include <sstream>

extern ActiveSession currentSession;

std::string cmdMkgrp(std::map<std::string, std::string> params) {

    if (!currentSession.active)
        return "Error: No hay sesión activa. Ejecute login primero.\n";

    if (currentSession.user != "root")
        return "Error: Solo el usuario root puede ejecutar mkgrp.\n";

    std::string name = params.count("name") ? params["name"] : "";
    if (name.empty())
        return "Error: Falta parámetro -name.\n";

    if (name.size() > 10)
        return "Error: El nombre del grupo no puede exceder 10 caracteres.\n";

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

    if (groupExists(lines, name)) {
        disk.close();
        return "Error: El grupo '" + name + "' ya existe.\n";
    }

    int newGid = getNextId(lines, "G");
    lines.push_back(std::to_string(newGid) + ",G," + name);

    if (!writeUsersFile(disk, sb, buildContent(lines))) {
        disk.close();
        return "Error: No se pudo escribir en users.txt.\n";
    }

    disk.close();
    return "Grupo '" + name + "' creado exitosamente con GID=" +
           std::to_string(newGid) + ".\n";
}