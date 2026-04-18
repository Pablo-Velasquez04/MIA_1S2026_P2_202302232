// DiskManagement/Mount.cpp
#include "../DiskManagement/DiskManagement.h"
#include "Mounting.h"
#include "../Estructuras/Structs.h"
#include "../Utilidades/Utilities.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <vector>
#include <map>
#include "../Estructuras/Ebr.h"

//ESTADO GLOBAL EN RAM

static std::vector<MountedPartition> mountedPartitions;

// Orden de aparición de cada disco (path → letra)
// Disco2.mia fue el primero  → 'A'
// Disco1.mia fue el segundo  → 'B'
// Disco3.mia fue el tercero  → 'C'
static std::map<std::string, char>   diskLetterMap;
static std::map<std::string, int>    diskPartCount;  //cuántas particiones por disco
static char nextLetter = 'A';

//ACCESO GLOBAL — lo usan MKFS, LOGIN, REP, etc.

const std::vector<MountedPartition>& getMountedPartitions() {
    return mountedPartitions;

}

//Buscar partición montada por ID — la usará MKFS, LOGIN, REP
const MountedPartition* getMountedById(const std::string& id) {
    std::string searchId = mayusculas(id);
    for (const auto& mp : mountedPartitions) {
        if (mayusculas(mp.Id) == searchId) {
            return &mp;
        }
    }
    return nullptr;
}

//HELPERS

static bool readMBR(std::fstream& disk, MBR& mbr) {
    disk.seekg(0);
    disk.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    return !disk.fail();
}

static bool writeMBR(std::fstream& disk, const MBR& mbr) {
    disk.seekp(0);
    disk.write(reinterpret_cast<const char*>(&mbr), sizeof(MBR));
    return !disk.fail();
}

static bool readEBR(std::fstream& disk, int pos, EBR& ebr) {
    disk.seekg(pos);
    disk.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
    return !disk.fail();
}

static bool writeEBR(std::fstream& disk, int pos, const EBR& ebr) {
    disk.seekp(pos);
    disk.write(reinterpret_cast<const char*>(&ebr), sizeof(EBR));
    return !disk.fail();
}

static std::string trimNull(const char* p) {
    std::string s(p);
    size_t pos = s.find('\0');
    if (pos != std::string::npos) s.resize(pos);
    return s;
}

static bool isAlreadyMounted(const std::string& path, const std::string& name) {
    for (const auto& mp : mountedPartitions)
        if (mp.Path == path && mp.Name == name)
            return true;
    return false;
}

// LÓGICA DEL ID
// Regla del enunciado:
//   - Mismo disco    → incrementa número, misma letra
//   - Disco nuevo    → siguiente letra, número reinicia en 1
//   Formato: [2 dígitos carnet][número][letra]
//   Ejemplo carnet 202302232 → sufijo "32"
//     Disco2 Part2 → 321A
//     Disco2 Part1 → 322A
//     Disco1 Part3 → 321B   ← letra B porque Disco1 es nuevo, número reinicia
//     Disco3 Part2 → 321C

static std::string generateId(const std::string& path) {
    const std::string carnetSuffix = "32";

    //Si el disco no tiene letra aún, asignarle la siguiente
    if (diskLetterMap.find(path) == diskLetterMap.end()) {
        diskLetterMap[path]  = nextLetter++;
        diskPartCount[path]  = 0;  //inicia en 0, se incrementa abajo
    }

    //Incrementar número de partición para este disco
    diskPartCount[path]++;

    char        letter      = diskLetterMap[path];
    int         partNumber  = diskPartCount[path];

    //Formato: "32" + "1" + "A" = "321A"
    return carnetSuffix + std::to_string(partNumber) + letter;
}

//MOUNT ----------------------------------------------------------------------------

