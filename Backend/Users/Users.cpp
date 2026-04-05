// Users/Users.cpp
// Implementación de todos los helpers compartidos
#include "Users.h"
#include "../Utilidades/Utilities.h"
#include "../FilesFolders/FilesFolders.h" // Para ffAllocBlock
#include <sstream>
#include <cstring>
#include <ctime>
#include <algorithm>

extern ActiveSession currentSession;

// I/O básico 

// I/O básico - Using Utilities.h

//Superbloque 
bool readSuperblock(std::fstream& disk, SuperBloque& sb) {
    return leerStruct(disk, currentSession.sbStart, sb);
}

// Leer users.txt
std::string readUsersFile(std::fstream& disk, const SuperBloque& sb) {
    long inodePos = sb.InodeStart + 1 * sb.InodeSize;
    Inode usersInode{};
    if (!leerStruct(disk, inodePos, usersInode))
        return "";

    std::string content;
    for (int i = 0; i < 12; i++) {
        if (usersInode.Block[i] == -1) break;

        long blockPos = sb.BlockStart + (long)usersInode.Block[i] * sb.BlockSize;
        FileBlock fb{};
        if (!leerStruct(disk, blockPos, fb)) break;

        int remaining = usersInode.Size - (int)content.size();
        int toRead    = std::min(remaining, sb.BlockSize);
        if (toRead <= 0) break;

        content.append(fb.content, toRead);
    }
    return content;
}

//Escribir users.txt

bool writeUsersFile(std::fstream& disk, const SuperBloque& sb,
                    const std::string& newContent) {
    long inodePos = sb.InodeStart + 1 * sb.InodeSize;
    Inode usersInode{};
    if (!leerStruct(disk, inodePos, usersInode))
        return false;

    int contentSize  = (int)newContent.size();
    int blockSize    = sb.BlockSize;
    int blocksNeeded = (contentSize + blockSize - 1) / blockSize;

    SuperBloque sbUpdated{};
    leerStruct(disk, currentSession.sbStart, sbUpdated);

    for (int i = 0; i < blocksNeeded; i++) {
        if (usersInode.Block[i] != -1) continue;

        int freeBlock = ffAllocBlock(disk, sbUpdated);
        if (freeBlock == -1) return false;

        usersInode.Block[i] = freeBlock;
    }

    for (int i = 0; i < blocksNeeded; i++) {
        FileBlock fb{};
        memset(fb.content, 0, blockSize);
        int offset = i * blockSize;
        int toCopy = std::min(blockSize, contentSize - offset);
        memcpy(fb.content, newContent.c_str() + offset, toCopy);
        long blockPos = sb.BlockStart + (long)usersInode.Block[i] * blockSize;
        escribirStruct(disk, blockPos, fb);
    }

    usersInode.Size  = contentSize;
    usersInode.Mtime = time(nullptr);
    escribirStruct(disk, inodePos, usersInode);
    escribirStruct(disk, currentSession.sbStart, sbUpdated);

    return true;
}

//Parseo de líneas

std::vector<std::string> parseLines(const std::string& content) {
    std::vector<std::string> lines;
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

std::string buildContent(const std::vector<std::string>& lines) {
    std::string content;
    for (const auto& line : lines)
        content += line + "\n";
    return content;
}

//IDs y existencia 

int getNextId(const std::vector<std::string>& lines, const std::string& type) {
    int maxId = 0;
    for (const auto& line : lines) {
        if (line.empty()) continue;
        std::vector<std::string> fields;
        std::istringstream ls(line);
        std::string f;
        while (std::getline(ls, f, ',')) {
            f.erase(0, f.find_first_not_of(" "));
            f.erase(f.find_last_not_of(" ") + 1);
            fields.push_back(f);
        }
        if (fields.size() >= 2 && fields[1] == type && fields[0] != "0") {
            try {
                int id = std::stoi(fields[0]);
                if (id > maxId) maxId = id;
            } catch (...) {}
        }
    }
    return maxId + 1;
}

bool groupExists(const std::vector<std::string>& lines, const std::string& name) {
    for (const auto& line : lines) {
        if (line.empty()) continue;
        std::vector<std::string> fields;
        std::istringstream ls(line);
        std::string f;
        while (std::getline(ls, f, ',')) {
            f.erase(0, f.find_first_not_of(" "));
            f.erase(f.find_last_not_of(" ") + 1);
            fields.push_back(f);
        }
        if (fields.size() >= 3 && fields[1] == "G" &&
            fields[2] == name   && fields[0] != "0")
            return true;
    }
    return false;
}

bool userExists(const std::vector<std::string>& lines, const std::string& name) {
    for (const auto& line : lines) {
        if (line.empty()) continue;
        std::vector<std::string> fields;
        std::istringstream ls(line);
        std::string f;
        while (std::getline(ls, f, ',')) {
            f.erase(0, f.find_first_not_of(" "));
            f.erase(f.find_last_not_of(" ") + 1);
            fields.push_back(f);
        }
        if (fields.size() >= 4 && fields[1] == "U" &&
            fields[3] == name   && fields[0] != "0")
            return true;
    }
    return false;
}

std::fstream openSessionDisk() {
    return std::fstream(currentSession.diskPath,
                        std::ios::binary | std::ios::in | std::ios::out);
}