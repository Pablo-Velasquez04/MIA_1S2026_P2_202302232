"use client";

import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { ScrollArea } from "@/components/ui/scroll-area";
import { HardDrive, Database } from "lucide-react";

export interface Partition {
  id: string;
  name: string;
  path: string;
  size: number;
  type: string;
  status: "mounted" | "unmounted";
}

interface MountedPartitionsProps {
  partitions: Partition[];
}

export function MountedPartitions({ partitions }: MountedPartitionsProps) {
  const formatSize = (bytes: number) => {
    if (bytes >= 1024 * 1024 * 1024) {
      return `${(bytes / (1024 * 1024 * 1024)).toFixed(2)} GB`;
    } else if (bytes >= 1024 * 1024) {
      return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
    } else if (bytes >= 1024) {
      return `${(bytes / 1024).toFixed(2)} KB`;
    }
    return `${bytes} B`;
  };

  return (
    <Card className="h-full">
      <CardHeader className="pb-3">
        <CardTitle className="flex items-center gap-2 text-lg">
          <HardDrive className="h-5 w-5" />
          Particiones Montadas
        </CardTitle>
      </CardHeader>
      <CardContent>
        <ScrollArea className="h-[200px]">
          {partitions.length === 0 ? (
            <div className="flex flex-col items-center justify-center h-full text-muted-foreground py-8">
              <Database className="h-8 w-8 mb-2 opacity-50" />
              <p className="text-sm">No hay particiones montadas</p>
              <p className="text-xs mt-1">Usa el comando MOUNT para montar una particion</p>
            </div>
          ) : (
            <div className="space-y-3">
              {partitions.map((partition) => (
                <div
                  key={partition.id}
                  className="flex items-center justify-between p-3 rounded-lg border bg-muted/30"
                >
                  <div className="flex items-center gap-3">
                    <div className="h-10 w-10 rounded-md bg-primary/10 flex items-center justify-center">
                      <Database className="h-5 w-5 text-primary" />
                    </div>
                    <div>
                      <div className="flex items-center gap-2">
                        <span className="font-medium">{partition.name}</span>
                        <Badge variant="secondary" className="text-xs">
                          {partition.id}
                        </Badge>
                      </div>
                      <p className="text-xs text-muted-foreground truncate max-w-[200px]">
                        {partition.path}
                      </p>
                    </div>
                  </div>
                  <div className="text-right">
                    <Badge
                      variant={partition.type === "P" ? "default" : "outline"}
                      className="mb-1"
                    >
                      {partition.type === "P" ? "Primaria" : partition.type === "E" ? "Extendida" : "Logica"}
                    </Badge>
                    <p className="text-xs text-muted-foreground">
                      {formatSize(partition.size)}
                    </p>
                  </div>
                </div>
              ))}
            </div>
          )}
        </ScrollArea>
      </CardContent>
    </Card>
  );
}
