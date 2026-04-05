//DiskManagement/DiskManagement.h
#pragma once
#include <string>
#include <map>

std::string cmdMkdisk(std::map<std::string, std::string> params);
std::string cmdRmdisk(std::map<std::string, std::string> params);
std::string cmdFdisk(std::map<std::string, std::string> params);