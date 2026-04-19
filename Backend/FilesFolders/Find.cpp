//FilesFolders/Find.cpp
#include "FilesFolders.h"
#include <sstream>
#include <regex>
#include <iostream>
#include <cstring>

extern ActiveSession currentSession;

static void recursiveFind(std::fstream& disk, const SuperBloque& sb, int inodeIdx, const std::regex& nameRegex, std::string& output, std::string indent) {
    if (inodeIdx < 0 || inodeIdx >= sb.InodesCount) return;

    Inode curInode;
    if (!ffReadInode(disk, sb, inodeIdx, curInode)) return;
    if (curInode.Type == '1') return; // Solo iteramos contenido de carpetas

    auto processEntry = [&](int blockIdx) {
        FolderBlock fb;
        if (!ffReadFolderBlock(disk, sb, blockIdx, fb)) return;
        for (int e = 0; e < 4; e++) {
            if (fb.content[e].Inodo != -1) {
                std::string entryName(fb.content[e].Name, strnlen(fb.content[e].Name, 12));
                if (entryName == "." || entryName == "..") continue;

                if (std::regex_match(entryName, nameRegex)) {
                    output += indent + "|_ " + entryName + "\n";
                }

                Inode childInode;
                if (ffReadInode(disk, sb, fb.content[e].Inodo, childInode) && childInode.Type == '0') {
                    recursiveFind(disk, sb, fb.content[e].Inodo, nameRegex, output, indent + "  ");
                }
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

std::string cmdFind(std::map<std::string, std::string> params) {
    if (!currentSession.active) return "Error: No hay sesión activa. Ejecute login primero.\n";

    std::string path = params.count("path") ? params["path"] : "";
    std::string name = params.count("name") ? params["name"] : "";

    if (path.empty() || name.empty())
        return "Error: Faltan parámetros obligatorios -path y -name.\n";

    std::fstream disk(currentSession.diskPath, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco.\n";

    SuperBloque sb{};
    if (!ffReadSuperblock(disk, sb)) { disk.close(); return "Error: No se pudo leer el superbloque.\n"; }

    int startInodeIdx = 0;
    if (path != "/") {
        startInodeIdx = ffResolvePath(disk, sb, path, false, currentSession.uid, currentSession.gid);
        if (startInodeIdx == -2) {
            disk.close();
            return "Error: Sin permiso en la ruta indicada.\n";
        }
        if (startInodeIdx == -1) {
            disk.close();
            return "Error: La ruta -path indicada no existe.\n";
        }
    }

    Inode parentInode{};
    ffReadInode(disk, sb, startInodeIdx, parentInode);
    if (!ffHasPermission(parentInode, currentSession.uid, currentSession.gid, 'r')) {
        disk.close();
        return "Error: Sin permiso de lectura en la ruta indicada.\n";
    }

    if (parentInode.Type == '1') {
        disk.close();
        return "Error: El -path debe ser una carpeta, no un archivo.\n";
    }

    // Convert wildcards: * -> .+, ? -> .
    // The prompt says: *: Uno o más caracteres, ?: Un solo carácter
    std::string regexStr = "";
    for (char c : name) {
        if (c == '*') regexStr += ".+";
        else if (c == '?') regexStr += ".";
        else if (c == '.' || c == '[' || c == ']' || c == '(' || c == ')' || c == '{' || c == '}' || c == '+' || c == '^' || c == '$' || c == '|' || c == '\\') {
            regexStr += "\\"; regexStr += c;
        }
        else regexStr += c;
    }

    std::regex nameRegex;
    try {
        nameRegex = std::regex(regexStr);
    } catch (...) {
        disk.close();
        return "Error: Patrón de búsqueda inválido en -name.\n";
    }

    std::string output = path + "\n";
    recursiveFind(disk, sb, startInodeIdx, nameRegex, output, "");

    disk.close();
    return output;
}
