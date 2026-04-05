//Users/Mkusr.cpp
#include "Users.h"
#include <sstream>

extern ActiveSession currentSession;

std::string cmdMkusr(std::map<std::string, std::string> params) {

    if (!currentSession.active)
        return "Error: No hay sesión activa.\n";

    if (currentSession.user != "root")
        return "Error: Solo root puede ejecutar mkusr.\n";

    std::string user = params.count("user") ? params["user"] : "";
    std::string pass = params.count("pass") ? params["pass"] : "";
    std::string grp  = params.count("grp")  ? params["grp"]  : "";

    if (user.empty()) return "Error: Falta parámetro -user.\n";
    if (pass.empty()) return "Error: Falta parámetro -pass.\n";
    if (grp.empty())  return "Error: Falta parámetro -grp.\n";

    if (user.size() > 10)
        return "Error: El nombre de usuario no puede exceder 10 caracteres.\n";
    if (pass.size() > 10)
        return "Error: La contraseña no puede exceder 10 caracteres.\n";

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

    if (userExists(lines, user)) {
        disk.close();
        return "Error: El usuario '" + user + "' ya existe.\n";
    }

    int newUid = getNextId(lines, "U");
    lines.push_back(std::to_string(newUid) + ",U," + grp + "," + user + "," + pass);

    if (!writeUsersFile(disk, sb, buildContent(lines))) {
        disk.close();
        return "Error: No se pudo escribir en users.txt.\n";
    }

    disk.close();
    return "Usuario '" + user + "' creado exitosamente con UID=" +
           std::to_string(newUid) + " en grupo '" + grp + "'.\n";
}