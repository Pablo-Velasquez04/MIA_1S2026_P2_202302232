//DiskManagement/Fdisk.cpp
#include "../Utilidades/Utilities.h"
#include "../Estructuras/Structs.h"
#include "../Estructuras/Ebr.h"
#include "DiskManagement.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <cstring>
#include <climits>

//HELPERS BÁSICOS
static bool isValidUnit(const std::string &unit) {
    return unit == "B" || unit == "K" || unit == "M" || unit == "";
}
static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

static std::string trim(std::string s) {
    if (s.empty()) return s;
    s.erase(0, s.find_first_not_of(" \t\n\r"));
    if (s.empty()) return s;
    s.erase(s.find_last_not_of(" \t\n\r") + 1);
    return s;
}

static std::string trimNull(const char *p) {
    std::string s(p);
    size_t pos = s.find('\0');
    if (pos != std::string::npos) s.resize(pos);
    return s;
}

static bool readMBR(std::fstream &disk, MBR &mbr) {
    disk.seekg(0);
    disk.read(reinterpret_cast<char*>(&mbr), sizeof(mbr));
    return !disk.fail();
}

static bool writeMBR(std::fstream &disk, const MBR &mbr) {
    disk.seekp(0);
    disk.write(reinterpret_cast<const char*>(&mbr), sizeof(mbr));
    return !disk.fail();
}

static bool readEBR(std::fstream &disk, int pos, EBR &ebr) {
    disk.seekg(pos);
    disk.read(reinterpret_cast<char*>(&ebr), sizeof(ebr));
    return !disk.fail();
}

static bool writeEBR(std::fstream &disk, int pos, const EBR &ebr) {
    disk.seekp(pos);
    disk.write(reinterpret_cast<const char*>(&ebr), sizeof(ebr));
    return !disk.fail();
}

//VALIDACIONES Y CÁLCULOS GENERALES
static bool validatePartitionConstraints(const MBR &mbr,
                                         const std::string &partType,
                                         const std::string &name,
                                         std::string &err) {
    int priExt = 0;
    bool hasExt = false;
    
    for (int i = 0; i < 4; i++) {
        const Partition &p = mbr.Partitions[i];
        if (p.Size > 0) {
            std::string exName = trimNull(p.Name);
            if (exName == name) {
                err = "ya existe una partición con el nombre '" + name + "'";
                return false;
            }
            if (p.Type == 'P' || p.Type == 'E') priExt++;
            if (p.Type == 'E') hasExt = true;
        }
    }

    if (partType == "P" || partType == "E") {
        if (priExt >= 4) {
            err = "no se pueden crear más de 4 particiones primarias y extendidas";
            return false;
        }
        if (partType == "E" && hasExt) {
            err = "solo puede haber una partición extendida por disco";
            return false;
        }
    } else if (partType == "L") {
        if (!hasExt) {
            err = "no se puede crear una partición lógica sin una partición extendida";
            return false;
        }
    }

    return true;
}

static int findAvailableSpace(const MBR &mbr, int size, char /*fit*/, std::string &err) {
    int used = 0;
    for (int i = 0; i < 4; i++)
        if (mbr.Partitions[i].Size > 0)
            used += mbr.Partitions[i].Size;

    int available = mbr.MbrSize - used - static_cast<int>(sizeof(MBR));
    if (available < size) {
        std::ostringstream os;
        os << "no hay suficiente espacio disponible. Requerido: " << size
           << " bytes, Disponible: " << available << " bytes";
        err = os.str();
        return -1;
    }
    return static_cast<int>(sizeof(MBR)) + used;
}

//ESTRUCTURAS AUXILIARES PARA PARTICIONES LÓGICAS
struct LogicalSpace {
    int EBRPos;
    int DataStart;
    int DataEnd;
    std::string Name;
};

