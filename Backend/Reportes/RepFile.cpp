//Reportes/RepFile.cpp
#include "Reportes.h"
#include "RepHelpers.h"
#include <fstream>
#include <sstream>
#include "../FilesFolders/FilesFolders.h"

std::string repFile(const std::string& diskPath, const std::string& outPath, long sbStart, const std::string& pathFile) {
    if (pathFile.empty()) return "Error: Falta parámetro -path_file_ls para reporte file\n";
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco " + diskPath;
    SuperBloque sb;
    disk.seekg(sbStart);
    disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    if (disk.fail()) { disk.close(); return "Error: No se pudo leer el SuperBloque"; }
    int inodeIdx = ffResolvePath(disk, sb, pathFile, false, 1, 1);
    if (inodeIdx < 0) { disk.close(); return "Error: El archivo '" + pathFile + "' no existe\n"; }
    Inode inode;
    if (!ffReadInode(disk, sb, inodeIdx, inode)) { disk.close(); return "Error: No se pudo leer el inodo\n"; }
    if (inode.Type == '0') { disk.close(); return "Error: '" + pathFile + "' es una carpeta, no un archivo\n"; }
    std::string content = readFileContent(disk, sb, inode);
    disk.close();
    std::string fileName = fs::path(pathFile).filename().string();
    std::string actualPath = outPath;
    if (fs::path(actualPath).extension() != ".txt") {
        actualPath = fs::path(actualPath).replace_extension(".txt").string();
    }

    prepareOutputPath(actualPath);
    std::ofstream out(actualPath);
    if (!out.is_open()) return "Error: No se pudo crear " + actualPath;
    out << "Archivo: " << fileName << "\n" << "Ruta   : " << pathFile << "\n" << "Tamaño : " << inode.Size << " bytes\n" << std::string(40, '-') << "\n" << content << "\n";
    out.close();
    return "Reporte FILE generado: " + actualPath;
}
