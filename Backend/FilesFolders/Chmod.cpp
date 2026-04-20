//FilesFolders/Chmod.cpp
#include "FilesFolders.h"
#include <sstream>
#include <cstring>
#include <ctime>
#include <iostream>

extern ActiveSession currentSession;

static void recursiveChmod(std::fstream& disk, const SuperBloque& sb, int inodeIdx, const std::string& ugo) {
    if (inodeIdx < 0 || inodeIdx >= sb.InodesCount) return;

    Inode curInode;
    if (!ffReadInode(disk, sb, inodeIdx, curInode)) return;

    // Solo el propietario o root pueden cambiar permisos
    if (currentSession.user != "root" && currentSession.uid != curInode.Uid) {
        return; 
    }

    curInode.Perm[0] = ugo[0];
    curInode.Perm[1] = ugo[1];
    curInode.Perm[2] = ugo[2];
    curInode.Mtime = time(nullptr);
    ffWriteInode(disk, sb, inodeIdx, curInode);

    if (curInode.Type == '1') return; // Es un archivo

    auto processEntry = [&](int blockIdx) {
        FolderBlock fb;
        if (!ffReadFolderBlock(disk, sb, blockIdx, fb)) return;
        for (int e = 0; e < 4; e++) {
            if (fb.content[e].Inodo != -1) {
                std::string entryName(fb.content[e].Name, strnlen(fb.content[e].Name, 12));
                if (entryName == "." || entryName == "..") continue;
                recursiveChmod(disk, sb, fb.content[e].Inodo, ugo);
            }
        }
    };

    auto processPointerBlock = [&](int bIdx, int level, auto& self) -> void {
        PointerBlock pb;
        if (!ffReadPointerBlock(disk, sb, bIdx, pb)) return;
        for (int p = 0; p < 16; p++) {
            if (pb.pointers[p] != -1) {
                if (level == 1) processEntry(pb.pointers[p]);
                else self(pb.pointers[p], level - 1, self);
            }
        }
    };

    for (int i = 0; i < 15; i++) {
        int blockIdx = curInode.Block[i];
        if (blockIdx == -1) continue;

        if (i < 12) {
            processEntry(blockIdx);
        } else if (i == 12) {
            processPointerBlock(blockIdx, 1, processPointerBlock);
        } else if (i == 13) {
            processPointerBlock(blockIdx, 2, processPointerBlock);
        } else if (i == 14) {
            processPointerBlock(blockIdx, 3, processPointerBlock);
        }
    }
}

std::string cmdChmod(std::map<std::string, std::string> params) {
    if (!currentSession.active) return "Error: No hay sesión activa. Ejecute login primero.\n";

    std::string path = params.count("path") ? params["path"] : "";
    std::string ugo = params.count("ugo") ? params["ugo"] : "";
    bool recursive = params.count("r") > 0;

    if (path.empty()) return "Error: Falta parámetro obligatorio -path en chmod.\n";
    if (ugo.empty()) return "Error: Falta parámetro obligatorio -ugo en chmod.\n";

    if (ugo.length() != 3) return "Error: El parámetro -ugo debe tener exactamente 3 dígitos.\n";
    for(char c : ugo) {
        if (c < '0' || c > '7') return "Error: Los dígitos de -ugo deben estar entre 0 y 7.\n";
    }

    std::fstream disk(currentSession.diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco.\n";

    SuperBloque sb{};
    if (!ffReadSuperblock(disk, sb)) { disk.close(); return "Error: No se pudo leer el superbloque.\n"; }

    int targetInodeIdx = 0;
    if (path != "/") {
        targetInodeIdx = ffResolvePath(disk, sb, path, false, currentSession.uid, currentSession.gid);
        if (targetInodeIdx < 0) {
            disk.close();
            return "Error: La ruta '" + path + "' no existe o no tiene permisos de acceso.\n";
        }
    }

    Inode curInode;
    if (!ffReadInode(disk, sb, targetInodeIdx, curInode)) {
        disk.close();
        return "Error: No se pudo leer el inodo del archivo/carpeta.\n";
    }

    // Solo el propietario o root pueden cambiar permisos
    if (currentSession.user != "root" && currentSession.uid != curInode.Uid) {
        disk.close();
        return "Error: Solo el propietario o root puede cambiar los permisos de este archivo/carpeta.\n";
    }

    if (recursive && curInode.Type == '0') {
        recursiveChmod(disk, sb, targetInodeIdx, ugo);
    } else {
        curInode.Perm[0] = ugo[0];
        curInode.Perm[1] = ugo[1];
        curInode.Perm[2] = ugo[2];
        curInode.Mtime = time(nullptr);
        ffWriteInode(disk, sb, targetInodeIdx, curInode);
    }

    if (sb.FilesystemType == 3) {
        ffWriteJournal(disk, sb, "chmod", path, ugo);
    }
    disk.close();
    return "Permisos de " + path + " cambiados a " + ugo + " exitosamente.\n";
}
