//FileSystem/Loss.cpp
#include "FileSystem.h"
#include "../Mounting/Mounting.h"
#include "../FilesFolders/FilesFolders.h"
#include <iostream>
#include <vector>

std::string cmdLoss(std::map<std::string, std::string> params) {
    std::string id = params.count("id") ? params["id"] : "";

    if (id.empty()) {
        return "Error: Falta el parámetro obligatorio -id.\n";
    }

    const MountedPartition* mp = getMountedById(id);
    if (!mp) {
        return "Error: No existe una partición montada con el ID '" + id + "'.\n";
    }

    std::fstream disk(mp->Path, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open()) {
        return "Error: No se pudo abrir el disco en la ruta '" + mp->Path + "'.\n";
    }

    SuperBloque sb{};
    disk.seekg(mp->Start, std::ios::beg);
    disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));

    if (sb.Magic != 0xEF53) {
        disk.close();
        return "Error: La partición '" + id + "' no tiene un sistema de archivos EXT2/EXT3 válido o ya está corrupto.\n";
    }

    char zero = '\0';

    std::vector<int> protectedInodes = {0, 1};
    std::vector<long> protectedBlocks;
    
    for (int i : protectedInodes) {
        Inode in{};
        long pos = sb.InodeStart + i * sb.InodeSize;
        disk.seekg(pos);
        if (disk.read((char*)&in, sizeof(Inode))) {
            for(int j=0; j<15; j++) {
                if (in.Block[j] != -1) {
                    protectedBlocks.push_back(in.Block[j]);
                }
            }
        }
    }

    auto isProtectedInode = [&](int idx) {
        for (int p : protectedInodes) if (p == idx) return true;
        return false;
    };
    auto isProtectedBlock = [&](long idx) {
        for (long p : protectedBlocks) if (p == idx) return true;
        return false;
    };

    // 1. Limpiar bitmap de inodos
    for (int i = 0; i < sb.InodesCount; i++) {
        if (!isProtectedInode(i)) {
            disk.seekp(sb.BmInodeStart + i, std::ios::beg);
            disk.write(&zero, 1);
        }
    }

    // 2. Limpiar bitmap de bloques
    for (int i = 0; i < sb.BlocksCount; i++) {
        if (!isProtectedBlock(i)) {
            disk.seekp(sb.BmBlockStart + i, std::ios::beg);
            disk.write(&zero, 1);
        }
    }

    // 3. Limpiar área de inodos
    for (int i = 0; i < sb.InodesCount; i++) {
        if (!isProtectedInode(i)) {
            disk.seekp(sb.InodeStart + i * sb.InodeSize, std::ios::beg);
            for(int b = 0; b < sb.InodeSize; b++) disk.write(&zero, 1);
        }
    }

    // 4. Limpiar área de bloques
    for (long i = 0; i < sb.BlocksCount; i++) {
        if (!isProtectedBlock(i)) {
            disk.seekp(sb.BlockStart + i * sb.BlockSize, std::ios::beg);
            for(int b = 0; b < sb.BlockSize; b++) disk.write(&zero, 1);
        }
    }

    disk.close();
    return "Pérdida del sistema de archivos en la partición '" + id + "' simulada exitosamente.\n";
}
