#include "Reportes.h"
#include "RepHelpers.h"

#include "../Estructuras/Structs.h"
#include "../Estructuras/Ebr.h"
#include "../Utilidades/Utilities.h"
#include "../DiskManagement/DiskManagement.h"
#include "../Mounting/Mounting.h"
#include "../FilesFolders/FilesFolders.h"  
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <set>

namespace fs = std::filesystem;

//REPORTE MBR
//Muestra tabla con info del MBR y todas sus particiones.
static std::string repMBR(const std::string& diskPath,
                           const std::string& outPath) {
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open())
        return "Error: No se pudo abrir el disco " + diskPath;

    MBR mbr;
    if (!repReadMBR(disk, mbr)) {
        disk.close();
        return "Error: No se pudo leer el MBR";
    }

    std::ostringstream dot;
    dot << "digraph MBR {\n";
    dot << "  graph [bgcolor=\"#1e1e2e\" fontname=\"Helvetica\"];\n";
    dot << "  node  [shape=plain fontname=\"Helvetica\"];\n\n";

    dot << "  mbr [label=<\n";
    dot << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" "
           "CELLPADDING=\"6\" BGCOLOR=\"#1e1e2e\">\n";

    //Encabezado
    dot << "      <TR><TD COLSPAN=\"2\" BGCOLOR=\"#6c3483\" ALIGN=\"CENTER\">"
           "<FONT COLOR=\"white\"><B>REPORTE DE MBR</B></FONT></TD></TR>\n";

    //Campos MBR
    dot << "      <TR><TD BGCOLOR=\"#d2b4de\" ALIGN=\"RIGHT\"><B>mbr_tamano</B></TD>"
        << "<TD BGCOLOR=\"#f5eef8\" ALIGN=\"LEFT\">" << mbr.MbrSize << "</TD></TR>\n";
    dot << "      <TR><TD BGCOLOR=\"#d2b4de\" ALIGN=\"RIGHT\"><B>mbr_fecha_creacion</B></TD>"
        << "<TD BGCOLOR=\"#f5eef8\" ALIGN=\"LEFT\">" 
        << formatearTiempo(mbr.CreationDate) << "</TD></TR>\n";
    dot << "      <TR><TD BGCOLOR=\"#d2b4de\" ALIGN=\"RIGHT\"><B>mbr_disk_signature</B></TD>"
        << "<TD BGCOLOR=\"#f5eef8\" ALIGN=\"LEFT\">" << mbr.Signature << "</TD></TR>\n";

    //Ordenar particiones por Start
    struct PartEntry {
        int       idx;
        int       start;
        Partition p;
    };
    std::vector<PartEntry> sortedParts;
    for (int i = 0; i < 4; i++) {
        if (mbr.Partitions[i].Size > 0)
            sortedParts.push_back({i, mbr.Partitions[i].Start, mbr.Partitions[i]});
    }
    std::sort(sortedParts.begin(), sortedParts.end(),
            [](const PartEntry& a, const PartEntry& b){
                return a.start < b.start;
            });

    //Particiones en orden físico
    bool hasExtended = false;
    int  extStart    = -1;
    int  extSize     = -1;

    for (const auto& entry : sortedParts) {
        const Partition& p = entry.p;

        std::string tipo      = (p.Type == 'P') ? "Particion" : "Particion Extendida";
        std::string hdrColor  = (p.Type == 'E') ? "#1a5276"   : "#4a235a";
        std::string rowColor  = (p.Type == 'E') ? "#aed6f1"   : "#d2b4de";
        std::string cellColor = (p.Type == 'E') ? "#d6eaf8"   : "#f5eef8";

        dot << "      <TR><TD COLSPAN=\"2\" BGCOLOR=\"" << hdrColor
            << "\" ALIGN=\"CENTER\"><FONT COLOR=\"white\"><B>"
            << tipo << "</B></FONT></TD></TR>\n";

        dot << "      <TR><TD BGCOLOR=\"" << rowColor
            << "\" ALIGN=\"RIGHT\"><B>part_status</B></TD>"
            << "<TD BGCOLOR=\"" << cellColor << "\" ALIGN=\"LEFT\">"
            << p.Status << "</TD></TR>\n";

        dot << "      <TR><TD BGCOLOR=\"" << rowColor
            << "\" ALIGN=\"RIGHT\"><B>part_type</B></TD>"
            << "<TD BGCOLOR=\"" << cellColor << "\" ALIGN=\"LEFT\">"
            << (char)tolower(p.Type) << "</TD></TR>\n";

        dot << "      <TR><TD BGCOLOR=\"" << rowColor
            << "\" ALIGN=\"RIGHT\"><B>part_fit</B></TD>"
            << "<TD BGCOLOR=\"" << cellColor << "\" ALIGN=\"LEFT\">"
            << (char)tolower(p.Fit) << "</TD></TR>\n";

        dot << "      <TR><TD BGCOLOR=\"" << rowColor
            << "\" ALIGN=\"RIGHT\"><B>part_start</B></TD>"
            << "<TD BGCOLOR=\"" << cellColor << "\" ALIGN=\"LEFT\">"
            << p.Start << "</TD></TR>\n";

        dot << "      <TR><TD BGCOLOR=\"" << rowColor
            << "\" ALIGN=\"RIGHT\"><B>part_size</B></TD>"
            << "<TD BGCOLOR=\"" << cellColor << "\" ALIGN=\"LEFT\">"
            << p.Size << "</TD></TR>\n";

        dot << "      <TR><TD BGCOLOR=\"" << rowColor
            << "\" ALIGN=\"RIGHT\"><B>part_name</B></TD>"
            << "<TD BGCOLOR=\"" << cellColor << "\" ALIGN=\"LEFT\">"
            << trimNull(p.Name) << "</TD></TR>\n";

        if (p.Type == 'E') {
            hasExtended = true;
            extStart    = p.Start;
            extSize     = p.Size;
        }
    }

    dot << "    </TABLE>>];\n\n";

    //EBRs encadenados
    if (hasExtended && extStart >= 0) {
        int extEnd  = extStart + extSize;
        int currPos = extStart;
        int ebrIdx  = 0;

        while (currPos != -1 && currPos < extEnd) {
            EBR ebr;
            if (!repReadEBR(disk, currPos, ebr)) break;
            if (isEBREmpty(ebr)) break;

            std::string nodeName = "ebr" + std::to_string(ebrIdx);
            //Color según si está montada o no
            std::string ebrHdr = (ebr.Mount == 1) ? "#922b21" : "#c0392b";

            dot << "  " << nodeName << " [label=<\n";
            dot << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" "
                   "CELLPADDING=\"6\" BGCOLOR=\"#1e1e2e\">\n";

            dot << "      <TR><TD COLSPAN=\"2\" BGCOLOR=\"" << ebrHdr
                << "\" ALIGN=\"CENTER\"><FONT COLOR=\"white\"><B>"
                << "Particion Logica</B></FONT></TD></TR>\n";

            //part_status viene de ebr.Mount
            dot << "      <TR><TD BGCOLOR=\"#f1948a\" ALIGN=\"RIGHT\">"
                   "<B>part_status</B></TD>"
                << "<TD BGCOLOR=\"#fadbd8\" ALIGN=\"LEFT\">"
                << (int)ebr.Mount << "</TD></TR>\n";

            dot << "      <TR><TD BGCOLOR=\"#f1948a\" ALIGN=\"RIGHT\">"
                   "<B>part_next</B></TD>"
                << "<TD BGCOLOR=\"#fadbd8\" ALIGN=\"LEFT\">"
                << ebr.Next << "</TD></TR>\n";

            dot << "      <TR><TD BGCOLOR=\"#f1948a\" ALIGN=\"RIGHT\">"
                   "<B>part_fit</B></TD>"
                << "<TD BGCOLOR=\"#fadbd8\" ALIGN=\"LEFT\">"
                << (char)tolower(ebr.Fit) << "</TD></TR>\n";

            dot << "      <TR><TD BGCOLOR=\"#f1948a\" ALIGN=\"RIGHT\">"
                   "<B>part_start</B></TD>"
                << "<TD BGCOLOR=\"#fadbd8\" ALIGN=\"LEFT\">"
                << ebr.Start << "</TD></TR>\n";

            dot << "      <TR><TD BGCOLOR=\"#f1948a\" ALIGN=\"RIGHT\">"
                   "<B>part_size</B></TD>"
                << "<TD BGCOLOR=\"#fadbd8\" ALIGN=\"LEFT\">"
                << ebr.Size << "</TD></TR>\n";

            dot << "      <TR><TD BGCOLOR=\"#f1948a\" ALIGN=\"RIGHT\">"
                   "<B>part_name</B></TD>"
                << "<TD BGCOLOR=\"#fadbd8\" ALIGN=\"LEFT\">"
                << getEBRName(ebr) << "</TD></TR>\n";

            dot << "    </TABLE>>];\n\n";

            if (ebrIdx == 0)
                dot << "  mbr -> " << nodeName
                    << " [color=\"#e74c3c\" label=\"EBR\"];\n";
            else
                dot << "  ebr" << (ebrIdx - 1) << " -> " << nodeName
                    << " [color=\"#e74c3c\" label=\"next\"];\n";

            ebrIdx++;
            if (!hasNextEBR(ebr)) break;
            currPos = ebr.Next;
        }
    }

    dot << "}\n";
    disk.close();

    prepareOutputPath(outPath);
    std::string err = generateImage(dot.str(), outPath, "jpg");
    if (!err.empty()) return err;

    return "Reporte MBR generado: " + outPath;
}

