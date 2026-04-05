#pragma once

#include "Structs.h"
#include <string>
#include <vector>

// Funciones para crear EBR
EBR crearEBR();
EBR crearEBRConDatos(char fit, int start, int size, const std::string& name, int next);

// Conversión a bytes (usar con escribirEstructuraEnArchivo de Utilities)
std::vector<char> ebrToBytes(const EBR& ebr);
int getEBRSize();

// Helpers específicos del EBR
void setEBRName(EBR& ebr, const std::string& name);
std::string getEBRName(const EBR& ebr);
bool isEBREmpty(const EBR& ebr);
bool hasNextEBR(const EBR& ebr);

