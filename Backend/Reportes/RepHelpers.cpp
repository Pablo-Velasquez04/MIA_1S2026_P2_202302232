//Reportes/RepHelpers.cpp
#include "RepHelpers.h"
#include "../Utilidades/Utilities.h"
#include <sstream>
#include <cstring>
#include <cstdlib>

std::string trimNull(const char* p) {
    std::string s(p);
    size_t pos = s.find('\0');
    if (pos != std::string::npos) s.resize(pos);
    return s;
}

bool repReadMBR(std::fstream& disk, MBR& mbr) {
    disk.seekg(0);
    disk.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));
    return !disk.fail();
}

bool repReadEBR(std::fstream& disk, int pos, EBR& ebr) {
    disk.seekg(pos);
    disk.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
    return !disk.fail();
}

std::string prepareOutputPath(const std::string& path) {
    fs::path p(path);
    if (p.has_parent_path())
        fs::create_directories(p.parent_path());
    return path;
}

bool runDot(const std::string& dotFile, const std::string& outFile, const std::string& format) {
    std::string cmd = "dot -T" + format + " \"" + dotFile +
                      "\" -o \"" + outFile + "\"";
    int ret = system(cmd.c_str());
    return ret == 0;
}

std::string generateImage(const std::string& dotContent, const std::string& outPath, const std::string& format) {
    std::string dotPath = outPath.substr(0, outPath.find_last_of('.')) + ".dot";
    std::ofstream dotFile(dotPath);
    if (!dotFile.is_open())
        return "Error: No se pudo crear archivo .dot en " + dotPath;
    dotFile << dotContent;
    dotFile.close();

    if (!runDot(dotPath, outPath, format))
        return "Error: Graphviz falló. ¿Está instalado? (sudo apt install graphviz)";

    fs::remove(dotPath);
    return "";
}

bool repReadPointerBlock(std::fstream& disk, const SuperBloque& sb, int idx, PointerBlock& out) {

    long pos = sb.BlockStart + (long)idx * sb.BlockSize;
    disk.seekg(pos);
    disk.read(reinterpret_cast<char*>(&out), sizeof(PointerBlock));
    return !disk.fail();
}

std::vector<char> readBitmap(std::fstream& disk, long start, int count) {
    std::vector<char> bm(count, 0);
    disk.seekg(start);
    disk.read(bm.data(), count);
    return bm;
}

std::string permToStr(char type, const char perm[3]) {
    auto toRWX = [](char c) -> std::string {
        int v = c - '0';
        std::string s = "---";
        if (v & 4) s[0] = 'r';
        if (v & 2) s[1] = 'w';
        if (v & 1) s[2] = 'x';
        return s;
    };
    std::string prefix = (type == '1') ? "-" : "d";
    return prefix + toRWX(perm[0]) + toRWX(perm[1]) + toRWX(perm[2]);
}

#include "../FilesFolders/FilesFolders.h"  

std::string readFileContent(std::fstream& disk, const SuperBloque& sb, const Inode& inode) {
    std::string content;
    int remaining = inode.Size;

    auto readBlock = [&](int bidx) {
        if (bidx < 0 || remaining <= 0) return;
        FileBlock fb;
        if (!ffReadFileBlock(disk, sb, bidx, fb)) return;
        int take = std::min(remaining, 64);
        content.append(fb.content, take);
        remaining -= take;
    };

    for (int i = 0; i < 12 && remaining > 0; i++) readBlock(inode.Block[i]);

    if (remaining > 0 && inode.Block[12] >= 0) {
        PointerBlock pb;
        if (repReadPointerBlock(disk, sb, inode.Block[12], pb))
            for (int i = 0; i < 16 && remaining > 0; i++) readBlock(pb.pointers[i]);
    }

    if (remaining > 0 && inode.Block[13] >= 0) {
        PointerBlock pb1;
        if (repReadPointerBlock(disk, sb, inode.Block[13], pb1))
            for (int i = 0; i < 16 && remaining > 0; i++) {
                if (pb1.pointers[i] < 0) continue;
                PointerBlock pb2;
                if (repReadPointerBlock(disk, sb, pb1.pointers[i], pb2))
                    for (int j = 0; j < 16 && remaining > 0; j++) readBlock(pb2.pointers[j]);
            }
    }
    return content;
}

std::string uidToName(std::fstream& disk, const SuperBloque& sb, int uid) {
    if (uid == 1) return "root";
    Inode userInode;
    if (!ffReadInode(disk, sb, 1, userInode)) return std::to_string(uid);
    std::string content = readFileContent(disk, sb, userInode);
    std::istringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        std::string tok;
        std::vector<std::string> parts;
        while (std::getline(ls, tok, ',')) parts.push_back(tok);
        if (parts.size() >= 4 && parts[1] == "U" && std::stoi(parts[0]) == uid) return parts[3];
    }
    return std::to_string(uid);
}

std::string gidToName(std::fstream& disk, const SuperBloque& sb, int gid) {
    if (gid == 1) return "root";
    Inode userInode;
    if (!ffReadInode(disk, sb, 1, userInode)) return std::to_string(gid);
    std::string content = readFileContent(disk, sb, userInode);
    std::istringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        std::istringstream ls(line);
        std::string tok;
        std::vector<std::string> parts;
        while (std::getline(ls, tok, ',')) parts.push_back(tok);
        if (parts.size() >= 3 && parts[1] == "G" && std::stoi(parts[0]) == gid) return parts[2];
    }
    return std::to_string(gid);
}

std::string repBitmap(const std::string& diskPath, long bmStart, int count,
                      const std::string& outPath, const std::string& title) {
    std::fstream disk(diskPath, std::ios::binary | std::ios::in);
    if (!disk.is_open()) return "Error: No se pudo abrir el disco " + diskPath;

    std::vector<char> bm = readBitmap(disk, bmStart, count);
    disk.close();

    std::string actualPath = outPath;
    if (fs::path(actualPath).extension() != ".txt") {
        actualPath = fs::path(actualPath).replace_extension(".txt").string();
    }

    prepareOutputPath(actualPath);
    std::ofstream out(actualPath);
    if (!out.is_open()) return "Error: No se pudo crear " + actualPath;

    for (int i = 0; i < count; i++) {
        // Soporta tanto '1'/'0' como 1/0 binario
        out << ((bm[i] == '1' || bm[i] == 1) ? "1" : "0");
        if ((i + 1) % 20 == 0)
            out << "\n";
        else if (i + 1 < count)
            out << " ";
    }
    if (count % 20 != 0) out << "\n";
    out.close();
    return "Reporte " + title + " generado: " + actualPath;

}

