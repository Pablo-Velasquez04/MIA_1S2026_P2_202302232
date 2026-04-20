//FilesFolders/Helpers.cpp
//Implementación de todos los helpers compartidos de EXT2
#include "FilesFolders.h"
#include "../Utilidades/Utilities.h"
#include <cstring>
#include <ctime>

extern ActiveSession currentSession;

// I/O BÁSICO - Using Utilities.h

bool ffReadSuperblock(std::fstream& disk, SuperBloque& sb) {
    return leerStruct(disk, currentSession.sbStart, sb);
}

//BITMAP — asignar inodo/bloque libre

int ffAllocInode(std::fstream& disk, SuperBloque& sb) {
    for (int i = 0; i < sb.InodesCount; i++) {
        char bit = 0;
        disk.seekg(sb.BmInodeStart + i);
        disk.read(&bit, 1);
        if (bit == '0') {
            char one = '1';
            disk.seekp(sb.BmInodeStart + i);
            disk.write(&one, 1);
            sb.FreeInodesCount--;
            sb.FirstIno = i + 1;
            escribirStruct(disk, currentSession.sbStart, sb);
            return i;
        }
    }
    return -1; //sin espacio
}

int ffAllocBlock(std::fstream& disk, SuperBloque& sb) {
    for (int i = 0; i < sb.BlocksCount; i++) {
        char bit = 0;
        disk.seekg(sb.BmBlockStart + i);
        disk.read(&bit, 1);
        if (bit == '0') {
            char one = '1';
            disk.seekp(sb.BmBlockStart + i);
            disk.write(&one, 1);
            sb.FreeBlocksCount--;
            sb.FirstBlo = i + 1;
            escribirStruct(disk, currentSession.sbStart, sb);
            return i;
        }
    }
    return -1;
}

//INODOS Y BLOQUES
bool ffReadInode(std::fstream& disk, const SuperBloque& sb,
                 int idx, Inode& out) {
    long pos = sb.InodeStart + (long)idx * sb.InodeSize;
    return leerStruct(disk, pos, out);
}

bool ffWriteInode(std::fstream& disk, const SuperBloque& sb,
                  int idx, const Inode& in) {
    long pos = sb.InodeStart + (long)idx * sb.InodeSize;
    return escribirStruct(disk, pos, in);
}

bool ffReadFolderBlock(std::fstream& disk, const SuperBloque& sb,
                       int idx, FolderBlock& out) {
    long pos = sb.BlockStart + (long)idx * sb.BlockSize;
    return leerStruct(disk, pos, out);
}

bool ffWriteFolderBlock(std::fstream& disk, const SuperBloque& sb,
                        int idx, const FolderBlock& fb) {
    long pos = sb.BlockStart + (long)idx * sb.BlockSize;
    return escribirStruct(disk, pos, fb);
}

bool ffReadFileBlock(std::fstream& disk, const SuperBloque& sb,
                     int idx, FileBlock& out) {
    long pos = sb.BlockStart + (long)idx * sb.BlockSize;
    return leerStruct(disk, pos, out);
}

bool ffWriteFileBlock(std::fstream& disk, const SuperBloque& sb,
                      int idx, const FileBlock& fb) {
    long pos = sb.BlockStart + (long)idx * sb.BlockSize;
    return escribirStruct(disk, pos, fb);
}

//PERMISOS UGO
//op: 'r'=lectura, 'w'=escritura, 'x'=ejecución

bool ffHasPermission(const Inode& inode, int uid, int gid, char op) {
    //Root siempre tiene 777
    if (uid == 1) return true;

    //Determinar categoría
    char permChar;
    if (uid == inode.Uid)      permChar = inode.Perm[0]; //User
    else if (gid == inode.Gid) permChar = inode.Perm[1]; //Group
    else                       permChar = inode.Perm[2]; //Otros

    int perm = permChar - '0';
    if (op == 'r') return (perm & 4) != 0;
    if (op == 'w') return (perm & 2) != 0;
    if (op == 'x') return (perm & 1) != 0;
    return false;
}

