"use client";

import {
  Accordion,
  AccordionContent,
  AccordionItem,
  AccordionTrigger,
} from "@/components/ui/accordion";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { BookOpen } from "lucide-react";

const commandGroups = [
  {
    name: "Administracion de Discos",
    commands: [
      {
        name: "MKDISK",
        description: "Crear un nuevo disco",
        params: [
          { name: "-size", required: true, desc: "Tamano del disco" },
          { name: "-unit", required: false, desc: "Unidad (K/M), default M" },
          { name: "-fit", required: false, desc: "Ajuste (BF/FF/WF), default FF" },
          { name: "-path", required: true, desc: "Ruta del archivo .mia" },
        ],
        example: 'mkdisk -size=50 -unit=M -path="/home/disco.mia"',
      },
      {
        name: "RMDISK",
        description: "Eliminar un disco",
        params: [{ name: "-path", required: true, desc: "Ruta del disco" }],
        example: 'rmdisk -path="/home/disco.mia"',
      },
      {
        name: "FDISK",
        description: "Administrar particiones",
        params: [
          { name: "-size", required: false, desc: "Tamano de la particion" },
          { name: "-unit", required: false, desc: "Unidad (B/K/M)" },
          { name: "-path", required: true, desc: "Ruta del disco" },
          { name: "-type", required: false, desc: "Tipo (P/E/L)" },
          { name: "-fit", required: false, desc: "Ajuste (BF/FF/WF)" },
          { name: "-name", required: true, desc: "Nombre de particion" },
          { name: "-delete", required: false, desc: "Eliminar particion (full)" },
          { name: "-add", required: false, desc: "Agregar/quitar espacio" },
        ],
        example: 'fdisk -size=10 -unit=M -path="/home/disco.mia" -name="Part1"',
      },
      {
        name: "MOUNT",
        description: "Montar una particion",
        params: [
          { name: "-path", required: true, desc: "Ruta del disco" },
          { name: "-name", required: true, desc: "Nombre de particion" },
        ],
        example: 'mount -path="/home/disco.mia" -name="Part1"',
      },
      {
        name: "MOUNTED",
        description: "Mostrar particiones montadas",
        params: [],
        example: "mounted",
      },
    ],
  },
  {
    name: "Sistema de Archivos",
    commands: [
      {
        name: "MKFS",
        description: "Formatear particion como EXT2",
        params: [
          { name: "-id", required: true, desc: "ID de particion montada" },
          { name: "-type", required: false, desc: "Tipo de formato (full)" },
        ],
        example: "mkfs -id=641A -type=full",
      },
      {
        name: "CAT",
        description: "Mostrar contenido de archivos",
        params: [
          { name: "-file1", required: true, desc: "Ruta del archivo" },
          { name: "-fileN", required: false, desc: "Archivos adicionales" },
        ],
        example: 'cat -file1="/home/user/archivo.txt"',
      },
    ],
  },
  {
    name: "Usuarios y Grupos",
    commands: [
      {
        name: "LOGIN",
        description: "Iniciar sesion",
        params: [
          { name: "-user", required: true, desc: "Nombre de usuario" },
          { name: "-pass", required: true, desc: "Contrasena" },
          { name: "-id", required: true, desc: "ID de particion" },
        ],
        example: "login -user=root -pass=123 -id=641A",
      },
      {
        name: "LOGOUT",
        description: "Cerrar sesion",
        params: [],
        example: "logout",
      },
      {
        name: "MKGRP",
        description: "Crear grupo (solo root)",
        params: [{ name: "-name", required: true, desc: "Nombre del grupo" }],
        example: 'mkgrp -name="usuarios"',
      },
      {
        name: "RMGRP",
        description: "Eliminar grupo (solo root)",
        params: [{ name: "-name", required: true, desc: "Nombre del grupo" }],
        example: 'rmgrp -name="usuarios"',
      },
      {
        name: "MKUSR",
        description: "Crear usuario (solo root)",
        params: [
          { name: "-user", required: true, desc: "Nombre de usuario" },
          { name: "-pass", required: true, desc: "Contrasena" },
          { name: "-grp", required: true, desc: "Grupo del usuario" },
        ],
        example: 'mkusr -user="juan" -pass="abc" -grp="usuarios"',
      },
      {
        name: "RMUSR",
        description: "Eliminar usuario (solo root)",
        params: [{ name: "-user", required: true, desc: "Nombre de usuario" }],
        example: 'rmusr -user="juan"',
      },
      {
        name: "CHGRP",
        description: "Cambiar grupo de usuario",
        params: [
          { name: "-user", required: true, desc: "Nombre de usuario" },
          { name: "-grp", required: true, desc: "Nuevo grupo" },
        ],
        example: 'chgrp -user="juan" -grp="admin"',
      },
    ],
  },
  {
    name: "Archivos y Carpetas",
    commands: [
      {
        name: "MKFILE",
        description: "Crear archivo",
        params: [
          { name: "-path", required: true, desc: "Ruta del archivo" },
          { name: "-r", required: false, desc: "Crear carpetas recursivamente" },
          { name: "-size", required: false, desc: "Tamano en bytes" },
          { name: "-cont", required: false, desc: "Ruta de contenido" },
        ],
        example: 'mkfile -path="/home/user/file.txt" -size=100',
      },
      {
        name: "MKDIR",
        description: "Crear carpeta",
        params: [
          { name: "-path", required: true, desc: "Ruta de la carpeta" },
          { name: "-p", required: false, desc: "Crear padres recursivamente" },
        ],
        example: 'mkdir -path="/home/user/nueva" -p',
      },
    ],
  },
  {
    name: "Reportes",
    commands: [
      {
        name: "REP",
        description: "Generar reportes con Graphviz",
        params: [
          { name: "-id", required: true, desc: "ID de particion" },
          { name: "-path", required: true, desc: "Ruta del reporte" },
          { name: "-name", required: true, desc: "Tipo de reporte" },
          { name: "-ruta", required: false, desc: "Ruta para reporte file/ls" },
        ],
        example: 'rep -id=641A -path="/home/reporte.png" -name=disk',
      },
    ],
  },
];

