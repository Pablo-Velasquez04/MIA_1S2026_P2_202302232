"use client";

import { useState, useEffect } from "react";
import { HardDrive, Server, Folder, FileText, ChevronRight, ArrowLeft, RefreshCw, X } from "lucide-react";
import { Button } from "@/components/ui/button";
import {
  getExplorerDisks,
  getExplorerPartitions,
  getExplorerDir,
  getExplorerFileContent,
  ExplorerDisk,
  ExplorerPartition,
  ExplorerFile
} from "@/lib/api";

type ViewLevel = "DISKS" | "PARTITIONS" | "FILES";

function formatSize(bytes: number) {
  if (bytes === 0) return "0 B";
  const k = 1024;
  const sizes = ["B", "KB", "MB", "GB"];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + " " + sizes[i];
}

interface FileSystemVisualizerProps {
  refreshTrigger?: number;
}

export function FileSystemVisualizer({ refreshTrigger }: FileSystemVisualizerProps) {
  const [level, setLevel] = useState<ViewLevel>("DISKS");
  const [loading, setLoading] = useState(false);

  // DISKS
  const [disks, setDisks] = useState<ExplorerDisk[]>([]);
  const [selectedDisk, setSelectedDisk] = useState<ExplorerDisk | null>(null);

  // PARTITIONS
  const [partitions, setPartitions] = useState<ExplorerPartition[]>([]);
  const [selectedPartition, setSelectedPartition] = useState<ExplorerPartition | null>(null);

  // FILES
  const [currentPath, setCurrentPath] = useState<string>("/");
  const [files, setFiles] = useState<ExplorerFile[]>([]);
  const [selectedFile, setSelectedFile] = useState<ExplorerFile | null>(null);
  const [fileContent, setFileContent] = useState<string>("");

  const loadDisks = async () => {
    setLoading(true);
    const d = await getExplorerDisks();
    setDisks(d);
    setLoading(false);
  };

  useEffect(() => {
    if (level === "DISKS") loadDisks();
  }, [level]);

  const selectDisk = async (disk: ExplorerDisk) => {
    setSelectedDisk(disk);
    setLoading(true);
    const parts = await getExplorerPartitions(disk.path);
    setPartitions(parts);
    setLoading(false);
    setLevel("PARTITIONS");
  };

  const selectPartition = async (part: ExplorerPartition) => {
    if (part.status !== "Mounted") return;
    setSelectedPartition(part);
    setLoading(true);
    const dirFiles = await getExplorerDir(part.id, "/");
    setFiles(dirFiles);
    setCurrentPath("/");
    setLevel("FILES");
    setLoading(false);
  };

  const navigateTo = async (folderName: string) => {
    setLoading(true);
    const newPath = currentPath === "/" ? `/${folderName}` : `${currentPath}/${folderName}`;
    const dirFiles = await getExplorerDir(selectedPartition!.id, newPath);
    setCurrentPath(newPath);
    setFiles(dirFiles);
    setLoading(false);
  };

  const navigateUp = async () => {
    if (currentPath === "/") return;
    setLoading(true);
    const parts = currentPath.split("/").filter(Boolean);
    parts.pop();
    const newPath = parts.length === 0 ? "/" : "/" + parts.join("/");
    const dirFiles = await getExplorerDir(selectedPartition!.id, newPath);
    setCurrentPath(newPath);
    setFiles(dirFiles);
    setLoading(false);
  };

  const openFile = async (file: ExplorerFile) => {
    setLoading(true);
    setSelectedFile(file);
    const fullPath = currentPath === "/" ? `/${file.name}` : `${currentPath}/${file.name}`;
    const content = await getExplorerFileContent(selectedPartition!.id, fullPath);
    setFileContent(content);
    setLoading(false);
  };

  const handleRefresh = async () => {
    if (level === "DISKS") loadDisks();
    if (level === "PARTITIONS" && selectedDisk) {
      setLoading(true);
      const parts = await getExplorerPartitions(selectedDisk.path);
      setPartitions(parts);
      setLoading(false);
    }
    if (level === "FILES" && selectedPartition) {
      setLoading(true);
      const dirFiles = await getExplorerDir(selectedPartition.id, currentPath);
      setFiles(dirFiles);
      setLoading(false);
    }
  };

  useEffect(() => {
    if (refreshTrigger) handleRefresh();
  }, [refreshTrigger]);

  return (
    <div className="flex flex-col h-full bg-background border rounded-lg overflow-hidden shadow-sm">
      {/* HEADER / BREADCRUMBS */}
      <div className="flex items-center justify-between p-3 border-b bg-muted/40">
        <div className="flex items-center gap-2 text-sm font-medium">
          <Button
            variant="ghost"
            size="sm"
            className="px-2"
            onClick={() => {
              if (level === "FILES") setLevel("PARTITIONS");
              else if (level === "PARTITIONS") setLevel("DISKS");
            }}
            disabled={level === "DISKS"}
          >
            <ArrowLeft className="w-4 h-4" />
          </Button>

          <span
            className={`cursor-pointer hover:underline ${level === "DISKS" ? "text-primary" : "text-muted-foreground"}`}
            onClick={() => setLevel("DISKS")}
          >
            Discos
          </span>

          {selectedDisk && (level === "PARTITIONS" || level === "FILES") && (
            <>
              <ChevronRight className="w-4 h-4 text-muted-foreground" />
              <span
                className={`cursor-pointer hover:underline ${level === "PARTITIONS" ? "text-primary" : "text-muted-foreground"}`}
                onClick={() => setLevel("PARTITIONS")}
              >
                {selectedDisk.name}
              </span>
            </>
          )}

          {selectedPartition && level === "FILES" && (
            <>
              <ChevronRight className="w-4 h-4 text-muted-foreground" />
              <span className="text-primary">{selectedPartition.name}</span>
            </>
          )}
        </div>

        <Button variant="outline" size="sm" onClick={handleRefresh} disabled={loading}>
          <RefreshCw className={`w-4 h-4 mr-2 ${loading ? "animate-spin" : ""}`} />
          Refrescar
        </Button>
      </div>

      {/* MAIN CONTENT AREA */}
      <div className="flex-1 overflow-auto p-4 relative">
        {level === "DISKS" && (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {disks.length === 0 && !loading && (
              <p className="text-sm text-muted-foreground col-span-full">No hay discos con particiones montadas.</p>
            )}
            {disks.map((d, i) => (
              <div
                key={i}
                className="border rounded-lg p-5 hover:border-primary cursor-pointer transition-colors"
                onClick={() => selectDisk(d)}
              >
                <div className="flex items-center gap-3 mb-3">
                  <div className="p-2 bg-primary/10 rounded-md text-primary">
                    <Server className="w-6 h-6" />
                  </div>
                  <div>
                    <h3 className="font-semibold text-lg leading-none">{d.name}</h3>
                    <p className="text-xs text-muted-foreground mt-1">{d.path}</p>
                  </div>
                </div>
                <div className="grid grid-cols-2 gap-2 text-sm">
                  <div>
                    <span className="text-muted-foreground">Capacidad:</span>{" "}
                    <span className="font-medium">{formatSize(d.size)}</span>
                  </div>
                  <div>
                    <span className="text-muted-foreground">Fit:</span>{" "}
                    <span className="font-medium">{d.fit}</span>
                  </div>
                  <div className="col-span-2">
                    <span className="text-muted-foreground">Particiones montadas:</span>{" "}
                    <span className="font-medium">{d.mountedCount}</span>
                  </div>
                </div>
              </div>
            ))}
          </div>
        )}

        {level === "PARTITIONS" && (
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {partitions.map((p, i) => (
              <div
                key={i}
                className={`border rounded-lg p-5 relative transition-colors ${
                  p.status === "Mounted" ? "hover:border-primary cursor-pointer" : "opacity-60 bg-muted/30"
                }`}
                onClick={() => selectPartition(p)}
              >
                <div className="flex items-center gap-3 mb-3">
                  <div className={`p-2 rounded-md ${p.status === "Mounted" ? "bg-primary/10 text-primary" : "bg-muted text-muted-foreground"}`}>
                    <HardDrive className="w-6 h-6" />
                  </div>
                  <div>
                    <h3 className="font-semibold text-lg leading-none">{p.name}</h3>
                    <p className="text-xs text-muted-foreground mt-1">ID: {p.id || "N/A"}</p>
                  </div>
                </div>
                <div className="grid grid-cols-2 gap-2 text-sm">
                  <div>
                    <span className="text-muted-foreground">Tamaño:</span>{" "}
                    <span className="font-medium">{formatSize(p.size)}</span>
                  </div>
                  <div>
                    <span className="text-muted-foreground">Tipo/Fit:</span>{" "}
                    <span className="font-medium">{p.type} / {p.fit}</span>
                  </div>
                </div>
                <div className="absolute top-3 right-3 text-xs font-medium px-2 py-1 rounded-full bg-background border">
                  {p.status === "Mounted" ? <span className="text-green-500">Montada</span> : <span className="text-muted-foreground">Desmontada</span>}
                </div>
              </div>
            ))}
          </div>
        )}

        {level === "FILES" && (
          <div className="flex h-full gap-4">
            <div className={`flex-1 border rounded-lg overflow-hidden flex flex-col ${selectedFile ? "hidden md:flex" : "flex"}`}>
              <div className="bg-muted/40 px-3 py-2 border-b text-sm font-medium flex items-center gap-2">
                <Button variant="ghost" size="sm" onClick={navigateUp} disabled={currentPath === "/"} className="h-7 px-2">
                  <ArrowLeft className="w-3 h-3 mr-1" /> Atrás
                </Button>
                <div className="px-2 py-1 bg-background border rounded font-mono text-xs text-primary">{currentPath}</div>
              </div>
              <div className="flex-1 overflow-auto p-0">
                <table className="w-full text-sm text-left">
                  <thead className="bg-muted/30 text-muted-foreground border-b sticky top-0">
                    <tr>
                      <th className="px-4 py-2 font-medium">Nombre</th>
                      <th className="px-4 py-2 font-medium">Inodo</th>
                      <th className="px-4 py-2 font-medium">Permisos</th>
                      <th className="px-4 py-2 font-medium">Tamaño</th>
                    </tr>
                  </thead>
                  <tbody>
                    {files.length === 0 && (
                      <tr><td colSpan={4} className="text-center py-4 text-muted-foreground">Carpeta vacía</td></tr>
                    )}
                    {files.sort((a, b) => a.type.localeCompare(b.type)).map((f, i) => (
                      <tr 
                        key={i} 
                        className="border-b hover:bg-muted/50 cursor-pointer transition-colors"
                        onClick={() => f.type === "folder" ? navigateTo(f.name) : openFile(f)}
                      >
                        <td className="px-4 py-3 flex items-center gap-2">
                          {f.type === "folder" ? <Folder className="w-4 h-4 text-primary" /> : <FileText className="w-4 h-4 text-muted-foreground" />}
                          <span className={f.type === "folder" ? "font-medium" : ""}>{f.name}</span>
                        </td>
                        <td className="px-4 py-3 font-mono text-xs">{f.inodo}</td>
                        <td className="px-4 py-3 font-mono text-xs">{f.perms}</td>
                        <td className="px-4 py-3 text-muted-foreground">{f.type === "file" ? formatSize(f.size) : "-"}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </div>

            {selectedFile && (
              <div className="flex-[0.8] border rounded-lg overflow-hidden flex flex-col bg-background shadow-lg">
                <div className="bg-muted/40 px-3 py-2 border-b flex items-center justify-between">
                  <div className="flex items-center gap-2 font-medium text-sm">
                    <FileText className="w-4 h-4 text-primary" />
                    {selectedFile.name}
                  </div>
                  <Button variant="ghost" size="sm" onClick={() => setSelectedFile(null)} className="h-7 w-7 p-0">
                    <X className="w-4 h-4" />
                  </Button>
                </div>
                <div className="flex-1 p-0 relative overflow-hidden">
                  <textarea 
                    className="w-full h-full p-4 font-mono text-sm resize-none outline-none bg-transparent"
                    readOnly
                    value={fileContent}
                  />
                </div>
              </div>
            )}
          </div>
        )}
      </div>
    </div>
  );
}
