// Users/Users.h
#pragma once
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include "../Estructuras/Structs.h"
#include "../FileSystem/FileSystem.h"  //ActiveSession viene de aquí

//Helpers compartidos 
bool        diskReadU   (std::fstream& disk, long pos, void* dest, size_t size);
bool        diskWriteU  (std::fstream& disk, long pos, const void* src, size_t size);
bool        readSuperblock  (std::fstream& disk, SuperBloque& sb);
std::string readUsersFile   (std::fstream& disk, const SuperBloque& sb);
bool        writeUsersFile  (std::fstream& disk, const SuperBloque& sb,
                             const std::string& newContent);
std::vector<std::string> parseLines  (const std::string& content);
std::string              buildContent(const std::vector<std::string>& lines);
int         getNextId   (const std::vector<std::string>& lines,
                         const std::string& type);
bool        groupExists (const std::vector<std::string>& lines,
                         const std::string& name);
bool        userExists  (const std::vector<std::string>& lines,
                         const std::string& name);
std::fstream openSessionDisk();

//Comandos 
std::string cmdMkgrp(std::map<std::string, std::string> params);
std::string cmdRmgrp(std::map<std::string, std::string> params);
std::string cmdMkusr(std::map<std::string, std::string> params);
std::string cmdRmusr(std::map<std::string, std::string> params);
std::string cmdChgrp(std::map<std::string, std::string> params);