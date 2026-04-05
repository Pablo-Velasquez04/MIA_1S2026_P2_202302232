//Reportes.h
#pragma once
#include <string>
#include <map>

// Función principal de despacho
std::string cmdRep(std::map<std::string, std::string> params);

// Reportes de Sistema de Archivos (en archivos separados)
std::string repSB(const std::string& diskPath, const std::string& outPath, long sbStart);
std::string repBitmap(const std::string& diskPath, long start, int count, const std::string& outPath, const std::string& title);
std::string repBmInode(const std::string& diskPath, const std::string& outPath, long sbStart);
std::string repBmBlock(const std::string& diskPath, const std::string& outPath, long sbStart);
std::string repInode(const std::string& diskPath, const std::string& outPath, long sbStart);
std::string repBlock(const std::string& diskPath, const std::string& outPath, long sbStart);
std::string repTree(const std::string& diskPath, const std::string& outPath, long sbStart);
std::string repFile(const std::string& diskPath, const std::string& outPath, long sbStart, const std::string& pathFile);
std::string repLs(const std::string& diskPath, const std::string& outPath, long sbStart, const std::string& pathFile);