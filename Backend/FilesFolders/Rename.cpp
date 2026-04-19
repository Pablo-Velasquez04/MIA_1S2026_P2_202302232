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

    if (newName.length() > 11) return "Error: El nuevo nombre es demasiado largo (máximo 11 caracteres)\n";
    if (path == "/") return "Error: No se puede renombrar la carpeta raíz\n";

    std::fstream disk(currentSession.diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco de la sesión\n";

    SuperBloque sb;
    if (!ffReadSuperblock(disk, sb)) { disk.close(); return "Error: No se pudo leer el superbloque\n"; }

    // 1. Resolver ruta del objetivo
    int targetInodeIdx = ffResolvePath(disk, sb, path, false, currentSession.uid, currentSession.gid);
    if (targetInodeIdx < 0) {
        disk.close();
        if (targetInodeIdx == -2) return "Error: Permisos insuficientes para navegar la ruta\n";
        return "Error: No se encontró el archivo o carpeta en '" + path + "'\n";
    }

    // 2. Validar permisos sobre el objetivo
    Inode targetInode{};
    ffReadInode(disk, sb, targetInodeIdx, targetInode);
    if (!ffHasPermission(targetInode, currentSession.uid, currentSession.gid, 'w')) {
        disk.close();
        return "Error: No tiene permisos de escritura sobre '" + path + "'\n";
    }

    // 3. Obtener inodo del padre
    auto parts = ffSplitPath(path);
    std::string oldName = parts.back();
    parts.pop_back();
    std::string parentPath = "/";
    for(const auto& p : parts) parentPath += p + "/";

    int parentInodeIdx = ffResolvePath(disk, sb, parentPath, false, currentSession.uid, currentSession.gid);
    if (parentInodeIdx < 0) { disk.close(); return "Error: No se pudo encontrar el directorio padre\n"; }

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
                if (entryName == oldName) {
                    memset(fb.content[j].Name, 0, 12);
                    strncpy(fb.content[j].Name, newName.c_str(), 11);
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
        // Registrar operación rename
    }

    disk.close();
    return "Elemento '" + path + "' renombrado a '" + newName + "' exitosamente\n";
}
