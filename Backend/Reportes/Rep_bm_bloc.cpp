//Reportes/Rep_bm_bloc.cpp

#include "Reportes.h"
#include "RepHelpers.h"

#include <fstream>


std::string repBmBlock(const std::string& diskPath, const std::string& outPath, long sbStart) {
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco " + diskPath;
    SuperBloque sb;
    disk.seekg(sbStart);
    disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    disk.close();
    return repBitmap(diskPath, sb.BmBlockStart, sb.BlocksCount, outPath, "BITMAP DE BLOQUES");
}
