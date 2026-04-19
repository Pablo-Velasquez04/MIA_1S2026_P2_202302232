//FilesFolders/FilesFolders.h
#pragma once
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include "../Estructuras/Structs.h"
#include "../FileSystem/FileSystem.h"

//Comandos
std::string cmdMkdir (std::map<std::string, std::string> params);
std::string cmdMkfile(std::map<std::string, std::string> params);
std::string cmdCat   (std::map<std::string, std::string> params);
std::string cmdRemove(std::map<std::string, std::string> params);
std::string cmdRename(std::map<std::string, std::string> params);
std::string cmdCopy  (std::map<std::string, std::string> params);
std::string cmdMove  (std::map<std::string, std::string> params);

//Helpers compartidos EXT2

//Superbloque
bool ffReadSuperblock(std::fstream& disk, SuperBloque& sb);

//Bitmap
int  ffAllocInode(std::fstream& disk, SuperBloque& sb); // retorna índice
int  ffAllocBlock(std::fstream& disk, SuperBloque& sb); // retorna índice

//Inodos y bloques
bool ffReadInode (std::fstream& disk, const SuperBloque& sb, int idx, Inode& out);
bool ffWriteInode(std::fstream& disk, const SuperBloque& sb, int idx, const Inode& in);
bool ffReadFolderBlock (std::fstream& disk, const SuperBloque& sb, int idx, FolderBlock& out);
bool ffWriteFolderBlock(std::fstream& disk, const SuperBloque& sb, int idx, const FolderBlock& fb);
bool ffReadFileBlock   (std::fstream& disk, const SuperBloque& sb, int idx, FileBlock& out);
bool ffWriteFileBlock  (std::fstream& disk, const SuperBloque& sb, int idx, const FileBlock& fb);

//Permisos UGO
bool ffHasPermission(const Inode& inode, int uid, int gid, char op); // op: 'r','w','x'

//Navegación de árbol
//Busca 'name' dentro del FolderBlock del inodo de carpeta 'dirInodeIdx'
//Retorna índice de inodo del hijo, -1 si no existe
int  ffLookup(std::fstream& disk, const SuperBloque& sb,
              int dirInodeIdx, const std::string& name);

bool ffRemoveEntry(std::fstream& disk, SuperBloque& sb,
                   int dirInodeIdx, const std::string& name);

//Agrega una entrada (name, inodeIdx) en la carpeta dirInodeIdx
//Retorna true si tuvo éxito
bool ffAddEntry(std::fstream& disk, SuperBloque& sb,
                int dirInodeIdx, const std::string& name, int inodeIdx);

void ffFreeInode(std::fstream& disk, SuperBloque& sb, int idx);
void ffFreeBlock(std::fstream& disk, SuperBloque& sb, int idx);

//Navega la ruta y retorna el inodo final, -1 si no existe
//Si createParents=true crea las carpetas que falten (para -r y -p)
int  ffResolvePath(std::fstream& disk, SuperBloque& sb,
                   const std::string& path,
                   bool createParents,
                   int ownerUid, int ownerGid);

//Divide una ruta en partes: "/home/user/docs" -> ["home","user","docs"]
std::vector<std::string> ffSplitPath(const std::string& path);
bool ffReadPointerBlock (std::fstream& disk, const SuperBloque& sb, int idx, PointerBlock& out);
bool ffWritePointerBlock(std::fstream& disk, const SuperBloque& sb, int idx, const PointerBlock& pb);