static std::pair<int,int> findFirstFitLogical(int extStart, int extEnd,
        const std::vector<LogicalSpace> &existing,
        int requiredSpace, int ebrSize, std::string &err) {
    int current = extStart;
    auto list = existing;
    std::sort(list.begin(), list.end(),
              [](const LogicalSpace &a, const LogicalSpace &b){ 
                  return a.EBRPos < b.EBRPos; 
              });
    
    for (auto &l : list) {
        int gap = l.EBRPos - current;
        if (gap >= requiredSpace) {
            return { current, current + ebrSize };
        }
        if (l.DataEnd > current) current = l.DataEnd;
    }
    
    if (extEnd - current >= requiredSpace)
        return { current, current + ebrSize };

    err = "no hay espacio suficiente en la partición extendida para "
          + std::to_string(requiredSpace) + " bytes";
    return { -1, -1 };
}

static std::pair<int,int> findBestFitLogical(int extStart, int extEnd,
        const std::vector<LogicalSpace> &existing,
        int requiredSpace, int ebrSize, std::string &err) {
    int bestPos = -1, bestData = -1;
    int bestSize = INT_MAX;
    int current = extStart;
    auto list = existing;
    std::sort(list.begin(), list.end(),
              [](const LogicalSpace &a, const LogicalSpace &b){ 
                  return a.EBRPos < b.EBRPos; 
              });
    
    for (auto &l : list) {
        int gap = l.EBRPos - current;
        if (gap >= requiredSpace && gap < bestSize) {
            bestSize = gap;
            bestPos = current;
            bestData = current + ebrSize;
        }
        if (l.DataEnd > current) current = l.DataEnd;
    }
    
    int gap = extEnd - current;
    if (gap >= requiredSpace && gap < bestSize) {
        bestPos = current;
        bestData = current + ebrSize;
    }
    
    if (bestPos == -1) {
        err = "no hay espacio suficiente en la partición extendida para "
              + std::to_string(requiredSpace) + " bytes";
    }
    return { bestPos, bestData };
}

static std::pair<int,int> findWorstFitLogical(int extStart, int extEnd,
        const std::vector<LogicalSpace> &existing,
        int requiredSpace, int ebrSize, std::string &err) {
    int worstPos = -1, worstData = -1;
    int worstSize = 0;
    int current = extStart;
    auto list = existing;
    std::sort(list.begin(), list.end(),
              [](const LogicalSpace &a, const LogicalSpace &b){ 
                  return a.EBRPos < b.EBRPos; 
              });
    
    for (auto &l : list) {
        int gap = l.EBRPos - current;
        if (gap >= requiredSpace && gap > worstSize) {
            worstSize = gap;
            worstPos = current;
            worstData = current + ebrSize;
        }
        if (l.DataEnd > current) current = l.DataEnd;
    }
    
    int gap = extEnd - current;
    if (gap >= requiredSpace && gap > worstSize) {
        worstPos = current;
        worstData = current + ebrSize;
    }
    
    if (worstPos == -1) {
        err = "no hay espacio suficiente en la partición extendida para "
              + std::to_string(requiredSpace) + " bytes";
    }
    return { worstPos, worstData };
}

static std::pair<int,int> findLogicalPartitionSpace(std::fstream &disk,
        const Partition &extPart,
        const std::vector<LogicalSpace> &existing,
        int requiredSpace, int ebrSize, char fit, std::string &err) {

    int extStart = extPart.Start;
    int extEnd   = extStart + extPart.Size;
    
    switch (fit) {
        case 'F': 
            return findFirstFitLogical(extStart, extEnd, existing,
                                      requiredSpace, ebrSize, err);
        case 'B': 
            return findBestFitLogical(extStart, extEnd, existing,
                                     requiredSpace, ebrSize, err);
        case 'W': 
            return findWorstFitLogical(extStart, extEnd, existing,
                                      requiredSpace, ebrSize, err);
        default:  
            return findFirstFitLogical(extStart, extEnd, existing,
                                      requiredSpace, ebrSize, err);
    }
}

