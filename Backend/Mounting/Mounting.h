//Mounting/Mounting.h
#pragma once
#include <string>
#include <map>
#include <vector>

struct MountedPartition;

std::string cmdMount(std::map<std::string, std::string> params);
std::string cmdUnmount(std::map<std::string, std::string> params);
std::string cmdMounted();
void resetMounts();

struct MountedPartition {
    std::string Id;
    std::string Path;
    std::string Name;
    int         Start;
    int         Size;
    char        Type;
};

const std::vector<MountedPartition>& getMountedPartitions();
const MountedPartition* getMountedById(const std::string& id);