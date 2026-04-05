// DiskManagement/Mounted.cpp
#include "../DiskManagement/DiskManagement.h"
#include "Mounting.h"
#include "../Estructuras/Structs.h"
#include "../Utilidades/Utilities.h"
#include <sstream>
#include <iomanip>

//MOUNTED — lista todas las particiones montadas en RAM

std::string cmdMounted() {
    std::ostringstream out;

    const auto& mounted = getMountedPartitions();

    if (mounted.empty()) {
        return "No hay particiones montadas actualmente\n";
    }

    out << "=============== PARTICIONES MONTADAS =============\n";
    out << std::left
        << std::setw(8)  << "ID"
        << std::setw(12) << "Nombre"
        << std::setw(8)  << "Tipo"
        << std::setw(12) << "Inicio"
        << std::setw(12) << "Tamaño"
        << "Disco\n";
    out << std::string(70, '-') << "\n";

    for (const auto& mp : mounted) {
        std::string tipo;
        switch (mp.Type) {
            case 'P': tipo = "Primaria";   break;
            case 'E': tipo = "Extendida";  break;
            case 'L': tipo = "Lógica";     break;
            default:  tipo = "Desconocido";
        }

        out << std::left
            << std::setw(8)  << mp.Id
            << std::setw(12) << mp.Name
            << std::setw(8)  << tipo
            << std::setw(12) << mp.Start
            << std::setw(12) << mp.Size
            << mp.Path << "\n";
    }

    out << std::string(70, '-') << "\n";
    out << "Total: " << mounted.size() << " partición(es) montada(s)\n";

    return out.str();
}