//FileSystem/Journaling.cpp
#include "FileSystem.h"
#include "../Mounting/Mounting.h"
#include "../Estructuras/Structs.h"
#include "../Utilidades/Utilities.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>

std::string cmdJournaling(std::map<std::string, std::string> params) {
    std::string id = params.count("id") ? params["id"] : "";

    if (id.empty()) {
        return "Error: Falta el parámetro obligatorio -id.\n";
    }

    const MountedPartition* mp = getMountedById(id);
    if (!mp) {
        return "Error: No existe una partición montada con el ID '" + id + "'.\n";
    }

    std::fstream disk(mp->Path, std::ios::binary | std::ios::in);
    if (!disk.is_open()) {
        return "Error: No se pudo abrir el disco.\n";
    }

    SuperBloque sb{};
    leerStruct(disk, mp->Start, sb);

    if (sb.Magic != 0xEF53) {
        disk.close();
        return "Error: No se encontró un sistema de archivos válido.\n";
    }

    if (sb.FilesystemType != 3) {
        disk.close();
        return "Error: El sistema de archivos no es EXT3, no tiene Journaling.\n";
    }

    long journalStart = (long)mp->Start + sizeof(SuperBloque);
    int journalCount = sb.InodesCount;

    std::ostringstream json;
    json << "[\n";
    bool first = true;

    for (int i = 0; i < journalCount; i++) {
        Journaling j;
        long pos = journalStart + (long)i * sizeof(Journaling);
        leerStruct(disk, pos, j);

        if (j.Operation[0] != '\0') {
            if (!first) json << ",\n";
            first = false;

            char dateStr[25];
            struct tm* timeinfo = localtime(&j.Date);
            strftime(dateStr, 25, "%Y-%m-%d %H:%M:%S", timeinfo);

            // Escapar comillas simples/dobles si fuera necesario, 
            // pero para este ámbito los strings son controlados (mkdir, rm, etc)
            json << "  {\n";
            json << "    \"operation\": \"" << j.Operation << "\",\n";
            json << "    \"path\": \"" << j.Path << "\",\n";
            json << "    \"content\": \"" << j.Content << "\",\n";
            json << "    \"date\": \"" << dateStr << "\"\n";
            json << "  }";
        }
    }
    json << "\n]";

    disk.close();
    return json.str();
}
