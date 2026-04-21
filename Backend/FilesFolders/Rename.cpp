//FilesFolders/Rename.cpp
#include "FilesFolders.h"
#include "../Utilidades/Utilities.h"
#include <iostream>
#include <cstring>

extern ActiveSession currentSession;

std::string cmdRename(std::map<std::string, std::string> params) {
    if (!currentSession.active) return "Error: No hay una sesión activa\n";
    if (params.find("path") == params.end()) return "Error: Parámetro -path es obligatorio\n";
    if (params.find("name") == params.end()) return "Error: Parámetro -name es obligatorio\n";

    std::string path    = params["path"];
    std::string newName = params["name"];

    if (path == "/") return "Error: No se puede renombrar la carpeta raíz\n";

    std::fstream disk(currentSession.diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco de la sesión\n";

    SuperBloque sb;
    if (!ffReadSuperblock(disk, sb)) { disk.close(); return "Error: No se pudo leer el superbloque\n"; }

    // 1. Obtener el directorio padre a partir del path
    auto parts = ffSplitPath(path);
    if (parts.empty()) return "Error: Ruta inválida\n";
    std::string oldName = parts.back();
    std::string parentPath = "/";
    for(size_t i = 0; i + 1 < parts.size(); i++) {
        parentPath += parts[i] + (i + 2 < parts.size() ? "/" : "");
    }

    int parentInodeIdx = 0;
    if (parts.size() > 1) {
        parentInodeIdx = ffResolvePath(disk, sb, parentPath, false, currentSession.uid, currentSession.gid);
    }
    if (parentInodeIdx < 0) { disk.close(); return "Error: No se encontró el directorio padre\n"; }

    // 2. Buscar el inode del archivo dentro del directorio padre
    int targetInodeIdx = ffLookup(disk, sb, parentInodeIdx, oldName);
    if (targetInodeIdx == -1) { disk.close(); return "Error: El archivo '" + oldName + "' no existe\n"; }

    // 3. Verificar permisos de escritura sobre el archivo
    Inode targetInode{};
    ffReadInode(disk, sb, targetInodeIdx, targetInode);
    if (!ffHasPermission(targetInode, currentSession.uid, currentSession.gid, 'w')) {
        disk.close();
        return "Error: No tiene permisos de escritura sobre '" + path + "'\n";
    }

    // 4. Verificar que el nuevo nombre no exista ya
    if (ffLookup(disk, sb, parentInodeIdx, newName) != -1) {
        disk.close();
        return "Error: Ya existe un elemento con el nombre '" + newName + "' en el mismo directorio\n";
    }

    // 5. Buscar y actualizar la entrada en el padre
    Inode parentInode{};
    ffReadInode(disk, sb, parentInodeIdx, parentInode);
    bool found = false;

    for (int i = 0; i < 12; i++) {
        if (parentInode.Block[i] == -1) break;
        FolderBlock fb{};
        if (!ffReadFolderBlock(disk, sb, parentInode.Block[i], fb)) continue;

        for (int j = 0; j < 4; j++) {
            if (fb.content[j].Inodo == targetInodeIdx) {
                // Verificar que el nombre coincide con el que queremos cambiar (por si acaso hay hardlinks o algo raro)
                std::string entryName(fb.content[j].Name, strnlen(fb.content[j].Name, 12));
                std::string searchName = oldName.length() > 12 ? oldName.substr(0, 12) : oldName;
                if (entryName == searchName) {
                    memset(fb.content[j].Name, 0, 12);
                    strncpy(fb.content[j].Name, newName.c_str(), 12);
                    ffWriteFolderBlock(disk, sb, parentInode.Block[i], fb);
                    found = true;
                    break;
                }
            }
        }
        if (found) break;
    }

    if (!found) {
        disk.close();
        return "Error crítico: No se encontró la entrada del archivo en el directorio padre\n";
    }

    // 6. Actualizar Mtime del padre e inodo objetivo
    parentInode.Mtime = time(nullptr);
    ffWriteInode(disk, sb, parentInodeIdx, parentInode);

    targetInode.Mtime = time(nullptr); // El inodo en sí no "cambia" pero su metadato de nombre en el padre sí... 
                                       // En muchos sistemas rename actualiza el ctime.
    ffWriteInode(disk, sb, targetInodeIdx, targetInode);

    // 7. EXT3 Journaling
    if (sb.FilesystemType == 3) {
        ffWriteJournal(disk, sb, "rename", path, newName);
    }

    disk.close();
    return "Elemento '" + path + "' renombrado a '" + newName + "' exitosamente\n";
}