export function CommandReference() {
  return (
    <Card>
      <CardHeader className="pb-3">
        <CardTitle className="flex items-center gap-2 text-lg">
          <BookOpen className="h-5 w-5" />
          Referencia de Comandos
        </CardTitle>
      </CardHeader>
      <CardContent className="pt-0">
        <Accordion type="single" collapsible className="w-full">
          {commandGroups.map((group, groupIndex) => (
            <AccordionItem key={groupIndex} value={`group-${groupIndex}`}>
              <AccordionTrigger className="text-sm">
                {group.name}
              </AccordionTrigger>
              <AccordionContent>
                <div className="space-y-4">
                  {group.commands.map((cmd, cmdIndex) => (
                    <div
                      key={cmdIndex}
                      className="border rounded-lg p-3 bg-muted/30"
                    >
                      <div className="flex items-center gap-2 mb-2">
                        <Badge variant="secondary" className="font-mono">
                          {cmd.name}
                        </Badge>
                        <span className="text-sm text-muted-foreground">
                          {cmd.description}
                        </span>
                      </div>
                      {cmd.params.length > 0 && (
                        <div className="mb-2">
                          <p className="text-xs text-muted-foreground mb-1">
                            Parametros:
                          </p>
                          <div className="flex flex-wrap gap-1">
                            {cmd.params.map((param, paramIndex) => (
                              <Badge
                                key={paramIndex}
                                variant={param.required ? "default" : "outline"}
                                className="text-xs"
                              >
                                {param.name}
                                {param.required && "*"}
                              </Badge>
                            ))}
                          </div>
                        </div>
                      )}
                      <code className="text-xs bg-muted px-2 py-1 rounded block overflow-x-auto">
                        {cmd.example}
                      </code>
                    </div>
                  ))}
                </div>
              </AccordionContent>
            </AccordionItem>
          ))}
        </Accordion>
      </CardContent>
    </Card>
  );
}
