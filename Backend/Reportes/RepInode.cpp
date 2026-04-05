//Reportes/RepInode.cpp
#include "Reportes.h"
#include "RepHelpers.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include "../FilesFolders/FilesFolders.h"


std::string repInode(const std::string& diskPath, const std::string& outPath, long sbStart) {
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco " + diskPath;

    SuperBloque sb;
    disk.seekg(sbStart);
    disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    if (disk.fail()) { disk.close(); return "Error: No se pudo leer el SuperBloque"; }

    std::vector<char> bm = readBitmap(disk, sb.BmInodeStart, sb.InodesCount);
    if (bm.empty()) { disk.close(); return "Error: No se pudo leer el bitmap de inodos"; }

    std::ostringstream dot;
    dot << "digraph INODES {\n";
    dot << "  graph [bgcolor=\"#1e1e2e\" fontname=\"Helvetica\" rankdir=\"LR\"];\n";
    dot << "  node  [shape=plain fontname=\"Helvetica\"];\n\n";

    bool first = true;
    std::string prevNode;

    for (int i = 0; i < sb.InodesCount; i++) {
        // Soporta tanto '1' (char) como 1 (byte)
        if (bm[i] != '1' && bm[i] != 1) continue;

        Inode inode;
        if (!ffReadInode(disk, sb, i, inode)) continue;


        std::string nodeName = "in" + std::to_string(i);
        bool isFile = (inode.Type == '1');
        std::string hdrColor = isFile ? "#1e8449" : "#1a5276";
        std::string rowColor = isFile ? "#a9dfbf" : "#aed6f1";
        std::string cellColor = isFile ? "#d5f5e3" : "#d6eaf8";

        dot << "  " << nodeName << " [label=<\n";
        dot << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\" BGCOLOR=\"#1e1e2e\">\n";
        dot << "      <TR><TD COLSPAN=\"2\" BGCOLOR=\"" << hdrColor << "\" ALIGN=\"CENTER\"><FONT COLOR=\"white\"><B>Inodo " << i << "</B></FONT></TD></TR>\n";

        auto row = [&](const std::string& lbl, const std::string& val) {
            dot << "      <TR><TD BGCOLOR=\"" << rowColor << "\"><B>" << lbl << "</B></TD><TD BGCOLOR=\"" << cellColor << "\">" << val << "</TD></TR>\n";
        };

        row("i_uid", std::to_string(inode.Uid));
        row("i_gid", std::to_string(inode.Gid));
        row("i_s",   std::to_string(inode.Size));
        row("i_atime", formatearTiempo(inode.Atime));
        row("i_ctime", formatearTiempo(inode.Ctime));
        row("i_mtime", formatearTiempo(inode.Mtime));
        row("i_type", isFile ? "Archivo" : "Carpeta");

        row("i_perm", std::string(1, inode.Perm[0]) + inode.Perm[1] + inode.Perm[2]);

        for (int b = 0; b < 15; b++) {
            if (inode.Block[b] != -1)
                row("i_block[" + std::to_string(b) + "]", std::to_string(inode.Block[b]));
        }

        dot << "    </TABLE>>];\n\n";

        if (!first) dot << "  " << prevNode << " -> " << nodeName << " [color=\"#48c9b0\"];\n";
        prevNode = nodeName;
        first = false;
    }

    disk.close();
    dot << "}\n";

    prepareOutputPath(outPath);
    std::string fmt = (fs::path(outPath).extension() == ".pdf") ? "pdf" : "jpg";
    std::string err = generateImage(dot.str(), outPath, fmt);
    if (!err.empty()) return err;
    return "Reporte INODE generado: " + outPath;
}
