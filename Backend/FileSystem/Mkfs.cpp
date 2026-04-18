//FileSystem/Mkfs.cpp
#include "FileSystem.h"
#include "../Estructuras/Structs.h"
#include "../Mounting/Mounting.h"
#include "../Utilidades/Utilities.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <ctime>

//HELPERS

// No local diskRead/diskWrite - using Utilities.h

//Escribir un byte en el bitmap (0 = libre, 1 = ocupado)
static void setBit(std::fstream& disk, long bitmapStart, int index, char value) {
    long pos = bitmapStart + index;
    disk.seekp(pos);
    disk.write(&value, 1);
}

//// MKFS
// Parámetros: -id (obligatorio), -type (opcional, default Full), -fs (opcional, default 2fs)
std::string cmdMkfs(std::map<std::string, std::string> params) {
    std::ostringstream out;

    // 1. PARÁMETROS 
    std::string id   = params.count("id")   ? params["id"]   : "";
    std::string type = params.count("type") ? params["type"] : "full";
    std::string fs   = params.count("fs")   ? params["fs"]   : "2fs";

    if (id.empty())
        return "Error: Falta parámetro -id\n";

    // Convertir a minúsculas
    for (auto& c : type) c = tolower(c);
    for (auto& c : fs)   c = tolower(c);

    if (type != "full")
        return "Error: -type solo admite 'full'\n";
    
    if (fs != "2fs" && fs != "3fs")
        return "Error: -fs solo admite '2fs' o '3fs'\n";

    // 2. BUSCAR PARTICIÓN MONTADA 
    const MountedPartition* mp = getMountedById(id);
    if (!mp)
        return "Error: No existe partición montada con id '" + id + "'\n";

    if (!archivoExiste(mp->Path))
        return "Error: El disco '" + mp->Path + "' no existe\n";

    // 3. ABRIR DISCO 
    std::fstream disk(mp->Path, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk.is_open())
        return "Error: No se pudo abrir el disco '" + mp->Path + "'\n";

    long partStart = mp->Start;
    long partSize  = mp->Size;

    //  4. CALCULAR N (número de inodos) 
    /* 
        Fórmulas del enunciado:
        EXT2 (n) = (partSize - sizeof(SuperBloque)) / (4 + sizeof(Inode) + 3*64)
        EXT3 (n) = (partSize - sizeof(SuperBloque)) / (sizeof(Journaling) + 4 + sizeof(Inode) + 3*64)
    */

    long sbSize      = sizeof(SuperBloque);
    long journalSize = sizeof(Journaling);
    long inodeSize   = sizeof(Inode);
    long blockSize   = sizeof(FolderBlock); // = 64

    long n;
    if (fs == "2fs") {
        n = (long)floor(
            (double)(partSize - sbSize) /
            (double)(4 + inodeSize + 3 * blockSize)
        );
    } else { // 3fs
        n = (long)floor(
            (double)(partSize - sbSize) /
            (double)(journalSize + 4 + inodeSize + 3 * blockSize)
        );
    }

    if (n <= 0) {
        disk.close();
        return "Error: Partición demasiado pequeña para el sistema de archivos solicitado\n";
    }

    long numInodes = n;
    long numBlocks = 3 * n;

    // 5. CALCULAR POSICIONES DE CADA ESTRUCTURA 
    // Layout EXT2: [SuperBloque][BitmapInodos][BitmapBloques][Inodos][Bloques]
    // Layout EXT3: [SuperBloque][Journaling][BitmapInodos][BitmapBloques][Inodos][Bloques]
    
    long sbPos      = partStart;
    long journalPos = -1;
    long bmInodePos;
    
    if (fs == "2fs") {
        bmInodePos = sbPos + sbSize;
    } else {
        journalPos = sbPos + sbSize;
        bmInodePos = journalPos + numInodes * journalSize;
    }

    long bmBlockPos = bmInodePos + numInodes;      // 1 byte por inodo
    long inodePos   = bmBlockPos + numBlocks;      // 1 byte por bloque
    long blockPos   = inodePos   + numInodes * inodeSize;

    // 6. INICIALIZAR BITMAPS CON CEROS (todo libre) 
    {
        char zero = '0';
        // Bitmap inodos
        for (long i = 0; i < numInodes; i++) {
            disk.seekp(bmInodePos + i);
            disk.write(&zero, 1);
        }
        // Bitmap bloques
        for (long i = 0; i < numBlocks; i++) {
            disk.seekp(bmBlockPos + i);
            disk.write(&zero, 1);
        }
    }

    // 7. INICIALIZAR JOURNALING (Solo si es EXT3)
    if (fs == "3fs") {
        Journaling emptyJournal{};
        memset(emptyJournal.Operation, 0, 10);
        memset(emptyJournal.Path, 0, 20);
        memset(emptyJournal.Content, 0, 12);
        emptyJournal.Date = 0;

        for (long i = 0; i < numInodes; i++) {
            escribirStruct(disk, journalPos + i * journalSize, emptyJournal);
        }
    }

    // 8. INICIALIZAR TABLA DE INODOS CON -1
    {
        Inode emptyInode{};
        emptyInode.Uid  = -1;
        emptyInode.Gid  = -1;
        emptyInode.Size = -1;
        emptyInode.Type = '-';
        for (int i = 0; i < 15; i++)
            emptyInode.Block[i] = -1;

        for (long i = 0; i < numInodes; i++)
            escribirStruct(disk, inodePos + i * inodeSize, emptyInode);
    }

    // 9. INICIALIZAR TABLA DE BLOQUES CON -1
    {
        FolderBlock emptyBlock{};
        for (int i = 0; i < 4; i++) {
            emptyBlock.content[i].Inodo = -1;
            memset(emptyBlock.content[i].Name, 0, 12);
        }
        for (long i = 0; i < numBlocks; i++)
            escribirStruct(disk, blockPos + i * blockSize, emptyBlock);
    }

    // 10. CREAR INODO 0 — CARPETA RAÍZ "/"
    time_t now = time(nullptr);

    Inode rootInode{};
    rootInode.Uid   = 1;        // root
    rootInode.Gid   = 1;        // root
    rootInode.Size  = blockSize;
    rootInode.Atime = now;
    rootInode.Ctime = now;
    rootInode.Mtime = now;
    rootInode.Type  = '0';      // 0 = carpeta
    rootInode.Perm[0] = '7';
    rootInode.Perm[1] = '7';
    rootInode.Perm[2] = '7';
    for (int i = 0; i < 15; i++) rootInode.Block[i] = -1;
    rootInode.Block[0] = 0;     // apunta al bloque 0

    escribirStruct(disk, inodePos, rootInode);
    setBit(disk, bmInodePos, 0, '1'); // inodo 0 ocupado

    // 11. CREAR BLOQUE 0 — CONTENIDO CARPETA RAÍZ
    FolderBlock rootBlock{};
    for (int i = 0; i < 4; i++) {
        rootBlock.content[i].Inodo = -1;
        memset(rootBlock.content[i].Name, 0, 12);
    }
    strncpy(rootBlock.content[0].Name, ".",  11);
    rootBlock.content[0].Inodo = 0;
    strncpy(rootBlock.content[1].Name, "..", 11);
    rootBlock.content[1].Inodo = 0;
    strncpy(rootBlock.content[2].Name, "users.txt", 11);
    rootBlock.content[2].Inodo = 1;

    escribirStruct(disk, blockPos, rootBlock);
    setBit(disk, bmBlockPos, 0, '1'); // bloque 0 ocupado

    // 12. CREAR INODO 1 — ARCHIVO users.txt 
    std::string usersContent = "1,G,root\n1,U,root,root,123\n";
    int usersSize = (int)usersContent.size();

    Inode usersInode{};
    usersInode.Uid   = 1;       // root
    usersInode.Gid   = 1;       // root
    usersInode.Size  = usersSize;
    usersInode.Atime = now;
    usersInode.Ctime = now;
    usersInode.Mtime = now;
    usersInode.Type  = '1';     // 1 = archivo
    usersInode.Perm[0] = '6';
    usersInode.Perm[1] = '6';
    usersInode.Perm[2] = '4';
    for (int i = 0; i < 15; i++) usersInode.Block[i] = -1;
    usersInode.Block[0] = 1;    // apunta al bloque 1

    escribirStruct(disk, inodePos + inodeSize, usersInode);
    setBit(disk, bmInodePos, 1, '1'); // inodo 1 ocupado

    // 13. CREAR BLOQUE 1 — CONTENIDO users.txt 
    FileBlock usersBlock{};
    memset(usersBlock.content, 0, 64);
    memcpy(usersBlock.content, usersContent.c_str(),
           std::min((int)usersContent.size(), 64));

    escribirStruct(disk, blockPos + blockSize, usersBlock);
    setBit(disk, bmBlockPos, 1, '1'); // bloque 1 ocupado

    // 14. ESCRIBIR SUPERBLOQUE 
    SuperBloque sb{};
    sb.FilesystemType  = (fs == "2fs") ? 2 : 3;
    sb.InodesCount     = (int)numInodes;
    sb.BlocksCount     = (int)numBlocks;
    sb.FreeInodesCount = (int)numInodes - 2;  // 0 y 1 usados
    sb.FreeBlocksCount = (int)numBlocks - 2;  // 0 y 1 usados
    sb.Mtime           = now;
    sb.Umtime          = 0;
    sb.MntCount        = 1;
    sb.Magic           = 0xEF53;
    sb.InodeSize       = (int)inodeSize;
    sb.BlockSize       = (int)blockSize;
    sb.FirstIno        = 2;  // primer inodo libre
    sb.FirstBlo        = 2;  // primer bloque libre
    sb.BmInodeStart    = (int)bmInodePos;
    sb.BmBlockStart    = (int)bmBlockPos;
    sb.InodeStart      = (int)inodePos;
    sb.BlockStart      = (int)blockPos;

    escribirStruct(disk, sbPos, sb);

    disk.close();

    // 15. SALIDA 
    out << "=== MKFS EXITOSO (" << (fs == "2fs" ? "EXT2" : "EXT3") << ") ===\n";
    out << "  Partición     : " << mp->Name   << " (" << id << ")\n";
    out << "  Disco         : " << mp->Path   << "\n";
    out << "  Inodos        : " << numInodes  << "\n";
    out << "  Bloques       : " << numBlocks  << "\n";
    out << "  Inodos libres : " << sb.FreeInodesCount << "\n";
    out << "  Bloques libres: " << sb.FreeBlocksCount << "\n";
    if (fs == "3fs")
    out << "  Inicio Journal: byte " << journalPos << "\n";
    out << "  BM Inodos     : byte " << bmInodePos << "\n";
    out << "  BM Bloques    : byte " << bmBlockPos << "\n";
    out << "  Inicio inodos : byte " << inodePos   << "\n";
    out << "  Inicio bloques: byte " << blockPos   << "\n";
    out << "  users.txt     : creado con root/123\n";

    return out.str();
}