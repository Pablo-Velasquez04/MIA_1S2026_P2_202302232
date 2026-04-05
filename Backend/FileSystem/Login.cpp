//FileSystem/Login.cpp
#include "FileSystem.h"
#include "../Estructuras/Structs.h"
#include "../Mounting/Mounting.h"
#include "../Utilidades/Utilities.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>

//SESIÓN ACTIVA EN RAM
ActiveSession currentSession = {false, "", "", "", -1, -1, -1};

//Acceso global — lo usan MKDIR, MKFILE, CAT, etc.
const ActiveSession* getActiveSession() {
    return currentSession.active ? &currentSession : nullptr;
}

bool hasActiveSession() {
    return currentSession.active;
}


// Using Utilities.h for leerStruct

//Leer el contenido completo de users.txt desde el disco
//Sigue la cadena de bloques del inodo 1
static std::string readUsersFile(std::fstream& disk,
                                  const SuperBloque& sb) {
    // Leer inodo 1 (users.txt)
    long inodePos = sb.InodeStart + 1 * sb.InodeSize;
    Inode usersInode{};
    if (!leerStruct(disk, inodePos, usersInode))
        return "";

    std::string content;
    content.reserve(usersInode.Size);

    //Recorrer bloques directos (0-11)
    for (int i = 0; i < 12 && i < 15; i++) {
        if (usersInode.Block[i] == -1) break;

        long blockPos = sb.BlockStart +
                        (long)usersInode.Block[i] * sb.BlockSize;
        FileBlock fb{};
        if (!leerStruct(disk, blockPos, fb)) break;

        //Agregar solo hasta el tamaño real del archivo
        int remaining = usersInode.Size - (int)content.size();
        int toRead    = std::min(remaining, sb.BlockSize);
        if (toRead <= 0) break;

        content.append(fb.content, toRead);
    }

    return content;
}

//Parsear users.txt y buscar usuario/contraseña
// Formato de cada línea:
//   GID,G,nombre_grupo
//   UID,U,grupo,usuario,contraseña
// Retorna uid y gid si encontró el usuario, -1 si no
struct UserInfo {
    int         uid;
    int         gid;
    std::string group;
    bool        found;
};

static UserInfo findUser(const std::string& usersContent,
                          const std::string& user,
                          const std::string& pass) {
    UserInfo result = {-1, -1, "", false};

    std::istringstream iss(usersContent);
    std::string line;

    //Primera pasada: buscar el usuario y su grupo
    std::string userGroup;
    int         userUid = -1;
    bool        userFound = false;
    bool        passMatch = false;

    while (std::getline(iss, line)) {
        if (line.empty()) continue;

        //Eliminar \r si existe (Windows line endings)
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        //Parsear campos separados por coma
        std::vector<std::string> fields;
        std::istringstream lineStream(line);
        std::string field;
        while (std::getline(lineStream, field, ','))
            fields.push_back(field);

        if (fields.size() < 3) continue;

        //Verificar que no esté eliminado (UID/GID = 0)
        if (fields[0] == "0") continue;

        //Línea de usuario: UID,U,grupo,usuario,contraseña
        if (fields[1] == "U" && fields.size() == 5) {
            std::string lineUser = fields[3];
            std::string linePass = fields[4];

            if (lineUser == user && linePass == pass) {
                userUid   = std::stoi(fields[0]);
                userGroup = fields[2];
                userFound = true;
                passMatch = true;
            } else if (lineUser == user) {
                userFound = true;
                passMatch = false;
            }
        }
    }

    if (!userFound) {
        result.found = false;
        return result;
    }

    if (!passMatch) {
        result.found = false;
        result.uid   = -2; //señal de contraseña incorrecta
        return result;
    }

    //Segunda pasada: buscar el GID del grupo
    std::istringstream iss2(usersContent);
    while (std::getline(iss2, line)) {
        if (line.empty()) continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::vector<std::string> fields;
        std::istringstream lineStream(line);
        std::string field;
        while (std::getline(lineStream, field, ','))
            fields.push_back(field);

        if (fields.size() < 3) continue;
        if (fields[0] == "0") continue;

        //Línea de grupo: GID,G,nombre_grupo
        if (fields[1] == "G" && fields[2] == userGroup) {
            result.uid   = userUid;
            result.gid   = std::stoi(fields[0]);
            result.group = userGroup;
            result.found = true;
            return result;
        }
    }

    //Si no encontró el grupo (no debería pasar) usar gid=1
    result.uid   = userUid;
    result.gid   = 1;
    result.group = userGroup;
    result.found = true;
    return result;
}

