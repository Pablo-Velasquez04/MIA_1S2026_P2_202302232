#include "Ebr.h"
#include <cstring>    // memset, memcpy

// --------------------- creación ---------------------
EBR crearEBR() {
    EBR ebr = {};
    ebr.Mount = 0;
    ebr.Fit   = 0;
    ebr.Start = 0;
    ebr.Size  = 0;
    ebr.Next  = -1;
    // nombre ya inicializado a ceros por el brace-init
    return ebr;
}

EBR crearEBRConDatos(char fit, int start, int size, const std::string& name, int next) {
    EBR ebr = crearEBR();
    ebr.Fit   = fit;
    ebr.Start = start;
    ebr.Size  = size;
    ebr.Next  = next;
    setEBRName(ebr, name);
    return ebr;
}

// --------------------- conversion ---------------------
std::vector<char> ebrToBytes(const EBR& ebr) {
    std::vector<char> bytes(sizeof(EBR));
    std::memcpy(bytes.data(), &ebr, sizeof(EBR));
    return bytes;
}

int getEBRSize() {
    return sizeof(EBR);
}

// --------------------- I/O de disco ---------------------
// Nota: usar leerEstructuraDelArchivo / escribirEstructuraEnArchivo de Utilities.h
// Ejemplo: 
//   EBR ebr = crearEBRConDatos('F', 100, 5000, "logica1", -1);
//   std::vector<char> bytes = ebrToBytes(ebr);
//   escribirEstructuraEnArchivo(disco_path, bytes.data(), bytes.size());
//
//   EBR read_ebr;
//   leerEstructuraDelArchivo(disco_path, &read_ebr, sizeof(EBR), posicion);

// --------------------- helpers ---------------------

void setEBRName(EBR& ebr, const std::string& name) {
    std::memset(ebr.Name, 0, sizeof(ebr.Name));
    size_t len = name.size();
    if (len > sizeof(ebr.Name) - 1) len = sizeof(ebr.Name) - 1;
    std::memcpy(ebr.Name, name.c_str(), len);
}

std::string getEBRName(const EBR& ebr) {
    size_t len = 0;
    while (len < sizeof(ebr.Name) && ebr.Name[len] != 0) {
        len++;
    }
    return std::string(ebr.Name, len);
}

bool isEBREmpty(const EBR& ebr) {
    return ebr.Size == 0 && ebr.Start == 0;
}

bool hasNextEBR(const EBR& ebr) {
    return ebr.Next != -1 && ebr.Next > 0;
}
