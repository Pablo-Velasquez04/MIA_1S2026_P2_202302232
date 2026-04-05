#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include "../Estructuras/Structs.h"
#include "../Estructuras/Ebr.h"
#include "../Utilidades/Utilities.h"

namespace fs = std::filesystem;

// Helpers comunes para reportes
std::string trimNull(const char* p);
bool repReadMBR(std::fstream& disk, MBR& mbr);
bool repReadEBR(std::fstream& disk, int pos, EBR& ebr);
std::string prepareOutputPath(const std::string& path);
bool runDot(const std::string& dotFile, const std::string& outFile, const std::string& format);
std::string generateImage(const std::string& dotContent, const std::string& outPath, const std::string& format);
bool repReadPointerBlock(std::fstream& disk, const SuperBloque& sb, int idx, PointerBlock& out);

std::vector<char> readBitmap(std::fstream& disk, long start, int count);
std::string permToStr(char type, const char perm[3]);
std::string readFileContent(std::fstream& disk, const SuperBloque& sb, const Inode& inode);
std::string uidToName(std::fstream& disk, const SuperBloque& sb, int uid);
std::string gidToName(std::fstream& disk, const SuperBloque& sb, int gid);
std::string repBitmap(const std::string& diskPath, long bmStart, int count,
                      const std::string& outPath, const std::string& title);
