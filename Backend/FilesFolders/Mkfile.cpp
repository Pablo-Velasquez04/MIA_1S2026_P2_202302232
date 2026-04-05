// FilesFolders/Mkfile.cpp
#include "FilesFolders.h"
#include "../Utilidades/Utilities.h"
#include <sstream>
#include <fstream>
#include <cstring>
#include <ctime>
#include <filesystem>

namespace fs = std::filesystem;

extern ActiveSession currentSession;

//Generar contenido "0123456789012345..." de tamaño 'size'
static std::string generateContent(int size) {
    std::string content;
    content.reserve(size);
    for (int i = 0; i < size; i++)
        content += (char)('0' + (i % 10));
    return content;
}

//Escribir contenido en bloques del inodo (solo directos por ahora)
static bool writeFileContent(std::fstream& disk, SuperBloque& sb,
                              Inode& fileInode, int inodeIdx,
                              const std::string& content) {
    int blockSize   = sb.BlockSize;
    int contentSize = (int)content.size();
    int blocksNeeded = contentSize == 0 ? 0 :
                       (contentSize + blockSize - 1) / blockSize;

    int blockIndex = 0; //índice lógico del bloque de datos (0, 1, 2, ...)

    //Bloques DIRECTOS [0..11]
    for (int i = 0; i < 12 && blockIndex < blocksNeeded; i++, blockIndex++) {
        int newBlock = ffAllocBlock(disk, sb);
        if (newBlock == -1) return false;

        FileBlock fb{};
        memset(fb.content, 0, blockSize);
        int offset = blockIndex * blockSize;
        int toCopy = std::min(blockSize, contentSize - offset);
        memcpy(fb.content, content.c_str() + offset, toCopy);
        ffWriteFileBlock(disk, sb, newBlock, fb);

        fileInode.Block[i] = newBlock;
    }

    //Bloque SIMPLE INDIRECTO [12]
    if (blockIndex < blocksNeeded) {
        //Reservar bloque apuntador
        int ptrBlock = ffAllocBlock(disk, sb);
        if (ptrBlock == -1) return false;
        fileInode.Block[12] = ptrBlock;

        PointerBlock pb{};
        for (int k = 0; k < 16; k++) pb.pointers[k] = -1;

        for (int i = 0; i < 16 && blockIndex < blocksNeeded; i++, blockIndex++) {
            int newBlock = ffAllocBlock(disk, sb);
            if (newBlock == -1) return false;

            FileBlock fb{};
            memset(fb.content, 0, blockSize);
            int offset = blockIndex * blockSize;
            int toCopy = std::min(blockSize, contentSize - offset);
            memcpy(fb.content, content.c_str() + offset, toCopy);
            ffWriteFileBlock(disk, sb, newBlock, fb);

            pb.pointers[i] = newBlock;
        }
        // Escribir bloque apuntador en disco
        ffWritePointerBlock(disk, sb, ptrBlock, pb);
    }

    //Bloque DOBLE INDIRECTO 
    if (blockIndex < blocksNeeded) {
        int ptrBlock1 = ffAllocBlock(disk, sb);
        if (ptrBlock1 == -1) return false;
        fileInode.Block[13] = ptrBlock1;

        PointerBlock pb1{};
        for (int k = 0; k < 16; k++) pb1.pointers[k] = -1;

        for (int i = 0; i < 16 && blockIndex < blocksNeeded; i++) {
            int ptrBlock2 = ffAllocBlock(disk, sb);
            if (ptrBlock2 == -1) return false;
            pb1.pointers[i] = ptrBlock2;

            PointerBlock pb2{};
            for (int k = 0; k < 16; k++) pb2.pointers[k] = -1;

            for (int j = 0; j < 16 && blockIndex < blocksNeeded; j++, blockIndex++) {
                int newBlock = ffAllocBlock(disk, sb);
                if (newBlock == -1) return false;

                FileBlock fb{};
                memset(fb.content, 0, blockSize);
                int offset = blockIndex * blockSize;
                int toCopy = std::min(blockSize, contentSize - offset);
                memcpy(fb.content, content.c_str() + offset, toCopy);
                ffWriteFileBlock(disk, sb, newBlock, fb);

                pb2.pointers[j] = newBlock;
            }
            ffWritePointerBlock(disk, sb, ptrBlock2, pb2);
        }
        ffWritePointerBlock(disk, sb, ptrBlock1, pb1);
    }

    // Bloque TRIPLE INDIRECTO [14] 
    if (blockIndex < blocksNeeded) {
        int ptrBlock1 = ffAllocBlock(disk, sb);
        if (ptrBlock1 == -1) return false;
        fileInode.Block[14] = ptrBlock1;

        PointerBlock pb1{};
        for (int k = 0; k < 16; k++) pb1.pointers[k] = -1;

        for (int i = 0; i < 16 && blockIndex < blocksNeeded; i++) {
            int ptrBlock2 = ffAllocBlock(disk, sb);
            if (ptrBlock2 == -1) return false;
            pb1.pointers[i] = ptrBlock2;

            PointerBlock pb2{};
            for (int k = 0; k < 16; k++) pb2.pointers[k] = -1;

            for (int j = 0; j < 16 && blockIndex < blocksNeeded; j++) {
                int ptrBlock3 = ffAllocBlock(disk, sb);
                if (ptrBlock3 == -1) return false;
                pb2.pointers[j] = ptrBlock3;

                PointerBlock pb3{};
                for (int k = 0; k < 16; k++) pb3.pointers[k] = -1;

                for (int l = 0; l < 16 && blockIndex < blocksNeeded; l++, blockIndex++) {
                    int newBlock = ffAllocBlock(disk, sb);
                    if (newBlock == -1) return false;

                    FileBlock fb{};
                    memset(fb.content, 0, blockSize);
                    int offset = blockIndex * blockSize;
                    int toCopy = std::min(blockSize, contentSize - offset);
                    memcpy(fb.content, content.c_str() + offset, toCopy);
                    ffWriteFileBlock(disk, sb, newBlock, fb);

                    pb3.pointers[l] = newBlock;
                }
                ffWritePointerBlock(disk, sb, ptrBlock3, pb3);
            }
            ffWritePointerBlock(disk, sb, ptrBlock2, pb2);
        }
        ffWritePointerBlock(disk, sb, ptrBlock1, pb1);
    }

    fileInode.Size = contentSize;
    return true;
}