static bool insertEBRInChain(std::fstream &disk, const Partition &extPart,
                             const EBR &newEBR, int newPos) {
    int extStart = extPart.Start;
    int extEnd   = extStart + extPart.Size;
    EBR first;
    
    if (!readEBR(disk, extStart, first) || isEBREmpty(first))
        return true;

    int currPos = extStart;
    EBR curr, prev;
    int prevPos = -1;
    
    while (currPos != -1 && currPos < extEnd) {
        if (!readEBR(disk, currPos, curr)) break;
        if (newPos < currPos) {
            EBR copy = newEBR;
            copy.Next = currPos;
            if (prevPos != -1) {
                prev.Next = newPos;
                writeEBR(disk, prevPos, prev);
            }
            writeEBR(disk, newPos, copy);
            return true;
        }
        prev    = curr;
        prevPos = currPos;
        if (!hasNextEBR(curr)) break;
        currPos = curr.Next;
    }
    
    if (prevPos != -1) {
        prev.Next = newPos;
        writeEBR(disk, prevPos, prev);
    }
    return true;
}

//CREAR PARTICIONES
static void createPrimaryPartition(std::fstream &disk, MBR &mbr,
                                   const std::string &name, int start,
                                   int size, char fit, std::ostringstream &out) {
    for (int i = 0; i < 4; i++) {
        if (mbr.Partitions[i].Size <= 0) {
            mbr.Partitions[i].Status = '0';
            mbr.Partitions[i].Type   = 'P';
            mbr.Partitions[i].Fit    = fit;
            mbr.Partitions[i].Start  = start;
            mbr.Partitions[i].Size   = size;
            std::strncpy(mbr.Partitions[i].Name, name.c_str(),
                         sizeof(mbr.Partitions[i].Name)-1);
            mbr.Partitions[i].Correlative = -1;
            std::memset(mbr.Partitions[i].Id, 0, sizeof(mbr.Partitions[i].Id));

            if (!writeMBR(disk, mbr))
                out << "Error escribiendo MBR\n";
            else
                out << "=== PARTICIÓN PRIMARIA CREADA ===\n";
            return;
        }
    }
    out << "Error: No hay slots disponibles para particiones\n";
}

static void createExtendedPartition(std::fstream &disk, MBR &mbr,
                                    const std::string &name, int start,
                                    int size, char fit, std::ostringstream &out) {
    for (int i = 0; i < 4; i++) {
        if (mbr.Partitions[i].Size <= 0) {
            mbr.Partitions[i].Status = '0';
            mbr.Partitions[i].Type   = 'E';
            mbr.Partitions[i].Fit    = fit;
            mbr.Partitions[i].Start  = start;
            mbr.Partitions[i].Size   = size;
            std::strncpy(mbr.Partitions[i].Name, name.c_str(),
                         sizeof(mbr.Partitions[i].Name)-1);
            mbr.Partitions[i].Correlative = -1;
            std::memset(mbr.Partitions[i].Id, 0, sizeof(mbr.Partitions[i].Id));

            if (!writeMBR(disk, mbr))
                out << "Error escribiendo MBR\n";
            else
                out << "=== PARTICIÓN EXTENDIDA CREADA ===\n";
            return;
        }
    }
    out << "Error: No hay slots disponibles para particiones\n";
}

static void createLogicalPartition(std::fstream &disk, MBR &mbr,
                                   const std::string &name, int /*start*/,
                                   int size, char fit, std::ostringstream &out) {
    Partition *ext = nullptr;
    for (int i = 0; i < 4; i++)
        if (mbr.Partitions[i].Type == 'E' && mbr.Partitions[i].Size > 0) {
            ext = &mbr.Partitions[i];
            break;
        }
    if (!ext) {
        out << "Error: No se encontró partición extendida\n";
        return;
    }

    int ebrSize = sizeof(EBR);
    int required = size + ebrSize;
    std::vector<LogicalSpace> existing;
    int currPos = ext->Start;
    int extEnd  = ext->Start + ext->Size;
    
    while (currPos != -1 && currPos < extEnd) {
        EBR ebr;
        if (!readEBR(disk, currPos, ebr)) break;
        if (!isEBREmpty(ebr)) {
            std::string nm = getEBRName(ebr);
            if (nm == name) {
                out << "Error: ya existe una partición lógica con el nombre '"
                    << name << "'\n";
                return;
            }
            existing.push_back({currPos, ebr.Start, ebr.Start + ebr.Size, nm});
        }
        if (!hasNextEBR(ebr)) break;
        currPos = ebr.Next;
    }

    std::string err;
    auto pair = findLogicalPartitionSpace(disk, *ext, existing,
                                          required, ebrSize, fit, err);
    if (pair.first < 0) {
        out << err << "\n";
        return;
    }
    int ebrPos   = pair.first;
    int dataStart= pair.second;
    EBR newEBR = crearEBRConDatos(fit, dataStart, size, name, -1);
    if (!insertEBRInChain(disk, *ext, newEBR, ebrPos)) {
        out << "Error insertando EBR en la cadena\n";
        return;
    }
    if (!writeEBR(disk, ebrPos, newEBR)) {
        out << "Error escribiendo EBR\n";
        return;
    }
    out << "=== PARTICIÓN LÓGICA CREADA ===\n";
}

