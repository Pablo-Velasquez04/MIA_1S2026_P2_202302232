//FilesFolders/Cat.cpp
#include "FilesFolders.h"
#include <sstream>
#include <cstring>

extern ActiveSession currentSession;

//HELPER — leer contenido completo de un archivo dado su inodo
//Recorre bloques directos (0-11) y retorna el contenido como string
static std::string readFileContent(std::fstream& disk,
                                    const SuperBloque& sb,
                                    const Inode& fileInode) {
    std::string content;
    content.reserve(fileInode.Size);

    //Bloques directos (0-11)
    for (int i = 0; i < 12; i++) {
        if (fileInode.Block[i] == -1) break;

        FileBlock fb{};
        if (!ffReadFileBlock(disk, sb, fileInode.Block[i], fb)) break;

        int remaining = fileInode.Size - (int)content.size();
        int toRead    = std::min(remaining, sb.BlockSize);
        if (toRead <= 0) break;

        content.append(fb.content, toRead);
    }

    return content;
}

//CAT
//Parámetros: -file1, -file2, ..., -fileN (al menos uno obligatorio)
//Muestra contenido de cada archivo separado por salto de línea
std::string cmdCat(std::map<std::string, std::string> params) {

    // 1. SESIÓN
    if (!currentSession.active)
        return "Error: No hay sesión activa. Ejecute login primero.\n";

    // 2. RECOLECTAR ARCHIVOS EN ORDEN (file1, file2, ..., fileN) 
    std::vector<std::string> files;
    int n = 1;
    while (true) {
        std::string key = "file" + std::to_string(n);
        if (!params.count(key)) break;
        files.push_back(params[key]);
        n++;
    }

    if (files.empty())
        return "Error: Falta al menos un parámetro -file1.\n";

    // 3. ABRIR DISCO
    std::fstream disk(currentSession.diskPath,
                      std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open())
        return "Error: No se pudo abrir el disco.\n";

    SuperBloque sb{};
    if (!ffReadSuperblock(disk, sb)) {
        disk.close();
        return "Error: No se pudo leer el superbloque.\n";
    }

    // 4. LEER Y CONCATENAR CADA ARCHIVO
    std::ostringstream out;

    for (size_t f = 0; f < files.size(); f++) {
        const std::string& filePath = files[f];

        //Resolver ruta — no crear padres
        int inodeIdx = ffResolvePath(disk, sb, filePath,
                                     false,
                                     currentSession.uid,
                                     currentSession.gid);

        if (inodeIdx == -1) {
            out << "Error: El archivo '" << filePath << "' no existe.\n";
            continue;
        }

        //Leer inodo
        Inode fileInode{};
        if (!ffReadInode(disk, sb, inodeIdx, fileInode)) {
            out << "Error: No se pudo leer el inodo de '"
                << filePath << "'.\n";
            continue;
        }

        //Verificar que es archivo (no carpeta)
        if (fileInode.Type != '1') {
            out << "Error: '" << filePath << "' es una carpeta, "
                << "no un archivo.\n";
            continue;
        }

        //Verificar permiso de lectura — UGO
        if (!ffHasPermission(fileInode,
                             currentSession.uid,
                             currentSession.gid, 'r')) {
            out << "Error: Sin permiso de lectura en '"
                << filePath << "'.\n";
            continue;
        }

        //Actualizar Atime del inodo
        fileInode.Atime = time(nullptr);
        ffWriteInode(disk, sb, inodeIdx, fileInode);

        //Leer contenido
        std::string content = readFileContent(disk, sb, fileInode);

        //Agregar contenido — separado por salto de línea entre archivos
        out << content;
        if (f + 1 < files.size())
            out << "\n";
    }

    disk.close();
    return out.str();
}