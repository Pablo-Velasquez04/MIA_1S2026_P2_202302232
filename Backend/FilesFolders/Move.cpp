//FilesFolders/Move.cpp
#include "FilesFolders.h"
#include "../Utilidades/Utilities.h"
#include <iostream>
#include <cstring>
#include <vector>

extern ActiveSession currentSession;

std::string cmdMove(std::map<std::string, std::string> params) {
    if (!currentSession.active) return "Error: No hay una sesión activa\n";
    if (params.find("path") == params.end()) return "Error: Parámetro -path obligatorio\n";
    if (params.find("destino") == params.end()) return "Error: Parámetro -destino obligatorio\n";

    std::string srcPath  = params["path"];
    std::string destPath = params["destino"];

    if (srcPath == "/") return "Error: No se puede mover la raíz\n";

    auto srcParts = ffSplitPath(srcPath);
    if (srcParts.empty()) return "Error: Ruta origen inválida\n";
    std::string srcName = srcParts.back();

    // Validar cíclicos basándose en directorio destino y origen
    if (destPath == srcPath || (destPath.find(srcPath + "/") == 0)) {
        return "Error: No se puede mover una carpeta dentro de sí misma o uno de sus subdirectorios\n";
    }

    std::fstream disk(currentSession.diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco\n";

    SuperBloque sb;
    if (!ffReadSuperblock(disk, sb)) { disk.close(); return "Error: No se pudo leer el superbloque\n"; }

    // 1. Resolver Carpeta Destino directamente desde destPath
    int destDirInodeIdx = 0;
    if (destPath != "/") {
        destDirInodeIdx = ffResolvePath(disk, sb, destPath, false, currentSession.uid, currentSession.gid);
    }
    if (destDirInodeIdx < 0) { disk.close(); return "Error: La carpeta de destino '" + destPath + "' no existe\n"; }

    Inode destInode{};
    ffReadInode(disk, sb, destDirInodeIdx, destInode);
    if (destInode.Type != '0') { disk.close(); return "Error: El destino debe ser una carpeta\n"; }
    
    if (!ffHasPermission(destInode, currentSession.uid, currentSession.gid, 'w')) {
        disk.close();
        return "Error: No tiene permisos de escritura en la carpeta destino\n";
    }

    // 2. Resolver Inodo Origen
    int srcInodeIdx = ffResolvePath(disk, sb, srcPath, false, currentSession.uid, currentSession.gid);
    if (srcInodeIdx < 0) { disk.close(); return "Error: No existe la ruta de origen '" + srcPath + "'\n"; }

    Inode srcInode{};
    ffReadInode(disk, sb, srcInodeIdx, srcInode);

    if (!ffHasPermission(srcInode, currentSession.uid, currentSession.gid, 'w')) {
        disk.close();
        return "Error: No tiene permiso de escritura sobre el elemento a mover\n";
    }

    // 3. Obtener el padre del origen
    std::string srcParentPath = "/";
    for (size_t i = 0; i + 1 < srcParts.size(); i++) {
        srcParentPath += srcParts[i] + (i + 2 < srcParts.size() ? "/" : "");
    }

    int srcParentInodeIdx = 0;
    if (srcParts.size() > 1) {
        srcParentInodeIdx = ffResolvePath(disk, sb, srcParentPath, false, currentSession.uid, currentSession.gid);
    }
    if (srcParentInodeIdx < 0) { disk.close(); return "Error: No se pudo encontrar el padre del origen\n"; }

    // Verificar si el nombre original ya existe en destino
    if (ffLookup(disk, sb, destDirInodeIdx, srcName) != -1) {
        disk.close();
        return "Error: Ya existe un elemento llamado '" + srcName + "' en el destino\n";
    }

    // 4. Remover referencia antigua (con nombre original)
    if (!ffRemoveEntry(disk, sb, srcParentInodeIdx, srcName)) {
        disk.close();
        return "Error: No se pudo remover la referencia antigua\n";
    }

    // 5. Agregar nueva referencia (con nombre original)
    if (!ffAddEntry(disk, sb, destDirInodeIdx, srcName, srcInodeIdx)) {
        disk.close();
        return "Error: No se pudo agregar la referencia en el destino\n";
    }

    // 6. Si el origen es CARPETA, actualizar el registro ".."
    if (srcInode.Type == '0') {
        if (srcInode.Block[0] != -1) {
            FolderBlock fb{};
            if (ffReadFolderBlock(disk, sb, srcInode.Block[0], fb)) {
                for (int j = 0; j < 4; ++j) {
                    if (strncmp(fb.content[j].Name, "..", 2) == 0) {
                        fb.content[j].Inodo = destDirInodeIdx;
                        ffWriteFolderBlock(disk, sb, srcInode.Block[0], fb);
                        break;
                    }
                }
            }
        }
    }

    // 7. Journaling (EXT3)
    if (sb.FilesystemType == 3) {
        ffWriteJournal(disk, sb, "move", srcPath, destPath);
    }

    disk.close();
    return "Elemento '" + srcPath + "' movido exitosamente a '" + destPath + "'\n";
}