//ELIMINAR PARTICIONES
static bool deleteLogicalPartition(std::fstream &disk, const MBR &mbr,
                                   const std::string &name,
                                   const std::string &delType,
                                   std::ostringstream &out) {
    const Partition *ext = nullptr;
    for (int i = 0; i < 4; i++)
        if (mbr.Partitions[i].Type == 'E' && mbr.Partitions[i].Size > 0) {
            ext = &mbr.Partitions[i];
            break;
        }
    if (!ext) return false;

    int extStart = ext->Start;
    int extEnd   = extStart + ext->Size;
    int currPos  = extStart;
    int prevPos  = -1;
    EBR prevEbr;

    while (currPos != -1 && currPos < extEnd) {
        EBR ebr;
        if (!readEBR(disk, currPos, ebr)) break;
        if (!isEBREmpty(ebr) && getEBRName(ebr) == name) {
            out << "Eliminando partición lógica '" << name << "'...\n";
            if (delType == "FULL") {
                long tot = ebr.Size;
                disk.seekp(ebr.Start);
                std::vector<char> z(4096, 0);
                long rem = tot;
                while (rem > 0) {
                    long w = std::min<long>(rem, (long)z.size());
                    disk.write(z.data(), w);
                    rem -= w;
                }
            }
            if (prevPos != -1) {
                prevEbr.Next = ebr.Next;
                writeEBR(disk, prevPos, prevEbr);
            } else {
                if (ebr.Next != -1 && ebr.Next < extEnd) {
                    EBR next;
                    if (readEBR(disk, ebr.Next, next))
                        writeEBR(disk, extStart, next);
                }
            }
            EBR empty = crearEBR();
            writeEBR(disk, currPos, empty);
            out << "Partición lógica '" << name << "' eliminada exitosamente ("
                << delType << ")\n";
            return true;
        }
        prevPos = currPos;
        prevEbr = ebr;
        if (!hasNextEBR(ebr)) break;
        currPos = ebr.Next;
    }
    return false;
}

static void deletePartition(std::fstream &disk, MBR &mbr,
                            const std::string &name,
                            const std::string &delType,
                            std::ostringstream &out) {
    bool found = false;
    int idx = -1;
    char pt   = 0;
    
    for (int i = 0; i < 4; i++) {
        if (mbr.Partitions[i].Size > 0) {
            std::string nm = trimNull(mbr.Partitions[i].Name);
            if (nm == name) {
                found = true;
                idx = i;
                pt  = mbr.Partitions[i].Type;
                break;
            }
        }
    }
    
    if (!found) {
        if (deleteLogicalPartition(disk, mbr, name, delType, out)) return;
        out << "Error: partición '" << name << "' no encontrada\n";
        return;
    }

    if (delType == "FAST") {
        mbr.Partitions[idx] = {};
        writeMBR(disk, mbr);
        out << "Partición '" << name << "' (tipo: " << pt
            << ") eliminada exitosamente (FAST)\n";
    } else if (delType == "FULL") {
        Partition part = mbr.Partitions[idx];
        long tot = part.Size;
        disk.seekp(part.Start);
        std::vector<char> z(4096, 0);
        long rem = tot;
        while (rem > 0) {
            long w = std::min<long>(rem, (long)z.size());
            disk.write(z.data(), w);
            rem -= w;
        }
        mbr.Partitions[idx] = {};
        writeMBR(disk, mbr);
        out << "Partición '" << name << "' (tipo: " << pt
            << ") eliminada físicamente y removida de la tabla (FULL)\n";
    } else {
        out << "Error: Tipo de eliminación '" << delType << "' no válido\n";
    }
}