//LOGIN
std::string cmdLogin(std::map<std::string, std::string> params) {

    // 1. PARÁMETROS OBLIGATORIOS 
    std::string user = params.count("user") ? params["user"] : "";
    std::string pass = params.count("pass") ? params["pass"] : "";
    std::string id   = params.count("id")   ? params["id"]   : "";

    if (user.empty()) return "Error: Falta parámetro -user\n";
    if (pass.empty()) return "Error: Falta parámetro -pass\n";
    if (id.empty())   return "Error: Falta parámetro -id\n";

    // 2. VERIFICAR SESIÓN ACTIVA 
    if (currentSession.active)
        return "Error: Ya hay una sesión activa con el usuario '" +
               currentSession.user +
               "'. Debe ejecutar logout primero.\n";

    // 3. BUSCAR PARTICIÓN MONTADA 
    const MountedPartition* mp = getMountedById(id);
    if (!mp)
        return "Error: No existe partición montada con id '" + id + "'\n";

    if (!archivoExiste(mp->Path))
        return "Error: El disco '" + mp->Path + "' no existe\n";

    // 4. ABRIR DISCO Y LEER SUPERBLOQUE 
    std::fstream disk(mp->Path, std::ios::binary | std::ios::in);
    if (!disk.is_open())
        return "Error: No se pudo abrir el disco '" + mp->Path + "'\n";

    long sbPos = mp->Start;
    SuperBloque sb{};
    if (!leerStruct(disk, sbPos, sb)) {
        disk.close();
        return "Error: No se pudo leer el superbloque. "
               "¿Se ejecutó mkfs en esta partición?\n";
    }

    //Verificar que es EXT2 válido
    if (sb.Magic != 0xEF53) {
        disk.close();
        return "Error: La partición '" + id +
               "' no tiene un sistema de archivos EXT2 válido. "
               "Ejecute mkfs primero.\n";
    }

    // 5. LEER users.txt
    std::string usersContent = readUsersFile(disk, sb);
    disk.close();

    if (usersContent.empty())
        return "Error: No se pudo leer users.txt\n";

    // 6. BUSCAR USUARIO Y VALIDAR CONTRASEÑA
    UserInfo info = findUser(usersContent, user, pass);

    if (!info.found) {
        if (info.uid == -2)
            return "Error: Autenticación fallida. "
                   "Contraseña incorrecta para el usuario '" + user + "'.\n";
        else
            return "Error: El usuario '" + user + "' no existe.\n";
    }

    // 7. INICIAR SESIÓN EN RAM
    currentSession.active      = true;
    currentSession.user        = user;
    currentSession.partitionId = id;
    currentSession.diskPath    = mp->Path;
    currentSession.uid         = info.uid;
    currentSession.gid         = info.gid;
    currentSession.sbStart     = sbPos;

    std::ostringstream out;
    out << "============== LOGIN EXITOSO - BIENVENIDO :D ==============\n";
    out << "  Usuario   : " << user       << "\n";
    out << "  Grupo     : " << info.group << "\n";
    out << "  UID       : " << info.uid   << "\n";
    out << "  GID       : " << info.gid   << "\n";
    out << "  Partición : " << id         << "\n";
    out << "  Disco     : " << mp->Path   << "\n";

    return out.str();
}