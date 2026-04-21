//FilesFolders/Copy.cpp
#include "FilesFolders.h"
#include "../Utilidades/Utilities.h"
#include <iostream>
#include <cstring>
#include <vector>

extern ActiveSession currentSession;

// Helper para duplicar un inodo (archivo o carpeta)
static int recursiveCopy(std::fstream& disk, SuperBloque& sb, int srcInodeIdx, int destDirInodeIdx, std::string name) {
    Inode srcInode{};
    if (!ffReadInode(disk, sb, srcInodeIdx, srcInode)) return -1;

    // 1. Verificar permiso de LECTURA en origen
    if (!ffHasPermission(srcInode, currentSession.uid, currentSession.gid, 'r')) {
        std::cout << "Aviso: Saltando '" << name << "' por falta de permisos de lectura\n";
        return -1; 
    }

    // 2. Crear nuevo inodo en destino
    int newInodeIdx = ffAllocInode(disk, sb);
    if (newInodeIdx == -1) return -1;

    Inode newInode = srcInode; // Copiar metadatos básicos (tipo, permisos, etc)
    time_t now = time(nullptr);
    newInode.Atime = now;
    newInode.Ctime = now;
    newInode.Mtime = now;
    newInode.Uid   = currentSession.uid;
    newInode.Gid   = currentSession.gid;
    
    // Limpiar bloques para el nuevo inodo
    for (int i = 0; i < 15; i++) newInode.Block[i] = -1;

    if (srcInode.Type == '1') {
        // --- COPIA DE ARCHIVO ---
        for (int i = 0; i < 12; i++) {
            if (srcInode.Block[i] == -1) break;
            
            FileBlock srcFb{};
            if (!ffReadFileBlock(disk, sb, srcInode.Block[i], srcFb)) continue;

            int newBlockIdx = ffAllocBlock(disk, sb);
            if (newBlockIdx == -1) break;

            ffWriteFileBlock(disk, sb, newBlockIdx, srcFb);
            newInode.Block[i] = newBlockIdx;
        }
        ffWriteInode(disk, sb, newInodeIdx, newInode);
    } else {
        // --- COPIA DE CARPETA ---
        int newBlockIdx = ffAllocBlock(disk, sb);
        if (newBlockIdx == -1) { ffFreeInode(disk, sb, newInodeIdx); return -1; }

        newInode.Block[0] = newBlockIdx;
        newInode.Size = sizeof(FolderBlock); 
        ffWriteInode(disk, sb, newInodeIdx, newInode);

        // Inicializar carpeta destino con . y ..
        FolderBlock newFb{};
        for(int j=0; j<4; j++) { newFb.content[j].Inodo = -1; memset(newFb.content[j].Name, 0, 12); }
        strncpy(newFb.content[0].Name, ".",  12); newFb.content[0].Inodo = newInodeIdx;
        strncpy(newFb.content[1].Name, "..", 12); newFb.content[1].Inodo = destDirInodeIdx;
        ffWriteFolderBlock(disk, sb, newBlockIdx, newFb);

        // Recorrer hijos del origen y copiarlos
        for (int i = 0; i < 12; i++) {
            if (srcInode.Block[i] == -1) break;
            FolderBlock srcFb{};
            if (!ffReadFolderBlock(disk, sb, srcInode.Block[i], srcFb)) continue;

            for (int j = 0; j < 4; j++) {
                if (srcFb.content[j].Inodo != -1) {
                    std::string childName(srcFb.content[j].Name, strnlen(srcFb.content[j].Name, 12));
                    if (childName == "." || childName == "..") continue;

                    // Llamada recursiva para cada hijo
                    recursiveCopy(disk, sb, srcFb.content[j].Inodo, newInodeIdx, childName);
                }
            }
        }
    }

    // 3. Agregar entrada en el directorio padre destino
    ffAddEntry(disk, sb, destDirInodeIdx, name, newInodeIdx);
    return newInodeIdx;
}

std::string cmdCopy(std::map<std::string, std::string> params) {
    if (!currentSession.active) return "Error: No hay una sesión activa\n";
    if (params.find("path") == params.end()) return "Error: Parámetro -path obligatorio\n";
    if (params.find("destino") == params.end()) return "Error: Parámetro -destino obligatorio\n";

    std::string srcPath  = params["path"];
    std::string destPath = params["destino"];

    auto srcParts = ffSplitPath(srcPath);
    if (srcParts.empty()) return "Error: Ruta origen inválida\n";
    std::string srcName = srcParts.back();

    std::fstream disk(currentSession.diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco\n";

    SuperBloque sb;
    if (!ffReadSuperblock(disk, sb)) { disk.close(); return "Error: No se pudo leer el superbloque\n"; }

    // 1. Resolver origen
    int srcInodeIdx = ffResolvePath(disk, sb, srcPath, false, currentSession.uid, currentSession.gid);
    if (srcInodeIdx < 0) { disk.close(); return "Error: No existe la ruta de origen '" + srcPath + "'\n"; }

    // 2. Resolver carpeta destino
    int destDirInodeIdx = 0; // default root
    if (destPath != "/") {
        destDirInodeIdx = ffResolvePath(disk, sb, destPath, false, currentSession.uid, currentSession.gid);
    }
    if (destDirInodeIdx < 0) { disk.close(); return "Error: La carpeta de destino '" + destPath + "' no existe\n"; }

    Inode destInode{};
    ffReadInode(disk, sb, destDirInodeIdx, destInode);
    if (destInode.Type != '0') { disk.close(); return "Error: El destino debe ser una carpeta\n"; }

    // 3. Validar permiso de ESCRITURA en destino
    if (!ffHasPermission(destInode, currentSession.uid, currentSession.gid, 'w')) {
        disk.close();
        return "Error: No tiene permisos de escritura en la carpeta destino\n";
    }

    // 4. Verificar que el nombre original no exista ya en el destino
    if (ffLookup(disk, sb, destDirInodeIdx, srcName) != -1) {
        disk.close();
        return "Error: Ya existe un elemento llamado '" + srcName + "' en el destino\n";
    }

    // 5. Iniciar copia recursiva con el nombre original
    int res = recursiveCopy(disk, sb, srcInodeIdx, destDirInodeIdx, srcName);

    // 6. Journaling (EXT3)
    if (sb.FilesystemType == 3 && res != -1) {
        ffWriteJournal(disk, sb, "copy", srcPath, destPath);
    }

    disk.close();
    if (res == -1) return "Error: No se pudo realizar la copia de '" + srcPath + "'\n";
    return "Copia de '" + srcPath + "' a '" + destPath + "' completada exitosamente\n";
}