// MODIFICAR PARTICIONES
static void expandPartition(std::fstream &disk, MBR &mbr,
                            int index, int addSize, std::ostringstream &out) {
    Partition &p = mbr.Partitions[index];
    int end = p.Start + p.Size;
    int diskSize = mbr.MbrSize;
    int nextStart = diskSize;
    
    for (int i = 0; i < 4; i++) {
        const Partition &q = mbr.Partitions[i];
        if (i != index && q.Size > 0 && q.Start >= end && q.Start < nextStart)
            nextStart = q.Start;
    }
    
    int avail = nextStart - end;
    if (avail <= 0 || avail < addSize) {
        if (avail < 0) avail = 0;
        out << "Error: no hay espacio libre contiguo para expandir la partición. "
            << "Requerido: " << addSize << " bytes, Disponible: " << avail << " bytes\n";
        return;
    }
    
    int old = p.Size;
    p.Size += addSize;
    writeMBR(disk, mbr);
    out << "Partición '" << trimNull(p.Name) << "' expandida exitosamente\n"
        << "  Tamaño anterior: " << old << " bytes\n"
        << "  Tamaño nuevo: " << p.Size << " bytes\n"
        << "  Espacio agregado: " << addSize << " bytes\n";
}

static void shrinkPartition(std::fstream &disk, MBR &mbr,
                            int index, int reduceSize, std::ostringstream &out) {
    Partition &p = mbr.Partitions[index];
    if (p.Size <= reduceSize) {
        out << "no se puede reducir la partición a un tamaño menor o igual a cero\n";
        return;
    }
    
    out << "ADVERTENCIA: Reducir la partición '" << trimNull(p.Name)
        << "' puede causar pérdida de datos\n";
    
    int old = p.Size;
    int newSize = p.Size - reduceSize;
    long cleanStart = p.Start + newSize;
    std::vector<char> z(4096, 0);
    int blocks = (reduceSize + 4096 - 1) / 4096;
    
    for (int i = 0; i < blocks; i++) {
        int w = (i == blocks-1) ? (reduceSize - i*4096) : 4096;
        disk.seekp(cleanStart + i*4096);
        disk.write(z.data(), w);
    }
    
    p.Size = newSize;
    writeMBR(disk, mbr);
    out << "Partición '" << trimNull(p.Name) << "' reducida exitosamente\n"
        << "  Tamaño anterior: " << old << " bytes\n"
        << "  Tamaño nuevo: " << p.Size << " bytes\n"
        << "  Espacio liberado: " << reduceSize << " bytes\n";
}

static bool expandLogicalPartition(std::fstream &disk, EBR &ebr, int ebrPos,
                                   int addSize, int extEnd, std::ostringstream &out) {
    int partEnd = ebr.Start + ebr.Size;
    int avail = extEnd - partEnd;
    
    if (hasNextEBR(ebr)) {
        avail = ebr.Next - partEnd;
    }
    
    if (avail <= 0 || avail < addSize) {
        if (avail < 0) avail = 0;
        out << "Error: no hay espacio libre contiguo para expandir la partición lógica. "
            << "Requerido: " << addSize << " bytes, Disponible: " << avail << " bytes\n";
        return false;
    }
    
    int old = ebr.Size;
    ebr.Size += addSize;
    writeEBR(disk, ebrPos, ebr);
    out << "Partición lógica '" << getEBRName(ebr) << "' expandida exitosamente\n"
        << "  Tamaño anterior: " << old << " bytes\n"
        << "  Tamaño nuevo: " << ebr.Size << " bytes\n"
        << "  Espacio agregado: " << addSize << " bytes\n";
    return true;
}

