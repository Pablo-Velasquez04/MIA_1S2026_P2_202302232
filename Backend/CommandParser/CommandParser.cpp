// CommandParser/CommandParser.cpp

#include "CommandParser.h"
#include "../DiskManagement/DiskManagement.h"
#include "../Mounting/Mounting.h"
#include "../FileSystem/FileSystem.h"
#include "../FilesFolders/FilesFolders.h"
#include "../Reportes/Reportes.h"
#include "../Users/Users.h"
#include <sstream>
#include <algorithm>
#include <map>

// Convierte "mkdisk -size=10 -unit=M -path=/home/disco.mia"
// en un mapa: { "size":"10", "unit":"M", "path":"/home/disco.mia" }

bool hasUnclosedQuote(const std::string& line) {
    int count = 0;
    for (char c : line)
        if (c == '"') count++;

    return count % 2 != 0;
}

std::map<std::string, std::string> parseParams(const std::string& line) {
    std::map<std::string, std::string> params;
    std::istringstream iss(line);
    std::string token;

    while (iss >> token) {
        if (token.empty() || token[0] != '-')
            continue;

        // quitar el prefijo '-'
        std::string content = token.substr(1);
        auto eqPos = content.find('=');
        if (eqPos != std::string::npos) {
            std::string key = content.substr(0, eqPos);
            std::string value = content.substr(eqPos + 1);

            // manejar valores entrecomillados con espacios
            if (!value.empty() && value.front() == '"') {
                value.erase(0,1);
                if (value.back() != '"') {
                    std::string extra;
                    while (iss >> extra) {
                        value += " " + extra;
                        if (!extra.empty() && extra.back() == '"') break;
                    }
                }
            if (!value.empty() && value.back() == '"')
                value.pop_back();
        }

            // normalizar la bandera a minúsculas, preservar el valor intacto
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            
            params[key] = value;
        } else {
            // bandera sin valor
            std::string key = content;
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            params[key] = "true";
        }
    }
    return params;
}

// Procesa una línea completa
std::string processLine(const std::string& rawLine) {
    std::string line = rawLine;

    if (hasUnclosedQuote(line))
        return "Error: Comillas sin cerrar en la línea";
    
    // Ignorar comentarios y líneas vacías
    if (line.empty() || line[0] == '#') 
        return line; // retorna el comentario para mostrarlo
    
    // Obtener el comando (primera palabra)
    std::istringstream iss(line);
    std::string command;
    iss >> command;
    if (!command.empty() && command[0] == '-') {
        command = command.substr(1);
    }    

    std::transform(command.begin(), command.end(), 
                   command.begin(), ::tolower);

    auto params = parseParams(line);

    if (params.count("__error__"))
        return "Error: " + params["__error__"];
    
    if      (command == "mkdisk")  return cmdMkdisk(params);
    else if (command == "rmdisk")  return cmdRmdisk(params);
    else if (command == "fdisk")   return cmdFdisk(params);
    else if (command == "mount")   return cmdMount(params);
    else if (command == "unmount") return cmdUnmount(params);
    else if (command == "mounted") return cmdMounted();
    else if (command == "mkfs")    return cmdMkfs(params);
    else if (command == "login")   return cmdLogin(params);
    else if (command == "logout")  return cmdLogout();
    else if (command == "mkfile")  return cmdMkfile(params);
    else if (command == "mkdir")   return cmdMkdir(params);
    else if (command == "cat")     return cmdCat(params);
    else if (command == "remove")  return cmdRemove(params);
    else if (command == "rename")  return cmdRename(params);
    else if (command == "copy")    return cmdCopy(params);
    else if (command == "rep")     return cmdRep(params);
    else if (command == "mkgrp")   return cmdMkgrp(params);
    else if (command == "rmgrp")   return cmdRmgrp(params);
    else if (command == "mkusr")   return cmdMkusr(params);
    else if (command == "rmusr")   return cmdRmusr(params);
    else if (command == "chgrp")   return cmdChgrp(params);
    else if (command == "execute") return cmdReset();
    else return "Error: Comando '" + command + "' no reconocido, intente de nuevo o ingrese un comando válido";
}

//Procesa un script completo (múltiples líneas)
std::string processScript(const std::string& script) {
    std::string output;
    std::istringstream iss(script);
    std::string line;
    
    int lineNum = 1;

    while (std::getline(iss, line)) {
        output += "Linea " + std::to_string(lineNum) + ": ";
        output += processLine(line) + "\n";
        lineNum++;
    }
    return output;
}