//REPORTE DISK

static std::string repDisk(const std::string& diskPath,
                            const std::string& outPath) {
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open())
        return "Error: No se pudo abrir el disco " + diskPath;

    MBR mbr;
    if (!repReadMBR(disk, mbr)) {
        disk.close();
        return "Error: No se pudo leer el MBR";
    }

    //Recolectar segmentos ordenados por Start 
    struct Segment {
        int         start;
        int         size;
        std::string label;
        std::string color;
        bool        isExtended;
        //EBRs hijos si isExtended
        struct LogSeg {
            int         start;
            int         size;
            std::string label;
            bool        isFree;
        };
        std::vector<LogSeg> children;
    };

    std::vector<Segment> segments;

    //MBR siempre primero
    segments.push_back({0, (int)sizeof(MBR), "MBR", "#6c3483", false, {}});

    for (int i = 0; i < 4; i++) {
        const Partition& p = mbr.Partitions[i];
        if (p.Size <= 0) continue;

        Segment seg;
        seg.start      = p.Start;
        seg.size       = p.Size;
        seg.isExtended = (p.Type == 'E');

        if (p.Type == 'P') {
            seg.label = "Primaria<BR/>" + trimNull(p.Name);
            seg.color = "#1a5276";
        } else if (p.Type == 'E') {
            seg.label = "Extendida\n" + trimNull(p.Name);
            seg.color = "#117a65";

            //Leer hijos (EBRs)
            int extEnd  = p.Start + p.Size;
            int currPos = p.Start;
            int prevEnd = p.Start;  //rastrea hasta dónde llegamos

            while (currPos != -1 && currPos < extEnd) {
                EBR ebr;
                if (!repReadEBR(disk, currPos, ebr)) break;

                //EBR mismo
                seg.children.push_back({
                    currPos, (int)sizeof(EBR),
                    "EBR", false
                });

                if (!isEBREmpty(ebr)) {
                    int dataStart = ebr.Start;
                    int dataEnd   = ebr.Start + ebr.Size;

                    //Lógica
                    seg.children.push_back({
                        dataStart, ebr.Size,
                        "Logica\n" + getEBRName(ebr), false
                    });

                    prevEnd = dataEnd;  //actualizar SIEMPRE aquí
                }

                if (!hasNextEBR(ebr)) break;
                currPos = ebr.Next;
            }

            //UN SOLO Libre al final — cubre todo el espacio no usado
            if (prevEnd < extEnd) {
                seg.children.push_back({
                    prevEnd, extEnd - prevEnd,
                    "Libre", true
                });
            }

        } else {
            seg.label = trimNull(p.Name);
            seg.color = "#1a5276";
        }

        segments.push_back(seg);
    }

    //Ordenar por start
    std::sort(segments.begin(), segments.end(),
              [](const Segment& a, const Segment& b){
                  return a.start < b.start;
              });

    //Detectar espacios libres entre segmentos
    std::vector<Segment> final_segs;
    int cursor = 0;
    for (auto& seg : segments) {
        if (seg.start > cursor) {
            final_segs.push_back({
                cursor, seg.start - cursor,
                "Libre", "#5d6d7e", false, {}
            });
        }
        final_segs.push_back(seg);
        cursor = seg.start + seg.size;
    }
    //Espacio libre al final del disco
    if (cursor < mbr.MbrSize) {
        final_segs.push_back({
            cursor, mbr.MbrSize - cursor,
            "Libre", "#5d6d7e", false, {}
        });
    }

    //Construir DOT 
    //Nombre del disco (solo el archivo)
    std::string diskName = fs::path(diskPath).filename().string();

    std::ostringstream dot;
    dot << "digraph DISK {\n";
    dot << "  graph [bgcolor=\"#1e1e2e\" fontname=\"Helvetica\" "
           "label=\"" << diskName << "\" labelloc=\"t\" "
           "fontcolor=\"white\" fontsize=\"18\"];\n";
    dot << "  node  [shape=plain fontname=\"Helvetica\"];\n\n";

    dot << "  disk [label=<\n";
    dot << "    <TABLE BORDER=\"1\" CELLBORDER=\"0\" CELLSPACING=\"0\" "
           "CELLPADDING=\"0\" BGCOLOR=\"#1e1e2e\" COLOR=\"#48c9b0\">\n";
    dot << "      <TR>\n";

    for (const auto& seg : final_segs) {
        double pct = (double)seg.size / mbr.MbrSize * 100.0;
        std::ostringstream pctStr;
        pctStr << std::fixed << std::setprecision(1) << pct << "%";

        if (seg.isExtended && !seg.children.empty()) {
            //Celda extendida con sub-tabla
            dot << "        <TD VALIGN=\"MIDDLE\" ALIGN=\"CENTER\" "
                   "BGCOLOR=\"" << seg.color << "\">\n";
            dot << "          <TABLE BORDER=\"0\" CELLBORDER=\"1\" "
                   "CELLSPACING=\"2\" CELLPADDING=\"4\">\n";

            //Fila de encabezado extendida
            dot << "            <TR><TD COLSPAN=\""
                << seg.children.size()
                << "\" BGCOLOR=\"#117a65\" ALIGN=\"CENTER\">"
                << "<FONT COLOR=\"white\"><B>Extendida</B></FONT></TD></TR>\n";

            //Fila de hijos
            dot << "            <TR>\n";
            for (const auto& ch : seg.children) {
                double cpct = (double)ch.size / mbr.MbrSize * 100.0;
                std::ostringstream cpctStr;
                cpctStr << std::fixed << std::setprecision(1) << cpct << "%\ndel Disco";

                std::string chColor = ch.isFree ? "#5d6d7e" :
                                      (ch.label.find("EBR") != std::string::npos)
                                          ? "#1e8449" : "#2e86c1";

                dot << "              <TD BGCOLOR=\"" << chColor
                    << "\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\">"
                    << "<FONT COLOR=\"white\"><B>" << ch.label << "</B><BR/>"
                    << cpctStr.str() << "</FONT></TD>\n";
            }
            dot << "            </TR>\n";
            dot << "          </TABLE>\n";
            dot << "        </TD>\n";

        } else {
            //Celda simple
            dot << "        <TD BGCOLOR=\"" << seg.color
                << "\" ALIGN=\"CENTER\" VALIGN=\"MIDDLE\" "
                << "WIDTH=\"80\" HEIGHT=\"80\"><FONT COLOR=\"white\"><B>"
                << seg.label << "</B><BR/>"
                << pctStr.str() << " del disco</FONT></TD>\n";
        }
    }

    dot << "      </TR>\n";
    dot << "    </TABLE>>];\n";
    dot << "}\n";

    disk.close();

    //Determinar formato por extensión 
    std::string ext = fs::path(outPath).extension().string();
    std::string fmt = (ext == ".pdf") ? "pdf" : "jpg";

    prepareOutputPath(outPath);
    std::string err = generateImage(dot.str(), outPath, fmt);
    if (!err.empty()) return err;

    return "Reporte DISK generado: " + outPath;
}

