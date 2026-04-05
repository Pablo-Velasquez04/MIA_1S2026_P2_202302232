// Estructuras/MBR.cpp
#include "Mbr.h"
#include <ctime>      // para std::time
#include <cstring>    // para memset
#include <vector>     // para std::vector
#include <cstdlib>    // para rand()


//Función para crear un MBR con los datos necesarios
MBR crearMBR(int tamanoDisco, char fit) {
    MBR mbr = {};                     // todo en ceros

    mbr.MbrSize      = tamanoDisco;
    mbr.CreationDate = std::time(nullptr);
    mbr.Signature    = rand();
    mbr.Fit          = fit;

    inicializarParticiones(mbr);
    return mbr;
}

//Función para inicializar las 4 particiones del MBR con valores por defecto
void inicializarParticiones(MBR& mbr) {
    for (int i = 0; i < 4; i++) {
        mbr.Partitions[i].Status      = '0';
        mbr.Partitions[i].Type        = '0';
        mbr.Partitions[i].Fit         = '0';
        mbr.Partitions[i].Start       = -1;
        mbr.Partitions[i].Size        = -1;
        mbr.Partitions[i].Correlative = -1;

        memset(mbr.Partitions[i].Name, 0, sizeof(mbr.Partitions[i].Name));
        memset(mbr.Partitions[i].Id,   0, sizeof(mbr.Partitions[i].Id));
    }
}


//Función para convertir un MBR a un vector de bytes para escribirlo en disco y una función para obtener el tamaño del MBR
std::vector<char> mbrToBytes(const MBR& mbr) {
    std::vector<char> bytes(sizeof(MBR));
    std::memcpy(bytes.data(), &mbr, sizeof(MBR));
    return bytes;
}

//Función para obtener el tamaño del MBR
int getMBRSize() {
    return sizeof(MBR);
}