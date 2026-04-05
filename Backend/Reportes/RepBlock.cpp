//Reportes/RepBlock.cpp
#include "Reportes.h"
#include "RepHelpers.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <functional>
#include "../FilesFolders/FilesFolders.h"



std::string repBlock(const std::string& diskPath, const std::string& outPath, long sbStart) {
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco " + diskPath;

    SuperBloque sb;
    disk.seekg(sbStart);
    disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    if (disk.fail()) { disk.close(); return "Error: No se pudo leer el SuperBloque"; }

    std::vector<char> bmB = readBitmap(disk, sb.BmBlockStart, sb.BlocksCount);
    std::vector<char> bmI = readBitmap(disk, sb.BmInodeStart, sb.InodesCount);
    if (bmB.empty() || bmI.empty()) { disk.close(); return "Error: No se pudo leer los bitmaps"; }

    std::vector<char> blockType(sb.BlocksCount, 0);

    for (int i = 0; i < sb.InodesCount; i++) {
        if (bmI[i] != '1' && bmI[i] != 1) continue;
        Inode inode;
        if (!ffReadInode(disk, sb, i, inode)) continue;


        char btype = (inode.Type == '0') ? 'F' : 'A';
        for (int b = 0; b < 12; b++) {
            if (inode.Block[b] >= 0 && inode.Block[b] < sb.BlocksCount) 
                blockType[inode.Block[b]] = btype;
        }

        std::function<void(int, int)> markPointers = [&](int pidx, int level) {
            if (pidx < 0 || pidx >= sb.BlocksCount) return;
            blockType[pidx] = 'P';
            PointerBlock pb;
            if (repReadPointerBlock(disk, sb, pidx, pb)) {
                for (int p = 0; p < 16; p++) {
                    int nextP = pb.pointers[p];
                    if (nextP < 0 || nextP >= sb.BlocksCount) continue;
                    if (level == 1) blockType[nextP] = btype;
                    else markPointers(nextP, level - 1);
                }
            }
        };

        if (inode.Block[12] >= 0) markPointers(inode.Block[12], 1);
        if (inode.Block[13] >= 0) markPointers(inode.Block[13], 2);
        if (inode.Block[14] >= 0) markPointers(inode.Block[14], 3);
    }

    std::ostringstream dot;
    dot << "digraph BLOCKS {\n";
    dot << "  graph [bgcolor=\"#1e1e2e\" fontname=\"Helvetica\" rankdir=\"LR\"];\n";
    dot << "  node  [shape=plain fontname=\"Helvetica\"];\n\n";

    std::string prevNode;
    bool first = true;

    for (int i = 0; i < sb.BlocksCount; i++) {
        if (bmB[i] != '1' && bmB[i] != 1) continue;

        char bt = blockType[i];
        if (bt == 0) continue;

        std::string nodeName = "blk" + std::to_string(i);
        std::string hdrColor, rowColor, cellColor, title;

        if (bt == 'F') {
            title = "Bloque Carpeta " + std::to_string(i);
            hdrColor = "#1a5276"; rowColor = "#aed6f1"; cellColor = "#d6eaf8";
        } else if (bt == 'A') {
            title = "Bloque Archivo " + std::to_string(i);
            hdrColor = "#1e8449"; rowColor = "#a9dfbf"; cellColor = "#d5f5e3";
        } else {
            title = "Bloque Apuntadores " + std::to_string(i);
            hdrColor = "#7d6608"; rowColor = "#f9e79f"; cellColor = "#fef9e7";
        }

        dot << "  " << nodeName << " [label=<\n";
        dot << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"5\" BGCOLOR=\"#1e1e2e\">\n";
        dot << "      <TR><TD COLSPAN=\"2\" BGCOLOR=\"" << hdrColor << "\" ALIGN=\"CENTER\"><FONT COLOR=\"white\"><B>" << title << "</B></FONT></TD></TR>\n";

        if (bt == 'F') {
            FolderBlock fb;
            dot << "      <TR><TD BGCOLOR=\"" << rowColor << "\"><B>b_name</B></TD><TD BGCOLOR=\"" << rowColor << "\"><B>b_inodo</B></TD></TR>\n";
            if (ffReadFolderBlock(disk, sb, i, fb)) {
                for (int e = 0; e < 4; e++) {
                    std::string name(fb.content[e].Name, strnlen(fb.content[e].Name, 12));
                    if (name.empty() && fb.content[e].Inodo <= 0) continue;
                    dot << "      <TR><TD BGCOLOR=\"" << cellColor << "\">" << name << "</TD><TD BGCOLOR=\"" << cellColor << "\">" << fb.content[e].Inodo << "</TD></TR>\n";
                }
            }
        } else if (bt == 'A') {
            FileBlock ab;
            dot << "      <TR><TD BGCOLOR=\"" << rowColor << "\" COLSPAN=\"2\"><B>b_content</B></TD></TR>\n";
            if (ffReadFileBlock(disk, sb, i, ab)) {
                std::string content(ab.content, strnlen(ab.content, 64));
                std::string escaped;
                for (char c : content) {
                    if (c == '<') escaped += "&lt;";
                    else if (c == '>') escaped += "&gt;";
                    else if (c == '&') escaped += "&amp;";
                    else escaped += c;
                }
                dot << "      <TR><TD BGCOLOR=\"" << cellColor << "\" COLSPAN=\"2\">" << escaped << "</TD></TR>\n";
            }
        } else {
            PointerBlock pb;
            dot << "      <TR><TD BGCOLOR=\"" << rowColor << "\" COLSPAN=\"2\"><B>b_pointers</B></TD></TR>\n";
            if (repReadPointerBlock(disk, sb, i, pb)) {
                std::string ptrs;
                for (int p = 0; p < 16; p++) {
                    ptrs += std::to_string(pb.pointers[p]);
                    if (p < 15) ptrs += ", ";
                }
                dot << "      <TR><TD BGCOLOR=\"" << cellColor << "\" COLSPAN=\"2\">" << ptrs << "</TD></TR>\n";
            }
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
    return "Reporte BLOCK generado: " + outPath;
}
