//Reportes/Rep_Sb.cpp
#include "Reportes.h"
#include "RepHelpers.h"
#include <fstream>
#include <sstream>

std::string repSB(const std::string& diskPath, const std::string& outPath, long sbStart) {
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco " + diskPath;

    SuperBloque sb;
    disk.seekg(sbStart);
    disk.read(reinterpret_cast<char*>(&sb), sizeof(SuperBloque));
    if (disk.fail()) { disk.close(); return "Error: No se pudo leer el SuperBloque"; }
    disk.close();

    std::string diskName = fs::path(diskPath).filename().string();
    std::ostringstream dot;
    dot << "digraph SB {\n";
    dot << "  graph [bgcolor=\"#1e1e2e\" fontname=\"Helvetica\"];\n";
    dot << "  node  [shape=plain fontname=\"Helvetica\"];\n\n";

    dot << "  sb [label=<\n";
    dot << "    <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"6\" BGCOLOR=\"#1e1e2e\">\n";
    dot << "      <TR><TD COLSPAN=\"2\" BGCOLOR=\"#1a5276\" ALIGN=\"CENTER\"><FONT COLOR=\"white\"><B>Reporte de SUPERBLOQUE</B></FONT></TD></TR>\n";

    auto row = [&](const std::string& lbl, const std::string& val, bool alt) {
        std::string bg1 = alt ? "#aed6f1" : "#d6eaf8";
        std::string bg2 = alt ? "#d6eaf8" : "#ebf5fb";
        dot << "      <TR><TD BGCOLOR=\"" << bg1 << "\" ALIGN=\"RIGHT\"><B>" << lbl << "</B></TD><TD BGCOLOR=\"" << bg2 << "\" ALIGN=\"LEFT\">" << val << "</TD></TR>\n";
    };

    bool alt = false;
    row("sb_nombre_hd", diskName, alt); alt=!alt;
    row("sb_filesystem_type", std::to_string(sb.FilesystemType), alt); alt=!alt;
    row("sb_inodes_count", std::to_string(sb.InodesCount), alt); alt=!alt;
    row("sb_blocks_count", std::to_string(sb.BlocksCount), alt); alt=!alt;
    row("sb_free_blocks_count", std::to_string(sb.FreeBlocksCount), alt); alt=!alt;
    row("sb_free_inodes_count", std::to_string(sb.FreeInodesCount), alt); alt=!alt;
    row("sb_mtime", formatearTiempo(sb.Mtime), alt); alt=!alt;
    row("sb_umtime", formatearTiempo(sb.Umtime), alt); alt=!alt;
    row("sb_mnt_count", std::to_string(sb.MntCount), alt); alt=!alt;

    row("sb_magic", "0xEF53", alt); alt=!alt;
    row("sb_inode_size", std::to_string(sb.InodeSize), alt); alt=!alt;
    row("sb_block_size", std::to_string(sb.BlockSize), alt); alt=!alt;
    row("sb_first_ino", std::to_string(sb.FirstIno), alt); alt=!alt;
    row("sb_first_blo", std::to_string(sb.FirstBlo), alt); alt=!alt;
    row("sb_bm_inode_start", std::to_string(sb.BmInodeStart), alt); alt=!alt;
    row("sb_bm_block_start", std::to_string(sb.BmBlockStart), alt); alt=!alt;
    row("sb_inode_start", std::to_string(sb.InodeStart), alt); alt=!alt;
    row("sb_block_start", std::to_string(sb.BlockStart), alt);

    dot << "    </TABLE>>];\n}\n";

    prepareOutputPath(outPath);
    std::string fmt = (fs::path(outPath).extension() == ".pdf") ? "pdf" : "jpg";
    std::string err = generateImage(dot.str(), outPath, fmt);
    if (!err.empty()) return err;
    return "Reporte SB generado: " + outPath;
}