std::string cmdMount(std::map<std::string, std::string> params) {
    std::ostringstream out;

    // 1. PARÁMETROS OBLIGATORIOS 
    std::string path = params.count("path") ? params["path"] : "";
    std::string name = params.count("name") ? params["name"] : "";

    if (path.empty()) return "Error: Falta parámetro -path\n";
    if (name.empty()) return "Error: Falta parámetro -name\n";

    // 2. DISCO EXISTE 
    if (!archivoExiste(path))
        return "Error: El disco '" + path + "' no existe\n";

    // 3. NO ESTÁ YA MONTADA
    if (isAlreadyMounted(path, name))
        return "Error: La partición '" + name + "' de '" + path + 
               "' ya está montada\n";

    // 4. LEER MBR 
    std::fstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open())
        return "Error: No se pudo abrir '" + path + "'\n";

    MBR mbr;
    if (!readMBR(disk, mbr)) {
        disk.close();
        return "Error: No se pudo leer el MBR\n";
    }

    // 5. BUSCAR PARTICIÓN POR NOMBRE (PRIMARIA O LÓGICA)
    int foundIdx = -1;
    bool isLogical = false;
    int ebrPos = -1;
    EBR foundEbr;

    // A. Buscar en Primarias
    for (int i = 0; i < 4; i++) {
        Partition& p = mbr.Partitions[i];
        if (p.Size > 0 && p.Type == 'P') {
            if (trimNull(p.Name) == name) {
                foundIdx = i;
                break;
            }
        }
    }

    // B. Si no se encontró en primarias, buscar en la Extendida (Lógicas)
    if (foundIdx == -1) {
        int extIdx = -1;
        for (int i = 0; i < 4; i++) {
            if (mbr.Partitions[i].Type == 'E' && mbr.Partitions[i].Size > 0) {
                extIdx = i;
                break;
            }
        }

        if (extIdx != -1) {
            int currEbrPos = mbr.Partitions[extIdx].Start;
            int extEnd = mbr.Partitions[extIdx].Start + mbr.Partitions[extIdx].Size;
            
            while (currEbrPos != -1 && currEbrPos < extEnd) {
                EBR ebr;
                if (!readEBR(disk, currEbrPos, ebr)) break;
                if (!isEBREmpty(ebr) && getEBRName(ebr) == name) {
                    isLogical = true;
                    ebrPos = currEbrPos;
                    foundEbr = ebr;
                    break;
                }
                if (!hasNextEBR(ebr)) break;
                currEbrPos = ebr.Next;
            }
        }
    }

    if (foundIdx == -1 && !isLogical) {
        disk.close();
        return "Error: No se encontró partición '" + name + "' en " + path + "\n";
    }

    // 6. ACTUALIZAR ESTADO SEGÚN TIPO
    std::string newId = generateId(path);

    if (isLogical) {
        // Actualizar EBR en disco
        foundEbr.Mount = 1;
        if (!writeEBR(disk, ebrPos, foundEbr)) {
            disk.close();
            return "Error: No se pudo escribir el EBR\n";
        }
        disk.close();

        // Guardar en RAM
        MountedPartition mp;
        mp.Id    = newId;
        mp.Path  = path;
        mp.Name  = name;
        mp.Start = foundEbr.Start;
        mp.Size  = foundEbr.Size;
        mp.Type  = 'L';
        mountedPartitions.push_back(mp);
    } else {
        Partition& part = mbr.Partitions[foundIdx];
        part.Status      = '1';
        part.Correlative = diskPartCount[path];
        std::memset(part.Id, 0, sizeof(part.Id));
        std::strncpy(part.Id, newId.c_str(), sizeof(part.Id));

        if (!writeMBR(disk, mbr)) {
            disk.close();
            return "Error: No se pudo escribir el MBR\n";
        }
        disk.close();

        // Guardar en RAM
        MountedPartition mp;
        mp.Id    = newId;
        mp.Path  = path;
        mp.Name  = name;
        mp.Start = part.Start;
        mp.Size  = part.Size;
        mp.Type  = 'P';
        mountedPartitions.push_back(mp);
    }

    int finalStart = isLogical ? foundEbr.Start : mbr.Partitions[foundIdx].Start;
    int finalSize  = isLogical ? foundEbr.Size  : mbr.Partitions[foundIdx].Size;

    out << "Partición montada exitosamente\n";
    out << "  ID        : " << newId         << "\n";
    out << "  Nombre    : " << name          << "\n";
    out << "  Disco     : " << path          << "\n";
    out << "  Inicio    : " << finalStart    << " bytes\n";
    out << "  Tamaño    : " << finalSize     << " bytes\n";
    out << "  Tipo      : " << (isLogical ? "Lógica" : "Primaria") << "\n";

    return out.str();
}

//UNMOUNT --------------------------------------------------------------------------

std::string cmdUnmount(std::map<std::string, std::string> params) {
    if (params.find("id") == params.end()) {
        return "Error: Falta parámetro obligatorio -id\n";
    }

    std::string id = mayusculas(params["id"]);
    int foundIdx = -1;

    //1. Buscar en RAM
    for (size_t i = 0; i < mountedPartitions.size(); i++) {
        if (mayusculas(mountedPartitions[i].Id) == id) {
            foundIdx = i;
            break;
        }
    }

    if (foundIdx == -1) {
        return "Error: No se encontró la partición con id '" + id + "'\n";
    }

    MountedPartition mp = mountedPartitions[foundIdx];

    //2. Actualizar en DISCO
    std::fstream disk(mp.Path, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open()) {
        return "Error: No se pudo abrir el disco '" + mp.Path + "' para desmontar\n";
    }

    if (mp.Type == 'L') {
        //Buscar EBR y poner Mount = 0
        int currPos = -1;
        //Necesitamos el MBR para saber donde empieza la cadena
        MBR mbr;
        if (readMBR(disk, mbr)) {
            for (int i = 0; i < 4; i++) {
                if (mbr.Partitions[i].Type == 'E') {
                    currPos = mbr.Partitions[i].Start;
                    int extEnd = mbr.Partitions[i].Start + mbr.Partitions[i].Size;
                    while (currPos != -1 && currPos < extEnd) {
                        EBR ebr;
                        if (!readEBR(disk, currPos, ebr)) break;
                        if (!isEBREmpty(ebr) && getEBRName(ebr) == mp.Name) {
                            ebr.Mount = 0;
                            writeEBR(disk, currPos, ebr);
                            break;
                        }
                        if (!hasNextEBR(ebr)) break;
                        currPos = ebr.Next;
                    }
                    break;
                }
            }
        }
    } else {
        //Buscar en MBR y poner Status = '0'
        MBR mbr;
        if (readMBR(disk, mbr)) {
            for (int i = 0; i < 4; i++) {
                if (trimNull(mbr.Partitions[i].Name) == mp.Name) {
                    mbr.Partitions[i].Status = '0';
                    mbr.Partitions[i].Correlative = 0;
                    std::memset(mbr.Partitions[i].Id, 0, sizeof(mbr.Partitions[i].Id));
                    writeMBR(disk, mbr);
                    break;
                }
            }
        }
    }

    disk.close();

    //3. Remover de RAM
    mountedPartitions.erase(mountedPartitions.begin() + foundIdx);

    return "Partición con id '" + id + "' desmontada exitosamente\n";
}