static bool shrinkLogicalPartition(std::fstream &disk, EBR &ebr, int ebrPos,
                                   int reduceSize, std::ostringstream &out) {
    if (ebr.Size <= reduceSize) {
        out << "no se puede reducir la partición a un tamaño menor o igual a cero\n";
        return false;
    }
    
    int old = ebr.Size;
    ebr.Size -= reduceSize;
    long cleanStart = ebr.Start + ebr.Size;
    std::vector<char> z(4096, 0);
    int blocks = (reduceSize + 4096 - 1) / 4096;
    
    for (int i = 0; i < blocks; i++) {
        int w = (i == blocks-1) ? (reduceSize - i*4096) : 4096;
        disk.seekp(cleanStart + i*4096);
        disk.write(z.data(), w);
    }
    
    writeEBR(disk, ebrPos, ebr);
    out << "Partición lógica '" << getEBRName(ebr) << "' reducida exitosamente\n"
        << "  Tamaño anterior: " << old << " bytes\n"
        << "  Tamaño nuevo: " << ebr.Size << " bytes\n"
        << "  Espacio liberado: " << reduceSize << " bytes\n";
    return true;
}

static bool modifyLogicalPartition(std::fstream &disk, MBR &mbr,
                                   const std::string &name, int add,
                                   std::ostringstream &out) {
    Partition *ext = nullptr;
    for (int i = 0; i < 4; i++)
        if (mbr.Partitions[i].Type == 'E' && mbr.Partitions[i].Size > 0) {
            ext = &mbr.Partitions[i];
            break;
        }
    if (!ext) return false;

    int extStart = ext->Start;
    int extEnd   = extStart + ext->Size;
    int currPos  = extStart;
    
    while (currPos != -1 && currPos < extEnd) {
        EBR ebr;
        if (!readEBR(disk, currPos, ebr)) break;
        if (!isEBREmpty(ebr) && getEBRName(ebr) == name) {
            if (add > 0) return expandLogicalPartition(disk, ebr, currPos, add, extEnd, out);
            else if (add < 0) return shrinkLogicalPartition(disk, ebr, currPos, -add, out);
        }
        if (!hasNextEBR(ebr)) break;
        currPos = ebr.Next;
    }
    return false;
}

static void modifyPartition(std::fstream &disk, MBR &mbr,
                            const std::string &name, int add,
                            std::ostringstream &out) {
    bool found = false;
    int idx = -1;
    
    for (int i = 0; i < 4; i++) {
        if (mbr.Partitions[i].Size > 0) {
            if (trimNull(mbr.Partitions[i].Name) == name) {
                found = true;
                idx = i;
                break;
            }
        }
    }
    
    if (!found) {
        if (modifyLogicalPartition(disk, mbr, name, add, out)) return;
        out << "Error: partición '" << name << "' no encontrada\n";
        return;
    }
    
    if (add > 0) expandPartition(disk, mbr, idx, add, out);
    else if (add < 0) shrinkPartition(disk, mbr, idx, -add, out);
    else out << "Error: El valor de 'add' no puede ser cero\n";
}

