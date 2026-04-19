//FileSystem/Loss.cpp
#include "FileSystem.h"
#include "../Mounting/Mounting.h"
#include "../FilesFolders/FilesFolders.h"
#include <iostream>

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

    // 1. Limpiar bitmap de inodos
    disk.seekp(sb.BmInodeStart, std::ios::beg);
    for (int i = 0; i < sb.InodesCount; i++) {
        disk.write(&zero, 1);
    }

    // 2. Limpiar bitmap de bloques
    disk.seekp(sb.BmBlockStart, std::ios::beg);
    for (int i = 0; i < sb.BlocksCount; i++) {
        disk.write(&zero, 1);
    }

    // 3. Limpiar área de inodos
    disk.seekp(sb.InodeStart, std::ios::beg);
    for (int i = 0; i < sb.InodesCount * sb.InodeSize; i++) {
        disk.write(&zero, 1);
    }

    // 4. Limpiar área de bloques
    disk.seekp(sb.BlockStart, std::ios::beg);
    for (long i = 0; i < (long)sb.BlocksCount * sb.BlockSize; i++) {
        disk.write(&zero, 1);
    }

    disk.close();
    return "Pérdida del sistema de archivos en la partición '" + id + "' simulada exitosamente.\n";
}
