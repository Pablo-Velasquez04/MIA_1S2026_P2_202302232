//DiskManagement/Rmdisk.cpp
#include <map>
#include <string>
#include <iostream>
#include "../Estructuras/Structs.h"
#include "../Utilidades/Utilities.h"

std::string cmdRmdisk(std::map<std::string, std::string> params){
    //Validar parámetro requerido
    if(params.find("path") == params.end()){
        return "Error: Parámetro requerido: -path";
    }

    //Obtener path y cambiar extensión a .mia
    std::string path = params.at("path");
    path = cambiarExtension(path, ".mia");

    //Verifica si el archivo existe
    if(!archivoExiste(path)){
        return "Error: El disco '"+path+"' no existe";
    }

    //Borrar el archivo (disco virtual)
    if(std::remove(path.c_str()) != 0){
        return "Error: No se puede borrar el disco '"+path+"'";
    }

    //Mensaje de éxito (RECORDATORIO: para el frontend)
    return "RMDISK: Disco borrado exitosamente en "+path;
}