// FUNCIÓN PRINCIPAL
std::string cmdFdisk(std::map<std::string,std::string> params) {
    std::ostringstream out;

    std::string sizeStr   = params.count("size")   ? params["size"]   : "";
    std::string path      = params.count("path")   ? params["path"]   : "";
    std::string name      = params.count("name")   ? params["name"]   : "";
    std::string unit      = toUpper(params.count("unit") ? params["unit"] : "");
    std::string partType  = toUpper(params.count("type") ? params["type"] : "");
    std::string fit       = toUpper(params.count("fit")  ? params["fit"]  : "");
    std::string delType   = toUpper(params.count("delete") ? params["delete"] : "");
    std::string addStr    = params.count("add")    ? params["add"]    : "";

    // Validar parámetros no reconocidos
    for (const auto& pair : params) {
        std::string p = pair.first;
        if (p != "size" && p != "path" && p != "name" && p != "unit" && 
            p != "type" && p != "fit" && p != "delete" && p != "add") {
            return "Error: El parámetro '" + p + "' no es reconocido para fdisk\n";
        }
    }

    // Validaciones básicas
    if (path.empty())  return "Error: El parámetro 'path' es obligatorio\n";
    if (name.empty())  return "Error: El parámetro 'name' es obligatorio\n";

    bool isDelete = !delType.empty();
    bool isAdd    = !addStr.empty();
    bool isCreate = !isDelete && !isAdd;

    if (isCreate && sizeStr.empty())
        return "Error: El parámetro 'size' es obligatorio para crear particiones\n";

    // Validar y convertir tamaño
    int size = 0;
    if (!sizeStr.empty()) {
        try { size = std::stoi(sizeStr); }
        catch (...) { return "Error: El tamaño debe ser un número entero\n"; }
        if (size <= 0) return "Error: El tamaño debe ser mayor que cero\n";
    }

    long sizeInBytes = 0;
    if (sizeStr != "") {
        std::string u = trim(unit);
        if (u == "B") sizeInBytes = size;
        else if (u == "K" || u == "") sizeInBytes = size * 1024L;
        else if (u == "M") sizeInBytes = size * 1024L * 1024L;
        else return "Error: Unidad '" + u + "' no válida. Use B, K o M\n";
    }

    // Validar tipo de partición
    if (partType == "") partType = "P";
    if (partType != "P" && partType != "E" && partType != "L")
               "' no válido. Use P, E o L\n";

    if (fit == "BF")       fit = "BF";
    else if (fit == "FF")  fit = "FF";
    else if (fit == "WF" || fit == "") fit = "WF";
    else return "Error: Ajuste '" + fit + "' no válido. Use BF, FF o WF\n";

    char fitByte = 'W';
    if (fit == "BF") fitByte = 'B';
    else if (fit == "FF") fitByte = 'F';
    else if (fit == "WF") fitByte = 'W';

    // Validar delete
    if (isDelete && delType != "FAST" && delType != "FULL")
        return "Error: Tipo de eliminación '" + delType + "' no válido. Use FAST o FULL\n";

    // Validar add
    long addInBytes = 0;
    if (isAdd) {
        int addVal = 0;
        std::string addClean = trim(addStr);

        try { addVal = std::stoi(addClean); }
        catch (...) { return "Error: El valor de 'add' debe ser un número entero\n"; }
        
        std::string unitClean = trim(unit);

        if (unitClean == "B") addInBytes = addVal;
        else if (unitClean == "K" || unitClean == "") addInBytes = addVal * 1024L;
        else if (unitClean == "M") addInBytes = addVal * 1024L * 1024L;
        else return "Error: Unidad '" + unitClean + "' no válida. Use B, K o M\n";
    }

    // Asegurar extensión .mia usando utilidad robusta
    path = cambiarExtension(path, ".mia");

    // Validar que el disco existe
    if (!archivoExiste(path))
        return "Error: El disco '" + path + "' no existe\n";

    // Abrir el disco
    std::fstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open())
        return "Error abriendo el disco\n";

    // Leer MBR
    MBR mbr;
    if (!readMBR(disk, mbr)) {
        disk.close();
        return "Error leyendo el MBR\n";
    }

    // Procesar operación
    if (isDelete)
        deletePartition(disk, mbr, name, delType, out);
    else if (isAdd)
        modifyPartition(disk, mbr, name, (int)addInBytes, out);
    else {
        // Crear partición
        std::string err;
        if (!validatePartitionConstraints(mbr, partType, name, err)) {
            disk.close();
            return "Error: " + err + "\n";
        }
        
        int start = findAvailableSpace(mbr, sizeInBytes, fitByte, err);
        if (start < 0) {
            disk.close();
            return "Error: " + err + "\n";
        }
        
        if (partType == "P")
            createPrimaryPartition(disk, mbr, name, start, sizeInBytes, fitByte, out);
        else if (partType == "E")
            createExtendedPartition(disk, mbr, name, start, sizeInBytes, fitByte, out);
        else
            createLogicalPartition(disk, mbr, name, start, sizeInBytes, fitByte, out);
    }

    disk.close();
    return out.str();
}