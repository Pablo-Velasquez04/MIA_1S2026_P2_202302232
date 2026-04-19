//Backend/Utilidades/ResetUtil.cpp
#include "../Mounting/Mounting.h"
#include "../FileSystem/FileSystem.h"
#include <vector>
#include <map>

// Extern declarations for variables in Mount.cpp
extern std::vector<MountedPartition> mountedPartitions;
extern std::map<std::string, char>   diskLetterMap;
extern std::map<std::string, int>    diskPartCount;
extern char nextLetter;

// Extern declaration for variable in Login.cpp
extern ActiveSession currentSession;

void resetMounts() {
    mountedPartitions.clear();
    diskLetterMap.clear();
    diskPartCount.clear();
    nextLetter = 'A';
}

void resetSession() {
    currentSession.active      = false;
    currentSession.user        = "";
    currentSession.partitionId = "";
    currentSession.diskPath    = "";
    currentSession.uid         = -1;
    currentSession.gid         = -1;
    currentSession.sbStart     = -1;
}
