//Reportes/RepLs.cpp

#include "Reportes.h"
#include "RepHelpers.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <vector>

#include <cstring>
#include "../FilesFolders/FilesFolders.h"


std::string repLs(const std::string& diskPath, const std::string& outPath, long sbStart, const std::string& pathFile) {
    if (pathFile.empty()) return "Error: Falta parámetro -path_file_ls para reporte ls\n";
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco " + diskPath;
    SuperBloque sb;
    disk.seekg(sbStart);
    disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    if (disk.fail()) { disk.close(); return "Error: No se pudo leer el SuperBloque"; }
    int inodeIdx = ffResolvePath(disk, sb, pathFile, false, 1, 1);
    if (inodeIdx < 0) { disk.close(); return "Error: La ruta '" + pathFile + "' no existe\n"; }
    Inode dirInode;
    if (!ffReadInode(disk, sb, inodeIdx, dirInode)) { disk.close(); return "Error: No se pudo leer el inodo\n"; }
    if (dirInode.Type != '0') { disk.close(); return "Error: '" + pathFile + "' no es una carpeta\n"; }

    struct LsEntry {
        std::string name, type, perms, owner, group, dMod, hMod, fCre;
        int size;
    };

    std::vector<LsEntry> entries;

    auto processFB = [&](int bidx) {
        if (bidx < 0) return;
        FolderBlock fb;
        if (!ffReadFolderBlock(disk, sb, bidx, fb)) return;
        for (int e = 0; e < 4; e++) {
            std::string n(fb.content[e].Name, strnlen(fb.content[e].Name, 12));
            if (n.empty() || n == "." || n == "..") continue;
            int cIno = fb.content[e].Inodo;
            if (cIno < 0) continue;
            Inode ci;
            if (!ffReadInode(disk, sb, cIno, ci)) continue;

            char dMod[20], hMod[20], fCre[20];
            struct tm* tm_m = localtime(&ci.Mtime);
            strftime(dMod, 20, "%d/%m/%Y", tm_m);
            strftime(hMod, 20, "%H:%M", tm_m);
            struct tm* tm_c = localtime(&ci.Ctime);
            strftime(fCre, 20, "%d/%m/%Y %H:%M", tm_c);

            entries.push_back({
                n, 
                (ci.Type == '1' ? "Archivo" : "Carpeta"), 
                permToStr(ci.Type, ci.Perm), 
                uidToName(disk, sb, ci.Uid), 
                gidToName(disk, sb, ci.Gid), 
                dMod, 
                hMod, 
                fCre,
                ci.Size
            });

        }

    };

    for (int b = 0; b < 12; b++) processFB(dirInode.Block[b]);
    if (dirInode.Block[12] >= 0) {
        PointerBlock pb;
        if (repReadPointerBlock(disk, sb, dirInode.Block[12], pb))
            for (int p = 0; p < 16; p++) processFB(pb.pointers[p]);
    }
    disk.close();

    std::ostringstream dot;
    dot << "digraph LS {\n" << "  graph [bgcolor=\"#1e1e2e\" fontname=\"Helvetica\"];\n" << "  node  [shape=plain fontname=\"Helvetica\"];\n\n"
        << "  ls [label=<\n" << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"6\" BGCOLOR=\"#1e1e2e\">\n"
        << "      <TR><TD COLSPAN=\"9\" BGCOLOR=\"#1a5276\" ALIGN=\"CENTER\"><FONT COLOR=\"white\"><B>ls: " << pathFile << "</B></FONT></TD></TR>\n"
        << "      <TR><TD BGCOLOR=\"#aed6f1\"><B>Permisos</B></TD><TD BGCOLOR=\"#aed6f1\"><B>Owner</B></TD><TD BGCOLOR=\"#aed6f1\"><B>Grupo</B></TD><TD BGCOLOR=\"#aed6f1\"><B>Size</B></TD><TD BGCOLOR=\"#aed6f1\"><B>Modificación</B></TD><TD BGCOLOR=\"#aed6f1\"><B>Hora</B></TD><TD BGCOLOR=\"#aed6f1\"><B>Tipo</B></TD><TD BGCOLOR=\"#aed6f1\"><B>Creación</B></TD><TD BGCOLOR=\"#aed6f1\"><B>Nombre</B></TD></TR>\n";


    bool alt = false;
    for (const auto& e : entries) {
        std::string bg = alt ? "#d6eaf8" : "#ebf5fb";
        dot << "      <TR><TD BGCOLOR=\"" << bg << "\">" << e.perms << "</TD><TD BGCOLOR=\"" << bg << "\">" << e.owner << "</TD><TD BGCOLOR=\"" << bg << "\">" << e.group << "</TD><TD BGCOLOR=\"" << bg << "\">" << e.size << "</TD><TD BGCOLOR=\"" << bg << "\">" << e.dMod << "</TD><TD BGCOLOR=\"" << bg << "\">" << e.hMod << "</TD><TD BGCOLOR=\"" << bg << "\">" << e.type << "</TD><TD BGCOLOR=\"" << bg << "\">" << e.fCre << "</TD><TD BGCOLOR=\"" << bg << "\">" << e.name << "</TD></TR>\n";
        alt = !alt;
    }


    dot << "    </TABLE>>];\n}\n";

    prepareOutputPath(outPath);
    std::string fmt = (fs::path(outPath).extension() == ".pdf") ? "pdf" : "jpg";
    std::string err = generateImage(dot.str(), outPath, fmt);
    if (!err.empty()) return err;
    return "Reporte LS generado: " + outPath;
}