//REPORTES EXT2
//FUNCIÓN PRINCIPAL — cmdRep
std::string cmdRep(std::map<std::string, std::string> params) {

    std::string id          = params.count("id")           ? params["id"]           : "";
    std::string path        = params.count("path")         ? params["path"]         : "";
    std::string name        = params.count("name")         ? params["name"]         : "";
    std::string pathFileLs  = params.count("path_file_ls") ? params["path_file_ls"] : (params.count("file") ? params["file"] : "");

    if (id.empty())   return "Error: Falta parámetro -id\n";
    if (path.empty()) return "Error: Falta parámetro -path\n";
    if (name.empty()) return "Error: Falta parámetro -name\n";

    std::transform(name.begin(), name.end(), name.begin(), ::tolower);

    //Validar nombre de reporte
    static const std::vector<std::string> validNames = {
        "mbr","disk","sb","bm_inode","bm_block","inode","block","tree","file","ls"
    };
    if (std::find(validNames.begin(), validNames.end(), name) == validNames.end())
        return "Error: Reporte '" + name + "' no válido. Valores aceptados: mbr, disk, sb, bm_inode, bm_block, inode, block, tree, file, ls\n";

    //Buscar partición montada
    const MountedPartition* mp = getMountedById(id);
    if (!mp)
        return "Error: No existe partición montada con id '" + id + "'\n";

    std::string diskPath = mp->Path;
    if (!archivoExiste(diskPath))
        return "Error: El disco '" + diskPath + "' no existe\n";

    //Obtener sbStart desde la partición
    long sbStart = mp->Start;

    //Despachar
    if (name == "mbr")      return repMBR    (diskPath, path);
    if (name == "disk")     return repDisk   (diskPath, path);
    if (name == "sb")       return repSB     (diskPath, path, sbStart);
    if (name == "bm_inode") return repBmInode(diskPath, path, sbStart);
    if (name == "bm_block") return repBmBlock(diskPath, path, sbStart);
    if (name == "inode")    return repInode  (diskPath, path, sbStart);
    if (name == "block")    return repBlock  (diskPath, path, sbStart);
    if (name == "tree")     return repTree   (diskPath, path, sbStart);
    if (name == "file")     return repFile   (diskPath, path, sbStart, pathFileLs);
    if (name == "ls")       return repLs     (diskPath, path, sbStart, pathFileLs);

    return "Error: Reporte no implementado\n";
}