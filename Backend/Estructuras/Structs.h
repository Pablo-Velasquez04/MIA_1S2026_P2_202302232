//Estructuras/Structs.h
#pragma once
#pragma pack(push, 1)
#include <ctime>

//-------- ESTRUCTURAS DE DISCO ---------------------------------------

struct Partition {
    char   Status;           //0 = no montada, 1 = montada
    char   Type;             //P = Primaria, E = Extendida
    char   Fit;              //B = Best, F = First, W = Worst
    int    Start;            //Byte de inicio en el disco
    int    Size;             //Tamaño en bytes
    char   Name[16];         //Nombre de la partición
    int    Correlative;      //-1 hasta ser montada
    char   Id[4];            //ID generado al montar
};

struct MBR {
    int       MbrSize;          //Tamaño total del disco
    time_t    CreationDate;     //Fecha de creación
    int       Signature;        //Número aleatorio único
    char      Fit;              //B, F, W
    Partition Partitions[4];    //Las 4 particiones
};

struct EBR {
    char   Mount;            //0 = no montada, 1 = montada
    char   Fit;              //B, F, W
    int    Start;            //Byte de inicio
    int    Size;             //Tamaño en bytes
    int    Next;             //Byte del siguiente EBR, -1 si no hay
    char   Name[16];         //Nombre
};


//--------- ESTRUCTURAS EXT3 --------------------------------------

struct Journaling {
    char   Operation[10];    // "mkdir", "mkfile", etc.
    char   Path[20];         // Ruta del archivo/carpeta
    char   Content[12];      // Contenido (truncado para ajuste de 50 bytes)
    time_t Date;             // Fecha de la operación
};


//--------- ESTRUCTURAS EXT2 --------------------------------------

struct SuperBloque {
    int    FilesystemType;    //2 = EXT2
    int    InodesCount;       //Total de inodos
    int    BlocksCount;       //Total de bloques
    int    FreeBlocksCount;   //Bloques libres
    int    FreeInodesCount;   //Inodos libres
    time_t Mtime;             //Último montaje
    time_t Umtime;            //Último desmontaje
    int    MntCount;          //Veces montado
    int    Magic;             //0xEF53
    int    InodeSize;         //sizeof(Inode)
    int    BlockSize;         //sizeof(Block) = 64
    int    FirstIno;          //Primer inodo libre
    int    FirstBlo;          //Primer bloque libre
    int    BmInodeStart;      //Inicio bitmap inodos
    int    BmBlockStart;      //Inicio bitmap bloques
    int    InodeStart;        //Inicio tabla inodos
    int    BlockStart;        //Inicio tabla bloques
};

struct Inode {
    int    Uid;              //UID del propietario
    int    Gid;              //GID del grupo
    int    Size;             //Tamaño en bytes
    time_t Atime;            //Último acceso
    time_t Ctime;            //Creación
    time_t Mtime;            //Última modificación
    int    Block[15];        //12 directos + 3 indirectos
    char   Type;             //1 = archivo, 0 = carpeta
    char   Perm[3];          //Permisos UGO
};

struct Content {
    char   Name[12];         //Nombre archivo/carpeta
    int    Inodo;            //Apuntador al inodo
};

struct FolderBlock {
    Content content[4];      //4 entradas = 64 bytes
};

struct FileBlock {
    char content[64];        //64 bytes de contenido
};

struct PointerBlock {
    int pointers[16];        //16 apuntadores = 64 bytes
};

#pragma pack(pop)