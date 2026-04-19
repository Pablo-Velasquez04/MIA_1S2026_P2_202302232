//FilesFolders/Chown.cpp
#include "FilesFolders.h"
#include "../Users/Users.h"
#include <sstream>
#include <cstring>
#include <ctime>

extern ActiveSession currentSession;

static void recursiveChown(std::fstream& disk, const SuperBloque& sb, int inodeIdx, int targetUid) {
    if (inodeIdx < 0 || inodeIdx >= sb.InodesCount) return;

    Inode curInode;
    if (!ffReadInode(disk, sb, inodeIdx, curInode)) return;

    if (currentSession.user != "root" && currentSession.uid != curInode.Uid) {
        return; // No se tienen privilegios para cambiar este archivo
    }

    curInode.Uid = targetUid;
    curInode.Mtime = time(nullptr);
    ffWriteInode(disk, sb, inodeIdx, curInode);

    if (curInode.Type == '1') return; // Es un archivo y ya se editó

    auto processEntry = [&](int blockIdx) {
        FolderBlock fb;
        if (!ffReadFolderBlock(disk, sb, blockIdx, fb)) return;
        for (int e = 0; e < 4; e++) {
            if (fb.content[e].Inodo != -1) {
                std::string entryName(fb.content[e].Name, strnlen(fb.content[e].Name, 12));
                if (entryName == "." || entryName == "..") continue;
                recursiveChown(disk, sb, fb.content[e].Inodo, targetUid);
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

std::string cmdChown(std::map<std::string, std::string> params) {
    if (!currentSession.active) return "Error: No hay sesión activa. Ejecute login primero.\n";

    std::string path = params.count("path") ? params["path"] : "";
    std::string user = params.count("usuario") ? params["usuario"] : "";
    bool recursive = params.count("r") > 0;

    if (path.empty()) return "Error: Falta parámetro obligatorio -path en chown.\n";
    if (user.empty()) return "Error: Falta parámetro obligatorio -usuario en chown.\n";

    std::fstream disk(currentSession.diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco.\n";

    SuperBloque sb{};
    if (!ffReadSuperblock(disk, sb)) { disk.close(); return "Error: No se pudo leer el superbloque.\n"; }

    std::string content = readUsersFile(disk, sb);
    auto lines = parseLines(content);
    int targetUid = -1;
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
        if (fields.size() >= 4 && fields[1] == "U" && fields[3] == user && fields[0] != "0") {
            try { targetUid = std::stoi(fields[0]); } catch (...) {}
            break;
        }
    }

    if (targetUid == -1) {
        disk.close();
        return "Error: " + user + " no existe.\n";
    }

    int startInodeIdx = 0;
    if (path != "/") {
        startInodeIdx = ffResolvePath(disk, sb, path, false, currentSession.uid, currentSession.gid);
        if (startInodeIdx < 0) {
            disk.close();
            return "Error: La ruta '" + path + "' no existe o no tiene permisos.\n";
        }
    }

    Inode curInode;
    if (!ffReadInode(disk, sb, startInodeIdx, curInode)) {
        disk.close();
        return "Error: No se pudo leer inodo inicial.\n";
    }

    if (currentSession.user != "root" && currentSession.uid != curInode.Uid) {
        disk.close();
        return "Error: Solo el propietario o root puede cambiar los permisos.\n";
    }

    if (recursive && curInode.Type == '0') {
        recursiveChown(disk, sb, startInodeIdx, targetUid);
    } else {
        curInode.Uid = targetUid;
        curInode.Mtime = time(nullptr);
        ffWriteInode(disk, sb, startInodeIdx, curInode);
    }

    disk.close();
    return "Propietario de " + path + " cambiado a " + user + " exitosamente.\n";
}
