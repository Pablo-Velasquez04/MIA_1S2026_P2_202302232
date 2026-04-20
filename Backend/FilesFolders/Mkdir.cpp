//FilesFolders/Mkdir.cpp
#include "FilesFolders.h"
#include <sstream>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

extern ActiveSession currentSession;

std::string cmdMkdir(std::map<std::string, std::string> params) {

    // 1. SESIÓN
    if (!currentSession.active)
        return "Error: No hay sesión activa. Ejecute login primero.\n";

    // 2. PARÁMETROS
    std::string path = params.count("path") ? params["path"] : "";
    if (path.empty())
        return "Error: Falta parámetro -path.\n";

    // -p: si existe en el mapa, está activo
    bool createParents = params.count("p") > 0;

    // 3. ABRIR DISCO
    std::fstream disk(currentSession.diskPath,
                      std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open())
        return "Error: No se pudo abrir el disco.\n";

    SuperBloque sb{};
    if (!ffReadSuperblock(disk, sb)) {
        disk.close();
        return "Error: No se pudo leer el superbloque.\n";
    }

    // 4. SEPARAR RUTA PADRE Y NOMBRE FINAL
    auto parts = ffSplitPath(path);
    if (parts.empty()) {
        disk.close();
        return "Error: Ruta inválida.\n";
    }

    std::string dirName = parts.back();

    // 5. RESOLVER CARPETA PADRE
    std::string parentPath = "/";
    for (size_t i = 0; i + 1 < parts.size(); i++)
        parentPath += parts[i] + (i + 2 < parts.size() ? "/" : "");

    int parentInodeIdx = 0; //raíz por defecto
    if (parts.size() > 1) {
        parentInodeIdx = ffResolvePath(disk, sb, parentPath,
                                       createParents,
                                       currentSession.uid,
                                       currentSession.gid);
        if (parentInodeIdx == -2) {
            disk.close();
            return "Error: Sin permiso de escritura en la carpeta padre.\n";
        }
        if (parentInodeIdx == -1) {
            disk.close();
            return "Error: Las carpetas padres no existen. "
                   "Use -p para crearlas automáticamente.\n";
        }
    }

    // 6. VERIFICAR PERMISO DE ESCRITURA EN PADRE
    Inode parentInode{};
    ffReadInode(disk, sb, parentInodeIdx, parentInode);
    if (!ffHasPermission(parentInode, currentSession.uid,
                         currentSession.gid, 'w')) {
        disk.close();
        return "Error: Sin permiso de escritura en '" + parentPath + "'.\n";
    }

    // 7. VERIFICAR SI YA EXISTE
    int existing = ffLookup(disk, sb, parentInodeIdx, dirName);
    if (existing != -1) {
        disk.close();
        return "Error: La carpeta '" + dirName + "' ya existe.\n";
    }

    // 8. ASIGNAR INODO Y BLOQUE
    int newInodeIdx = ffAllocInode(disk, sb);
    if (newInodeIdx == -1) {
        disk.close();
        return "Error: No hay inodos libres.\n";
    }

    int newBlockIdx = ffAllocBlock(disk, sb);
    if (newBlockIdx == -1) {
        disk.close();
        return "Error: No hay bloques libres.\n";
    }

    // 9. CREAR INODO CARPETA
    time_t now = time(nullptr);
    Inode newInode{};
    newInode.Uid     = currentSession.uid;
    newInode.Gid     = currentSession.gid;
    newInode.Size    = sizeof(FolderBlock);
    newInode.Atime   = now;
    newInode.Ctime   = now;
    newInode.Mtime   = now;
    newInode.Type    = '0'; //carpeta
    newInode.Perm[0] = '6';
    newInode.Perm[1] = '6';
    newInode.Perm[2] = '4';
    for (int k = 0; k < 15; k++) newInode.Block[k] = -1;
    newInode.Block[0] = newBlockIdx;
    ffWriteInode(disk, sb, newInodeIdx, newInode);

    // 10. CREAR BLOQUE CARPETA CON "." Y ".."
    FolderBlock newFb{};
    for (int j = 0; j < 4; j++) {
        newFb.content[j].Inodo = -1;
        memset(newFb.content[j].Name, 0, 12);
    }
    strncpy(newFb.content[0].Name, ".",  11);
    newFb.content[0].Inodo = newInodeIdx;
    strncpy(newFb.content[1].Name, "..", 11);
    newFb.content[1].Inodo = parentInodeIdx;
    ffWriteFolderBlock(disk, sb, newBlockIdx, newFb);

    // 11. AGREGAR ENTRADA EN CARPETA PADRE 
    if (!ffAddEntry(disk, sb, parentInodeIdx, dirName, newInodeIdx)) {
        disk.close();
        return "Error: No se pudo agregar entrada en la carpeta padre.\n";
    }

    ffWriteJournal(disk, sb, "mkdir", path, "");
    disk.close();

    // Sincronización con el sistema de archivos real
    try {
        std::string realPath = "." + path; // Hacerlo relativo a la raíz del proyecto
        fs::create_directories(fs::path(realPath).parent_path());
        fs::create_directory(realPath);
    } catch (...) {
        // Fallar silenciosamente o reportar como advertencia
    }

    return "Carpeta '" + path + "' creada exitosamente.\n";
}