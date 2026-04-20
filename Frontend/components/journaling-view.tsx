"use client";

import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from "@/components/ui/table";
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from "@/components/ui/card";
import { ScrollArea } from "@/components/ui/scroll-area";
import { Activity } from "lucide-react";

export interface JournalEntry {
  operation: string;
  path: string;
  content: string;
  date: string;
}

interface JournalingViewProps {
  entries: JournalEntry[];
}

export function JournalingView({ entries }: JournalingViewProps) {
  return (
    <Card className="w-full">
      <CardHeader className="pb-3">
        <div className="flex items-center gap-2">
          <Activity className="h-5 w-5 text-primary" />
          <div>
            <CardTitle>Journaling (EXT3)</CardTitle>
            <CardDescription>
              Registro de transacciones realizadas en el sistema de archivos
            </CardDescription>
          </div>
        </div>
      </CardHeader>
      <CardContent>
        {entries.length === 0 ? (
          <div className="flex flex-col items-center justify-center py-10 text-center space-y-2">
            <p className="text-muted-foreground">No hay entradas de journaling registradas.</p>
            <p className="text-xs text-muted-foreground">
              Ejecuta el comando <code className="bg-muted px-1 rounded">journaling -id=...</code> para cargar los datos.
            </p>
          </div>
        ) : (
          <ScrollArea className="h-[400px] rounded-md border">
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead className="w-[100px]">Operación</TableHead>
                  <TableHead>Ruta</TableHead>
                  <TableHead>Contenido</TableHead>
                  <TableHead className="text-right">Fecha y Hora</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {entries.map((entry, index) => (
                  <TableRow key={index}>
                    <TableCell className="font-medium">
                      <span className="inline-flex items-center rounded-full bg-primary/10 px-2 py-1 text-xs font-medium text-primary">
                        {entry.operation}
                      </span>
                    </TableCell>
                    <TableCell className="max-w-[200px] truncate" title={entry.path}>
                      {entry.path}
                    </TableCell>
                    <TableCell className="max-w-[150px] truncate" title={entry.content}>
                      {entry.content}
                    </TableCell>
                    <TableCell className="text-right text-muted-foreground text-xs whitespace-nowrap">
                      {entry.date}
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </ScrollArea>
        )}
      </CardContent>
    </Card>
  );
}
