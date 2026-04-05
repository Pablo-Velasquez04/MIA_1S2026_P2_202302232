//Reportes/RepTree.cpp
#include "Reportes.h"
#include "RepHelpers.h"
#include <fstream>
#include <sstream>
#include <set>
#include <cstring>
#include <functional>
#include "../FilesFolders/FilesFolders.h"

static void treeVisitInode(std::fstream& disk, const SuperBloque& sb, int inodeIdx,
                           const std::string& name, std::ostringstream& dot,
                           std::set<int>& visited) {
    if (inodeIdx < 0 || visited.count(inodeIdx)) return;
    visited.insert(inodeIdx);

    Inode inode;
    if (!ffReadInode(disk, sb, inodeIdx, inode)) return;

    std::string inodeNode = "in" + std::to_string(inodeIdx);
    bool isFile = (inode.Type == '1');

    std::string hdrColor  = isFile ? "#1e8449" : "#2471a3";
    std::string rowColor  = isFile ? "#a9dfbf" : "#aed6f1";
    std::string cellColor = isFile ? "#d5f5e3" : "#d6eaf8";

    dot << "  " << inodeNode << " [label=<\n";
    dot << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    dot << "      <TR><TD COLSPAN=\"2\" BGCOLOR=\"" << hdrColor
        << "\" ALIGN=\"CENTER\"><FONT COLOR=\"white\"><B>I-nodo " << inodeIdx
        << " (" << name << ")</B></FONT></TD></TR>\n";

    auto row = [&](const std::string& lbl, const std::string& val) {
        dot << "      <TR>"
            << "<TD BGCOLOR=\"" << rowColor << "\"><B>" << lbl << "</B></TD>"
            << "<TD BGCOLOR=\"" << cellColor << "\">" << val << "</TD>"
            << "</TR>\n";
    };

    // Todos los campos que pide el enunciado
    row("i_uid",   std::to_string(inode.Uid));
    row("i_gid",   std::to_string(inode.Gid));
    row("i_s",     std::to_string(inode.Size) + " bytes");
    row("i_atime", formatearTiempo(inode.Atime));
    row("i_ctime", formatearTiempo(inode.Ctime));
    row("i_mtime", formatearTiempo(inode.Mtime));
    row("i_type",  std::string(1, inode.Type));
    std::string permStr;
    permStr += inode.Perm[0]; permStr += inode.Perm[1]; permStr += inode.Perm[2];
    row("i_perm", permStr);

    // Bloques directos [0..11]
    for (int b = 0; b < 12; b++) {
        if (inode.Block[b] == -1) continue;
        dot << "      <TR>"
            << "<TD BGCOLOR=\"" << rowColor << "\"><B>i_block[" << b << "]</B></TD>"
            // ✅ PORT en la segunda columna (el número)
            << "<TD BGCOLOR=\"" << cellColor << "\" PORT=\"b" << b << "\">" 
            << inode.Block[b] << "</TD>"
            << "</TR>\n";
    }
    // Bloques indirectos [12..14]
    for (int b = 12; b < 15; b++) {
        if (inode.Block[b] == -1) continue;
        dot << "      <TR>"
            << "<TD BGCOLOR=\"" << rowColor << "\"><B>i_block[" << b << "]</B></TD>"
            // ✅ PORT en la segunda columna
            << "<TD BGCOLOR=\"" << cellColor << "\" PORT=\"b" << b << "\">" 
            << inode.Block[b] << "</TD>"
            << "</TR>\n";
    }

    dot << "    </TABLE>>];\n\n";

    // ── Bloque de CARPETA (naranja) ───────────────────────────────────────────
    auto processFB = [&](int bidx, const std::string& sourcePort) {
        if (bidx < 0) return;
        FolderBlock fb;
        if (!ffReadFolderBlock(disk, sb, bidx, fb)) return;

        std::string blkNode = "blk" + std::to_string(bidx);
        dot << "  " << blkNode << " [label=<\n";
        dot << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
        dot << "      <TR><TD COLSPAN=\"2\" BGCOLOR=\"#e67e22\" ALIGN=\"CENTER\">"
            << "<FONT COLOR=\"white\"><B>Bloque " << bidx << "</B></FONT></TD></TR>\n";
        // Header de columnas
        dot << "      <TR>"
            << "<TD BGCOLOR=\"#f0b27a\"><B>b_name</B></TD>"
            << "<TD BGCOLOR=\"#f0b27a\"><B>b_inodo</B></TD>"
            << "</TR>\n";
        for (int e = 0; e < 4; e++) {
            int eIno = fb.content[e].Inodo;
            if (eIno == -1) continue;
            std::string eName(fb.content[e].Name, strnlen(fb.content[e].Name, 12));
            if (eName.empty()) eName = "-";
            dot << "      <TR>"
                << "<TD BGCOLOR=\"#fdebd0\" PORT=\"e" << e << "\">" << eName << "</TD>"
                << "<TD BGCOLOR=\"#fdebd0\">" << eIno << "</TD>"
                << "</TR>\n";
        }
        dot << "    </TABLE>>];\n\n";
        dot << "  " << sourcePort << " -> " << blkNode << ":w;\n";

        // Recursión para entradas que no son "." ni ".."
        for (int e = 0; e < 4; e++) {
            std::string eName(fb.content[e].Name, strnlen(fb.content[e].Name, 12));
            int eIno = fb.content[e].Inodo;
            if (eName.empty() || eName == "." || eName == ".." || eIno <= 0) continue;
            std::string childNode = "in" + std::to_string(eIno);
            dot << "  " << blkNode << ":e" << e << ":e -> " << childNode << ":w;\n";
            treeVisitInode(disk, sb, eIno, eName, dot, visited);
        }
    };

    // ── Bloque de ARCHIVO (verde) ─────────────────────────────────────────────
    auto processFileBk = [&](int bidx, const std::string& sourcePort) {
        if (bidx < 0) return;
        FileBlock fb;
        if (!ffReadFileBlock(disk, sb, bidx, fb)) return;

        std::string blkNode = "fblk" + std::to_string(bidx);
        std::string content(fb.content, strnlen(fb.content, 64));
        std::string esc;
        for (char c : content) {
            if      (c == '<')  esc += "&lt;";
            else if (c == '>')  esc += "&gt;";
            else if (c == '&')  esc += "&amp;";
            else if (c == '\n') esc += "\\n";
            else esc += c;
        }

        dot << "  " << blkNode << " [label=<\n";
        dot << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
        dot << "      <TR><TD BGCOLOR=\"#1e8449\" ALIGN=\"CENTER\">"
            << "<FONT COLOR=\"white\"><B>Bloque " << bidx << "</B></FONT></TD></TR>\n";
        dot << "      <TR><TD BGCOLOR=\"#a9dfbf\"><B>b_content</B></TD></TR>\n";
        dot << "      <TR><TD BGCOLOR=\"#d5f5e3\">" << esc << "</TD></TR>\n";
        dot << "    </TABLE>>];\n\n";
        dot << "  " << sourcePort << " -> " << blkNode << ":w;\n";
    };

    // ── Bloque APUNTADOR (amarillo) ───────────────────────────────────────────
    std::function<void(int, const std::string&, int)> processPointers =
        [&](int pidx, const std::string& parentPort, int level) {
        if (pidx < 0) return;
        PointerBlock pb;
        if (!repReadPointerBlock(disk, sb, pidx, pb)) return;

        std::string ptrNode = "ptr" + std::to_string(pidx);
        dot << "  " << ptrNode << " [label=<\n";
        dot << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
        dot << "      <TR><TD BGCOLOR=\"#efeb04\" ALIGN=\"CENTER\">"
            << "<FONT COLOR=\"black\"><B>Bloque " << pidx << "</B></FONT></TD></TR>\n";
        dot << "      <TR><TD BGCOLOR=\"#f9e79f\"><B>b_pointers</B></TD></TR>\n";
        for (int p = 0; p < 16; p++) {
            if (pb.pointers[p] == -1) continue;
            dot << "      <TR><TD BGCOLOR=\"#fef9e7\" PORT=\"p" << p << "\">"
                << pb.pointers[p] << "</TD></TR>\n";
        }
        dot << "    </TABLE>>];\n\n";
        dot << "  " << parentPort << " -> " << ptrNode << ":w;\n";

        for (int p = 0; p < 16; p++) {
            int nextP = pb.pointers[p];
            if (nextP < 0) continue;
            std::string pPort = ptrNode + ":p" + std::to_string(p) + ":e";
            if (level == 1) {
                if (isFile) processFileBk(nextP, pPort);
                else        processFB(nextP, pPort);
            } else {
                processPointers(nextP, pPort, level - 1);
            }
        }
    };

    // ── Despachar bloques del inodo ───────────────────────────────────────────
    for (int b = 0; b < 12; b++) {
        if (inode.Block[b] == -1) continue;
        std::string port = inodeNode + ":b" + std::to_string(b) + ":e";
        if (isFile) processFileBk(inode.Block[b], port);
        else        processFB(inode.Block[b], port);
    }
    if (inode.Block[12] >= 0)
        processPointers(inode.Block[12], inodeNode + ":b12:e", 1);
    if (inode.Block[13] >= 0)
        processPointers(inode.Block[13], inodeNode + ":b13:e", 2);
    if (inode.Block[14] >= 0)
        processPointers(inode.Block[14], inodeNode + ":b14:e", 3);
}