//SPLIT PATH
//"/home/user/docs" → ["home","user","docs"]
std::vector<std::string> ffSplitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : path) {
        if (c == '/') {
            if (!current.empty()) {
                parts.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty())
        parts.push_back(current);
    return parts;
}

//LOOKUP — buscar nombre en carpeta
int ffLookup(std::fstream& disk, const SuperBloque& sb,
             int dirInodeIdx, const std::string& name) {
    Inode dirInode{};
    if (!ffReadInode(disk, sb, dirInodeIdx, dirInode)) return -1;
    if (dirInode.Type != '0') return -1; // no es carpeta

    //Recorrer bloques directos de la carpeta
    for (int i = 0; i < 12; i++) {
        if (dirInode.Block[i] == -1) break;

        FolderBlock fb{};
        if (!ffReadFolderBlock(disk, sb, dirInode.Block[i], fb)) break;

        for (int j = 0; j < 4; j++) {
            if (fb.content[j].Inodo == -1) continue;
            //Comparar nombre (null-terminated)
            std::string entryName(fb.content[j].Name,
                strnlen(fb.content[j].Name, 12));
            if (entryName == name)
                return fb.content[j].Inodo;
        }
    }
    return -1;
}

//ADD ENTRY — agregar entrada en carpeta
bool ffAddEntry(std::fstream& disk, SuperBloque& sb,
                int dirInodeIdx, const std::string& name, int inodeIdx) {
    Inode dirInode{};
    if (!ffReadInode(disk, sb, dirInodeIdx, dirInode)) return false;

    //Buscar espacio libre en bloques existentes
    for (int i = 0; i < 12; i++) {
        if (dirInode.Block[i] == -1) break;

        FolderBlock fb{};
        if (!ffReadFolderBlock(disk, sb, dirInode.Block[i], fb)) continue;

        for (int j = 0; j < 4; j++) {
            if (fb.content[j].Inodo == -1) {
                //Espacio libre encontrado
                memset(fb.content[j].Name, 0, 12);
                strncpy(fb.content[j].Name, name.c_str(), 11);
                fb.content[j].Inodo = inodeIdx;
                ffWriteFolderBlock(disk, sb, dirInode.Block[i], fb);

                //Actualizar tamaño del inodo carpeta
                dirInode.Size  += sizeof(Content);
                dirInode.Mtime  = time(nullptr);
                ffWriteInode(disk, sb, dirInodeIdx, dirInode);
                return true;
            }
        }
    }

    //Necesita un nuevo bloque para la carpeta
    int newBlock = ffAllocBlock(disk, sb);
    if (newBlock == -1) return false;

    //Buscar slot libre en Block[] del inodo
    int slotIdx = -1;
    for (int i = 0; i < 12; i++) {
        if (dirInode.Block[i] == -1) { slotIdx = i; break; }
    }
    if (slotIdx == -1) return false; //carpeta llena (>48 entradas)

    //Inicializar nuevo bloque
    FolderBlock newFb{};
    for (int j = 0; j < 4; j++) {
        newFb.content[j].Inodo = -1;
        memset(newFb.content[j].Name, 0, 12);
    }
    memset(newFb.content[0].Name, 0, 12);
    strncpy(newFb.content[0].Name, name.c_str(), 11);
    newFb.content[0].Inodo = inodeIdx;

    ffWriteFolderBlock(disk, sb, newBlock, newFb);

    dirInode.Block[slotIdx] = newBlock;
    dirInode.Size           += sizeof(Content);
    dirInode.Mtime           = time(nullptr);
    ffWriteInode(disk, sb, dirInodeIdx, dirInode);

    return true;
}

//RESOLVE PATH — navegar/crear árbol de directorios
int ffResolvePath(std::fstream& disk, SuperBloque& sb,
                  const std::string& path,
                  bool createParents,
                  int ownerUid, int ownerGid) {
    auto parts = ffSplitPath(path);
    if (parts.empty()) return 0; //raíz

    int currentInodeIdx = 0; //inodo raíz

    for (size_t i = 0; i < parts.size(); i++) {
        const std::string& part = parts[i];
        int childIdx = ffLookup(disk, sb, currentInodeIdx, part);

        if (childIdx == -1) {
            if (!createParents) return -1; //no existe y no crear

            //Verificar permiso de escritura en carpeta actual
            Inode currentInode{};
            ffReadInode(disk, sb, currentInodeIdx, currentInode);
            if (!ffHasPermission(currentInode, ownerUid, ownerGid, 'w'))
                return -2; // sin permiso

            //Crear nueva carpeta
            int newInodeIdx = ffAllocInode(disk, sb);
            if (newInodeIdx == -1) return -1;

            int newBlockIdx = ffAllocBlock(disk, sb);
            if (newBlockIdx == -1) return -1;

            //Inicializar inodo carpeta
            time_t now = time(nullptr);
            Inode newInode{};
            newInode.Uid     = ownerUid;
            newInode.Gid     = ownerGid;
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

            //Inicializar bloque carpeta con "." y ".."
            FolderBlock newFb{};
            for (int j = 0; j < 4; j++) {
                newFb.content[j].Inodo = -1;
                memset(newFb.content[j].Name, 0, 12);
            }
            strncpy(newFb.content[0].Name, ".",  11);
            newFb.content[0].Inodo = newInodeIdx;
            strncpy(newFb.content[1].Name, "..", 11);
            newFb.content[1].Inodo = currentInodeIdx;
            ffWriteFolderBlock(disk, sb, newBlockIdx, newFb);

            //Agregar entrada en carpeta padre
            if (!ffAddEntry(disk, sb, currentInodeIdx, part, newInodeIdx))
                return -1;

            currentInodeIdx = newInodeIdx;
        } else {
            //Verificar que es carpeta (excepto el último componente)
            if (i < parts.size() - 1) {
                Inode childInode{};
                ffReadInode(disk, sb, childIdx, childInode);
                if (childInode.Type != '0') return -1; //no es carpeta
            }
            currentInodeIdx = childIdx;
        }
    }
    return currentInodeIdx;
}

bool ffReadPointerBlock(std::fstream& disk, const SuperBloque& sb,
                        int idx, PointerBlock& out) {
    long pos = sb.BlockStart + (long)idx * sb.BlockSize;
    return leerStruct(disk, pos, out);
}

bool ffWritePointerBlock(std::fstream& disk, const SuperBloque& sb,
                         int idx, const PointerBlock& pb) {
    long pos = sb.BlockStart + (long)idx * sb.BlockSize;
    return escribirStruct(disk, pos, pb);
}
void ffFreeInode(std::fstream& disk, SuperBloque& sb, int idx) {
    char zero = '0';
    disk.seekp(sb.BmInodeStart + idx);
    disk.write(&zero, 1);
    sb.FreeInodesCount++;
    escribirStruct(disk, currentSession.sbStart, sb);
}

void ffFreeBlock(std::fstream& disk, SuperBloque& sb, int idx) {
    char zero = '0';
    disk.seekp(sb.BmBlockStart + idx);
    disk.write(&zero, 1);
    sb.FreeBlocksCount++;
    escribirStruct(disk, currentSession.sbStart, sb);
}

bool ffRemoveEntry(std::fstream& disk, SuperBloque& sb,
                   int dirInodeIdx, const std::string& name) {
    Inode dirInode{};
    if (!ffReadInode(disk, sb, dirInodeIdx, dirInode)) return false;

    for (int i = 0; i < 12; i++) {
        if (dirInode.Block[i] == -1) break;

        FolderBlock fb{};
        if (!ffReadFolderBlock(disk, sb, dirInode.Block[i], fb)) continue;

        for (int j = 0; j < 4; j++) {
            if (fb.content[j].Inodo != -1) {
                std::string entryName(fb.content[j].Name, strnlen(fb.content[j].Name, 12));
                if (entryName == name) {
                    fb.content[j].Inodo = -1;
                    memset(fb.content[j].Name, 0, 12);
                    ffWriteFolderBlock(disk, sb, dirInode.Block[i], fb);

                    dirInode.Size -= sizeof(Content);
                    dirInode.Mtime = time(nullptr);
                    ffWriteInode(disk, sb, dirInodeIdx, dirInode);
                    return true;
                }
            }
        }
    }
    return false;
}

void ffWriteJournal(std::fstream& disk, const SuperBloque& sb, const std::string& op, const std::string& path, const std::string& content) {
    if (sb.FilesystemType != 3) return;

    long journalStart = currentSession.sbStart + sizeof(SuperBloque);
    int journalCount = sb.InodesCount;

    for (int i = 0; i < journalCount; i++) {
        Journaling j;
        long pos = journalStart + (long)i * sizeof(Journaling);
        leerStruct(disk, pos, j);
        
        if (j.Operation[0] == '\0') {
            memset(&j, 0, sizeof(Journaling));
            strncpy(j.Operation, op.c_str(), sizeof(j.Operation) - 1);
            j.Operation[sizeof(j.Operation) - 1] = '\0';
            strncpy(j.Path, path.c_str(), sizeof(j.Path) - 1);
            j.Path[sizeof(j.Path) - 1] = '\0';
            strncpy(j.Content, content.c_str(), sizeof(j.Content) - 1);
            j.Content[sizeof(j.Content) - 1] = '\0';
            j.Date = time(nullptr);
            escribirStruct(disk, pos, j);
            break;
        }
    }
}
