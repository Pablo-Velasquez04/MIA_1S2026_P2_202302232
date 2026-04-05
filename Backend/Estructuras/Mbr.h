//Mbr.h
#pragma once
#include "Structs.h"
#include <vector>

MBR crearMBR(int tamanoDisco, char fit);
void inicializarParticiones(MBR& mbr);
std::vector<char> mbrToBytes(const MBR& mbr);
int getMBRSize();