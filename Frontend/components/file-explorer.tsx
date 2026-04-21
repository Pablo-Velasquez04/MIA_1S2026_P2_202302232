"use client";

import { useEffect, useMemo, useState } from "react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { Button } from "@/components/ui/button";
import { ScrollArea } from "@/components/ui/scroll-area";
import { executeCommand } from "@/lib/api";
import {
  Folder,
  FileText,
  HardDrive,
  ChevronRight,
  ChevronLeft,
  Eye,
  RefreshCw,
} from "lucide-react";
import { Partition } from "@/components/mounted-partitions";

interface ExplorerProps {
  partitions: Partition[];
  isConnected: boolean;
  onLogout: () => void;
  onCommandExecuted?: number;
}

interface ExplorerNode {
  name: string;
  path: string;
  type: "folder" | "file";
  permissions?: string;
}

interface Disk {
  path: string;
  name: string;
  capacity: string;
  fit: string;
  partitionsCount: number;
}

interface PartitionInfo {
  id: string;
  name: string;
  size: string;
  fit: string;
  status: string;
}

export function FileExplorer({ partitions, isConnected, onLogout, onCommandExecuted }: ExplorerProps) {
  const [currentView, setCurrentView] = useState<'disks' | 'partitions' | 'filesystem'>('disks');
  const [disks, setDisks] = useState<Disk[]>([]);
  const [selectedDisk, setSelectedDisk] = useState<Disk | null>(null);
  const [partitionsList, setPartitionsList] = useState<PartitionInfo[]>([]);
  const [selectedPartition, setSelectedPartition] = useState<PartitionInfo | null>(null);
  const [currentPath, setCurrentPath] = useState("/");
  const [nodes, setNodes] = useState<ExplorerNode[]>([]);
  const [selectedNode, setSelectedNode] = useState<ExplorerNode | null>(null);
  const [fileContent, setFileContent] = useState<string>("");
  const [error, setError] = useState<string>("");
  const [isLoading, setIsLoading] = useState(false);
  const [isLoadingFile, setIsLoadingFile] = useState(false);

  useEffect(() => {
    if (currentView === 'disks') {
      loadDisks();
    }
  }, [currentView, onCommandExecuted]);

  const loadDisks = async () => {
    setIsLoading(true);
    setError("");
    try {
      const response = await executeCommand('mounted');

      if (!response.success) {
        setError(response.error || "No hay particiones montadas");
        setDisks([]);
        setIsLoading(false);
        return;
      }

      const output = response.output || "";
      const lines = output.split('\n');
      const diskMap = new Map<string, { count: number; partitions: PartitionInfo[] }>();

      for (const line of lines) {
        if (!line.trim() || line.includes('===') || line.includes('---') || line.includes('ID') || line.includes('Total:')) {
          continue;
        }
        const parts = line.split(/\s+/);
        if (parts.length >= 6) {
          const id = parts[0];
          const name = parts[1];
          const size = parts[4];
          const diskPath = parts.slice(5).join(' ');

          if (!diskMap.has(diskPath)) {
            diskMap.set(diskPath, { count: 0, partitions: [] });
          }
          const diskInfo = diskMap.get(diskPath)!;
          diskInfo.count++;
          diskInfo.partitions.push({ id, name, size, fit: 'Desconocido', status: 'montada' });
        }
      }

      const diskList: Disk[] = Array.from(diskMap.entries()).map(([path, info]) => ({
        path,
        name: path.split('/').pop() || 'Unknown',
        capacity: 'Desconocido',
        fit: 'Desconocido',
        partitionsCount: info.count,
      }));

      setDisks(diskList);
      setError("");
    } catch (e) {
      setError(`Error cargando discos: ${e instanceof Error ? e.message : String(e)}`);
      setDisks([]);
    } finally {
      setIsLoading(false);
    }
  };

  const loadPartitions = async (diskPath: string) => {
    setIsLoading(true);
    setError("");
    try {
      const response = await executeCommand('mounted');

      if (!response.success) {
        setError(response.error || "No hay particiones montadas");
        setPartitionsList([]);
        setIsLoading(false);
        return;
      }

      const output = response.output || "";
      const lines = output.split('\n');
      const partList: PartitionInfo[] = [];

      for (const line of lines) {
        if (!line.trim() || line.includes('===') || line.includes('---') || line.includes('ID') || line.includes('Total:')) {
          continue;
        }
        const parts = line.split(/\s+/);
        if (parts.length >= 6) {
          const id = parts[0];
          const name = parts[1];
          const size = parts[4];
          const currentDiskPath = parts.slice(5).join(' ');
          if (currentDiskPath === diskPath) {
            partList.push({ id, name, size, fit: 'Desconocido', status: 'montada' });
          }
        }
      }

      setPartitionsList(partList);
      setError("");
    } catch (e) {
      setError(`Error cargando particiones: ${e instanceof Error ? e.message : String(e)}`);
      setPartitionsList([]);
    } finally {
      setIsLoading(false);
    }
  };

  const loadDirectory = async (path: string) => {
    if (!selectedPartition) return;
    if (!isConnected) {
      setError("El backend no está conectado.");
      return;
    }

    setIsLoading(true);
    setError("");
    setNodes([]);

    try {
      const command = `find -id=${selectedPartition.id} -path="${path}"`;
      const response = await executeCommand(command);

      if (!response.success) {
        setError((response.error || response.output || "No se pudo listar la carpeta").replace(/^Error:\s*/i, ""));
        return;
      }

      const entries = response.output
        .split("\n")
        .map((line) => line.trim())
        .filter((line) => line.length > 0);

      const prefix = path === "/" ? "/" : `${path}/`;
      const directChildren = new Map<string, ExplorerNode>();

      for (const entry of entries) {
        if (!entry.startsWith(prefix)) continue;
        let relative = entry.slice(prefix.length);
        if (relative.startsWith("/")) relative = relative.slice(1);
        if (!relative) continue;

        const segments = relative.split("/");
        const name = segments[0];
        const childPath = path === "/" ? `/${name}` : `${path}/${name}`;
        const isFolder = segments.length > 1;

        if (!directChildren.has(childPath)) {
          directChildren.set(childPath, {
            name,
            path: childPath,
            type: isFolder ? "folder" : "file",
            permissions: "rwx",
          });
        } else if (isFolder) {
          const existing = directChildren.get(childPath);
          if (existing) existing.type = "folder";
        }
      }

      const sortedNodes = Array.from(directChildren.values()).sort((a, b) => {
        if (a.type !== b.type) return a.type === "folder" ? -1 : 1;
        return a.name.localeCompare(b.name);
      });
      setNodes(sortedNodes);
    } catch (err) {
      setError(`Error al listar la carpeta: ${err instanceof Error ? err.message : String(err)}`);
    } finally {
      setIsLoading(false);
    }
  };

  // ── Navegación entre vistas ──────────────────────────────
  const goToDisks = () => {
    setCurrentView('disks');
    setSelectedDisk(null);
    setSelectedPartition(null);
    setCurrentPath('/');
    setSelectedNode(null);
    setFileContent('');
    setError('');
  };

  // FUNCIÓN QUE FALTABA
  const goToPartitions = () => {
    setCurrentView('partitions');
    setSelectedPartition(null);
    setCurrentPath('/');
    setSelectedNode(null);
    setFileContent('');
    setError('');
    if (selectedDisk) {
      loadPartitions(selectedDisk.path);
    }
  };

  const handleSelectDisk = (disk: Disk) => {
    setSelectedDisk(disk);
    setCurrentView('partitions');
    loadPartitions(disk.path);
  };

  const handleSelectPartition = (partition: PartitionInfo) => {
    setSelectedPartition(partition);
    setCurrentPath('/');
    setCurrentView('filesystem');
    loadDirectory('/');
  };

  const handleNodeClick = (node: ExplorerNode) => {
    setSelectedNode(node);
    setError("");
    setFileContent("");
    if (node.type === "folder") {
      setCurrentPath(node.path);
      loadDirectory(node.path);
    } else {
      loadFileContent(node.path);
    }
  };

  const handleGoUp = () => {
    if (currentPath === "/") return;
    const segments = currentPath.split("/").filter(Boolean);
    const parent = segments.slice(0, -1).join("/");
    const nextPath = parent ? `/${parent}` : "/";
    setCurrentPath(nextPath);
    setSelectedNode(null);
    setFileContent("");
    loadDirectory(nextPath);
  };

  const loadFileContent = async (path: string) => {
    if (!isConnected) {
      setError("El backend no está conectado.");
      return;
    }

    setIsLoadingFile(true);
    setError("");
    setFileContent("");

    try {
      const escapedPath = path.replace(/"/g, '\\"');
      const response = await executeCommand(`cat -id=${selectedPartition!.id} -file1="${escapedPath}"`);
      if (!response.success) {
        setError((response.error || response.output || "No se pudo leer el archivo").replace(/^Error:\s*/i, ""));
        return;
      }
      setFileContent(response.output || "");
    } catch (err) {
      setError(`Error al leer el archivo: ${err instanceof Error ? err.message : String(err)}`);
    } finally {
      setIsLoadingFile(false);
    }
  };

  // ── Render ───────────────────────────────────────────────
  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h2 className="text-2xl font-bold">Visualizador del Sistema de Archivos</h2>
        <Button variant="outline" onClick={onLogout}>
          Cerrar Sesión
        </Button>
      </div>

      {/* VISTA: DISCOS */}
      {currentView === 'disks' && (
        <Card>
          <CardHeader>
            <CardTitle className="flex items-center gap-2">
              <HardDrive className="h-5 w-5" /> Selección de Disco
            </CardTitle>
          </CardHeader>
          <CardContent>
            {error && <p className="text-destructive mb-4">{error}</p>}
            {isLoading ? (
              <p>Cargando discos...</p>
            ) : disks.length === 0 ? (
              <p>No hay discos disponibles. Crea y monta particiones primero.</p>
            ) : (
              <div className="grid gap-4">
                {disks.map((disk) => (
                  <Card key={disk.path}>
                    <CardContent className="pt-4">
                      <div className="flex items-center justify-between">
                        <div>
                          <p className="font-medium">{disk.name}</p>
                          <p className="text-sm text-muted-foreground">
                            Capacidad: {disk.capacity} | Fit: {disk.fit} | Particiones: {disk.partitionsCount}
                          </p>
                        </div>
                        <Button onClick={() => handleSelectDisk(disk)}>Seleccionar Disco</Button>
                      </div>
                    </CardContent>
                  </Card>
                ))}
              </div>
            )}
          </CardContent>
        </Card>
      )}

      {/* VISTA: PARTICIONES */}
      {currentView === 'partitions' && selectedDisk && (
        <Card>
          <CardHeader>
            <CardTitle className="flex items-center gap-2">
              <HardDrive className="h-5 w-5" /> Selección de Partición — {selectedDisk.name}
            </CardTitle>
            <Button variant="outline" onClick={goToDisks}>Volver a Discos</Button>
          </CardHeader>
          <CardContent>
            {error && <p className="text-destructive mb-4">{error}</p>}
            {isLoading ? (
              <p>Cargando particiones...</p>
            ) : partitionsList.length === 0 ? (
              <p>No hay particiones en este disco.</p>
            ) : (
              <div className="grid gap-4">
                {partitionsList.map((partition) => (
                  <Card key={partition.id}>
                    <CardContent className="pt-4">
                      <div className="flex items-center justify-between">
                        <div>
                          <p className="font-medium">{partition.name}</p>
                          <p className="text-sm text-muted-foreground">
                            ID: {partition.id} | Tamaño: {partition.size} | Fit: {partition.fit} | Estado: {partition.status}
                          </p>
                        </div>
                        <Button onClick={() => handleSelectPartition(partition)}>Seleccionar Partición</Button>
                      </div>
                    </CardContent>
                  </Card>
                ))}
              </div>
            )}
          </CardContent>
        </Card>
      )}

      {/* VISTA: SISTEMA DE ARCHIVOS */}
      {currentView === 'filesystem' && selectedPartition && (
        <div className="grid gap-6 lg:grid-cols-[1fr_1fr]">
          <Card>
            <CardHeader>
              <CardTitle className="flex items-center gap-2">
                <Folder className="h-5 w-5" /> {selectedPartition.name} ({selectedPartition.id})
              </CardTitle>
              <div className="flex gap-2 flex-wrap">
                <Button variant="outline" onClick={goToPartitions}>Volver a Particiones</Button>
                <Button variant="outline" onClick={handleGoUp} disabled={currentPath === "/"}>Subir</Button>
                <Button variant="outline" onClick={() => loadDirectory(currentPath)}>
                  <RefreshCw className="h-4 w-4 mr-1" /> Refrescar
                </Button>
              </div>
            </CardHeader>
            <CardContent>
              <p className="text-sm font-medium mb-4">Ruta actual: {currentPath}</p>
              {error && <p className="text-destructive mb-4">{error}</p>}
              {isLoading ? (
                <p>Cargando contenido...</p>
              ) : nodes.length === 0 ? (
                <p>No hay elementos en esta carpeta.</p>
              ) : (
                <ScrollArea className="h-[400px]">
                  <div className="space-y-2">
                    {nodes.map((node) => (
                      <div
                        key={node.path}
                        className="flex items-center gap-3 p-2 rounded border cursor-pointer hover:bg-muted"
                        onClick={() => handleNodeClick(node)}
                      >
                        {node.type === 'folder'
                          ? <Folder className="h-4 w-4 text-yellow-500" />
                          : <FileText className="h-4 w-4 text-blue-500" />}
                        <div>
                          <p className="font-medium">{node.name}</p>
                          <p className="text-xs text-muted-foreground">Permisos: {node.permissions}</p>
                        </div>
                      </div>
                    ))}
                  </div>
                </ScrollArea>
              )}
            </CardContent>
          </Card>

          <Card>
            <CardHeader>
              <CardTitle className="flex items-center gap-2">
                <Eye className="h-5 w-5" /> Contenido del Archivo
              </CardTitle>
            </CardHeader>
            <CardContent>
              {selectedNode?.type === 'file' ? (
                <div>
                  <p className="font-medium mb-2">{selectedNode.name}</p>
                  {isLoadingFile ? (
                    <p>Cargando archivo...</p>
                  ) : fileContent ? (
                    <ScrollArea className="h-[400px] border rounded p-2">
                      <pre className="text-sm whitespace-pre-wrap">{fileContent}</pre>
                    </ScrollArea>
                  ) : (
                    <p className="text-muted-foreground text-sm">Cargando contenido...</p>
                  )}
                </div>
              ) : (
                <p className="text-muted-foreground text-sm">Selecciona un archivo para ver su contenido.</p>
              )}
            </CardContent>
          </Card>
        </div>
      )}
    </div>
  );
}