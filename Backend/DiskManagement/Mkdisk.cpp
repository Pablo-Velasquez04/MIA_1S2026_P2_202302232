// Mkdisk.cpp
#include <cstdlib>
#include <cstring>
#include "../Estructuras/Structs.h"
#include "../Estructuras/Mbr.h"
#include "../Utilidades/Utilities.h"
#include <iostream>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

std::string cmdMkdisk(std::map<std::string, std::string> params) {
    // Validar parámetros requeridos
    if (params.find("size") == params.end() || params.find("path") == params.end()) {
        return "Error: Parámetros requeridos: size, path";
    }

    // Validar parámetros no reconocidos
    for (const auto& pair : params) {
        if (pair.first != "size" && pair.first != "path" && pair.first != "unit" && pair.first != "fit") {
            return "Error: Parámetro '" + pair.first + "' no es reconocido para mkdisk";
        }
    }

    // Obtener parámetros
    std::string sizeStr = params["size"];
    std::string path = params["path"];
    std::string unit = mayusculas(params.count("unit") && !params["unit"].empty() ? params["unit"] : "M");
    std::string fit = mayusculas(params.count("fit") && !params["fit"].empty() ? params["fit"] : "FF");

    // Validar y convertir tamaño
    int size;
    try {
        size = std::stoi(sizeStr);
        if (size <= 0) {
            return "Error: El tamaño debe ser un número positivo mayor que cero";
        }
    } catch (const std::exception& e) {
        return "Error: El tamaño debe ser un número válido";
    }

    // Aplicar unidades (por defecto Megabytes)
    long sizeInBytes;
    std::string unitDisplay;
    if (unit == "K") {
        sizeInBytes = static_cast<long>(size) * 1024;
        unitDisplay = "KB";
    } else if (unit == "M") {
        sizeInBytes = static_cast<long>(size) * 1024 * 1024;
        unitDisplay = "MB";
    } else {
        return "Error: Unidad '" + unit + "' no válida. Use K para Kilobytes o M para Megabytes";
    }

    // Validar fit (por defecto First Fit)
    char fitByte;
    std::string fitDisplay;
    if (fit == "BF") {
        fitByte = 'B';
        fitDisplay = "Best Fit";
    } else if (fit == "FF") {
        fitByte = 'F';
        fitDisplay = "First Fit";
    } else if (fit == "WF") {
        fitByte = 'W';
        fitDisplay = "Worst Fit";
    } else {
        return "Error: Ajuste '" + fit + "' no válido. Use BF, FF o WF";
    }

    // Cambiar la extensión del archivo a .mia
    path = cambiarExtension(path, ".mia");

    // Verificar si el archivo ya existe
    if (archivoExiste(path)) {
        return "Error: El archivo '" + path + "' ya existe";
    }

    // Crear directorios si no existen
    crearCarpetasSiNoExiste(path);

    // Crear el MBR
    MBR mbr = crearMBR(static_cast<int>(sizeInBytes), fitByte);

    // Crear el archivo del disco con el tamaño especificado y llenar con ceros
    if (!crearArchivoConTamano(path, sizeInBytes)) {
        return "Error: No se pudo crear el archivo de disco";
    }

    // Escribir el MBR al inicio del archivo
    std::vector<char> mbrBytes = mbrToBytes(mbr);
    if (!escribirEstructuraEnArchivo(path, mbrBytes.data(), mbrBytes.size())) {
        return "Error: No se pudo escribir el MBR en el archivo";
    }

    // INFORMACIÓN DETALLADA DE DEPURACIÓN
    std::ostringstream result;
    result << "\n=== DISCO CREADO EXITOSAMENTE ===\n";
    result << "Ruta: " << path << "\n";
    result << "Tamaño: " << sizeInBytes << " bytes (" << std::fixed << std::setprecision(2)
            << static_cast<double>(size) << " " << unitDisplay << ")\n";
    result << "Ajuste: " << fitDisplay << "\n";
    result << "Máximo particiones: 4 primarias (o 3 primarias + 1 extendida)\n";

    // INFORMACIÓN DEL MBR
    result << "\n--- INFORMACIÓN DEL MBR ---\n";
    MBR readMbr;
    if (leerEstructuraDelArchivo(path, &readMbr, sizeof(MBR), 0)) {
        result << "Signature: " << readMbr.Signature << "\n";
        result << "Fecha creación: " << formatearTiempo(readMbr.CreationDate) << "\n";
        result << "Tamaño MBR: " << getMBRSize() << " bytes\n";
        result << "Fit configurado: " << readMbr.Fit << "\n";

        // Mostrar estado inicial de particiones
        result << "\n--- ESTADO INICIAL PARTICIONES ---\n";
        for (int i = 0; i < 4; i++) {
            Partition& partition = readMbr.Partitions[i];
            result << "Partición " << (i + 1) << ": ";
            if (partition.Status == '0' && partition.Start == -1) {
                result << "Libre\n";
            } else {
                result << "Ocupada (" << partition.Type << ", " << partition.Size
                       << " bytes, start: " << partition.Start << ")\n";
            }
        }
    }
    result << "=====================================\n";

    return result.str();
}