std::string repTree(const std::string& diskPath, const std::string& outPath, long sbStart) {
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco " + diskPath;

    SuperBloque sb;
    disk.seekg(sbStart);
    disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    if (disk.fail()) { disk.close(); return "Error: No se pudo leer el SuperBloque"; }

    std::ostringstream dot;
    dot << "digraph TREE {\n";
    dot << "  graph [\n";
    dot << "    bgcolor=\"white\"\n";
    dot << "    fontname=\"Helvetica\"\n";
    dot << "    rankdir=\"LR\"\n";
    dot << "    ranksep=1.5\n";   //espacio horizontal entre columnas
    dot << "    nodesep=0.8\n";   //espacio vertical entre nodos
    dot << "    splines=polyline\n"; //flechas más limpias
  	dot << "  ];\n";
    dot << "  node [shape=plain fontname=\"Helvetica\"];\n\n";

    std::set<int> visited;
    treeVisitInode(disk, sb, 0, "/", dot, visited);
    disk.close();

    dot << "}\n";

    prepareOutputPath(outPath);

    std::string ext = fs::path(outPath).extension().string();
    std::string fmt = (ext == ".pdf") ? "pdf" : "jpg";

    std::string err = generateImage(dot.str(), outPath, fmt);
    if (!err.empty()) return err;
    return "Reporte TREE generado: " + outPath;
}