std::string cmdMkfile(std::map<std::string, std::string> params) {

    // 1. SESIÓN
    if (!currentSession.active)
        return "Error: No hay sesión activa. Ejecute login primero.\n";

    // 2. PARÁMETROS
    std::string path = params.count("path") ? params["path"] : "";
    if (path.empty())
        return "Error: Falta parámetro -path.\n";

    // -r: si existe en el mapa, está activo
    bool recursive = params.count("r") > 0;

    // -size
    int fileSize = 0;
    if (params.count("size")) {
        try {
            fileSize = std::stoi(params["size"]);
        } catch (...) {
            return "Error: Valor inválido para -size.\n";
        }
        if (fileSize < 0)
            return "Error: El tamaño no puede ser negativo.\n";
    }

    // -cont tiene prioridad sobre -size
    std::string contPath = params.count("cont") ? params["cont"] : "";

    // 3. LEER CONTENIDO
    std::string fileContent;

    if (!contPath.empty()) {
        //Leer archivo del disco real de la computadora
        if (!archivoExiste(contPath))
            return "Error: El archivo '" + contPath + "' no existe.\n";

        std::ifstream contFile(contPath, std::ios::binary);
        if (!contFile.is_open())
            return "Error: No se pudo abrir '" + contPath + "'.\n";

        fileContent = std::string(
            std::istreambuf_iterator<char>(contFile),
            std::istreambuf_iterator<char>()
        );
        contFile.close();
    } else {
        //Generar contenido numérico 012345678901234...
        fileContent = generateContent(fileSize);
    }

    // 4. ABRIR DISCO
    std::fstream disk(currentSession.diskPath,
                      std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open())
        return "Error: No se pudo abrir el disco.\n";

    SuperBloque sb{};
    if (!ffReadSuperblock(disk, sb)) {
        disk.close();
        return "Error: No se pudo leer el superbloque.\n";
    }

    // 5. SEPARAR RUTA PADRE Y NOMBRE DEL ARCHIVO
    auto parts = ffSplitPath(path);
    if (parts.empty()) {
        disk.close();
        return "Error: Ruta inválida.\n";
    }

    std::string fileName = parts.back();

    std::string parentPath = "/";
    for (size_t i = 0; i + 1 < parts.size(); i++)
        parentPath += parts[i] + (i + 2 < parts.size() ? "/" : "");

    // 6. RESOLVER CARPETA PADRE
    int parentInodeIdx = 0;
    if (parts.size() > 1) {
        parentInodeIdx = ffResolvePath(disk, sb, parentPath,
                                       recursive,
                                       currentSession.uid,
                                       currentSession.gid);
        if (parentInodeIdx == -2) {
            disk.close();
            return "Error: Sin permiso de escritura en la carpeta padre.\n";
        }
        if (parentInodeIdx == -1) {
            disk.close();
            return "Error: Las carpetas padres '" + parentPath +
                   "' no existen. Use -r para crearlas.\n";
        }
    }

    // 7. VERIFICAR PERMISO DE ESCRITURA EN PADRE
    Inode parentInode{};
    ffReadInode(disk, sb, parentInodeIdx, parentInode);
    if (!ffHasPermission(parentInode, currentSession.uid,
                         currentSession.gid, 'w')) {
        disk.close();
        return "Error: Sin permiso de escritura en '" + parentPath + "'.\n";
    }

    // 8. VERIFICAR SI YA EXISTE
    int existing = ffLookup(disk, sb, parentInodeIdx, fileName);
    if (existing != -1) {
        disk.close();
        // Enunciado: mostrar mensaje si se desea sobreescribir
        return "Advertencia: El archivo '" + fileName +
               "' ya existe. Si desea sobreescribirlo elimínelo primero.\n";
    }

    // 9. ASIGNAR INODO
    int newInodeIdx = ffAllocInode(disk, sb);
    if (newInodeIdx == -1) {
        disk.close();
        return "Error: No hay inodos libres.\n";
    }

    // 10. CREAR INODO ARCHIVO
    time_t now = time(nullptr);
    Inode newInode{};
    newInode.Uid     = currentSession.uid;
    newInode.Gid     = currentSession.gid;
    newInode.Size    = 0;
    newInode.Atime   = now;
    newInode.Ctime   = now;
    newInode.Mtime   = now;
    newInode.Type    = '1'; //archivo
    newInode.Perm[0] = '6'; //permisos 664 según enunciado
    newInode.Perm[1] = '6';
    newInode.Perm[2] = '4';
    for (int k = 0; k < 15; k++) newInode.Block[k] = -1;

    // 11. ESCRIBIR CONTENIDO EN BLOQUES
    if (!fileContent.empty()) {
        if (!writeFileContent(disk, sb, newInode, newInodeIdx, fileContent)) {
            disk.close();
            return "Error: No hay bloques libres suficientes.\n";
        }
    }

    ffWriteInode(disk, sb, newInodeIdx, newInode);

    // 12. AGREGAR ENTRADA EN CARPETA PADRE
    if (!ffAddEntry(disk, sb, parentInodeIdx, fileName, newInodeIdx)) {
        disk.close();
        return "Error: No se pudo agregar entrada en la carpeta padre.\n";
    }

    disk.close();

    // Sincronización con el sistema de archivos real
    try {
        std::string realPath = "." + path; // Hacerlo relativo a la raíz del proyecto
        fs::create_directories(fs::path(realPath).parent_path());
        std::ofstream realFile(realPath, std::ios::binary);
        if (realFile.is_open()) {
            realFile << fileContent;
            realFile.close();
        }
    } catch (...) {
        // Fallar silenciosamente
    }

    std::ostringstream out;
    out << "Archivo '" << path << "' creado exitosamente.\n";
    out << "  Tamaño  : " << (int)fileContent.size() << " bytes\n";
    out << "  Inodo   : " << newInodeIdx << "\n";
    out << "  Permisos: 664\n";
    return out.str();
}