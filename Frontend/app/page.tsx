"use client";

import { useState, useCallback } from "react";
import { CommandEditor } from "@/components/command-editor";
import { OutputConsole, OutputLine } from "@/components/output-console";
import { MountedPartitions, Partition } from "@/components/mounted-partitions";
import { ReportViewer, Report } from "@/components/report-viewer";
import { ConnectionStatus } from "@/components/connection-status";
import { CommandReference } from "@/components/command-reference";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import { executeCommand, getReportUrl } from "@/lib/api";
import { HardDrive, Github, BookOpen } from "lucide-react";
import { Button } from "@/components/ui/button";

export default function Home() {
  const [output, setOutput] = useState<OutputLine[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [partitions, setPartitions] = useState<Partition[]>([]);
  const [reports, setReports] = useState<Report[]>([]);
  const [isConnected, setIsConnected] = useState(false);

  // Agregar línea al output
  const addOutputLine = useCallback(
    (type: OutputLine["type"], content: string) => {
      const newLine: OutputLine = {
        id: `${Date.now()}-${Math.random()}`,
        type,
        content,
        timestamp: new Date(),
      };
      setOutput((prev) => [...prev, newLine]);
    },
    []
  );

  // Parsear comandos del texto (eliminar comentarios y líneas vacías)
  const parseCommands = (text: string): string[] => {
    return text
      .split("\n")
      .map((line) => line.trim())
      .filter((line) => line && !line.startsWith("#"));
  };

  // Detectar si es un comando MOUNT exitoso y extraer info
  const handleMountResponse = (command: string, response: string) => {
    const mountMatch = command.toLowerCase().match(/mount\s+/);
    if (mountMatch && response.toLowerCase().includes("exitosamente")) {
      // Extraer información del comando mount
      const pathMatch = command.match(/-path\s*=\s*"?([^"\s]+)"?/i);
      const nameMatch = command.match(/-name\s*=\s*"?([^"\s]+)"?/i);
      const idMatch = response.match(/ID:\s*(\w+)/i) || response.match(/(\d{2,3}[A-Z])/);

      if (pathMatch && nameMatch) {
        const newPartition: Partition = {
          id: idMatch ? idMatch[1] : `${Date.now()}`,
          name: nameMatch[1],
          path: pathMatch[1],
          size: 0,
          type: "P",
          status: "mounted",
        };
        setPartitions((prev) => {
          // Evitar duplicados
          if (prev.some((p) => p.id === newPartition.id)) return prev;
          return [...prev, newPartition];
        });
      }
    }
  };

  // Detectar si es un comando REP exitoso y agregar reporte
  const handleReportResponse = (command: string, response: string) => {
    const repMatch = command.toLowerCase().match(/rep\s+/);
    if (repMatch && response.toLowerCase().includes("generado")) {
      const nameMatch = command.match(/-name\s*=\s*"?([^"\s]+)"?/i);
      const pathMatch = command.match(/-path\s*=\s*"?([^"\s]+)"?/i);

      if (nameMatch && pathMatch) {
        const newReport: Report = {
          id: `${Date.now()}`,
          name: pathMatch[1].split("/").pop() || "reporte",
          type: nameMatch[1].toLowerCase(),
          path: pathMatch[1],
          imageUrl: getReportUrl(pathMatch[1]),
          generatedAt: new Date(),
        };
        setReports((prev) => [...prev, newReport]);
      }
    }
  };

  // Ejecutar comandos
  const handleExecute = async (commandText: string) => {
    const allLines = commandText.split("\n");
    
    if (allLines.length === 0 || (allLines.length === 1 && !allLines[0].trim())) {
      addOutputLine("error", "No hay comandos validos para ejecutar");
      return;
    }

    setIsLoading(true);

    // NUEVO: Ejecutar 'execute' silenciosamente para limpiar RAM en el backend (Reset Global)
    if (isConnected) {
      await executeCommand("execute");
    }
    
    for (let line of allLines) {
      const trimmedLine = line.trim();
      
      // Preservar líneas en blanco o comentarios
      if (!trimmedLine || trimmedLine.startsWith("#")) {
        addOutputLine("info", line || " "); // " " para línea vacía real
        continue;
      }

      // Es un comando
      addOutputLine("command", `> ${trimmedLine}`);

      if (!isConnected) {
        addOutputLine(
          "error",
          "Backend no conectado. Conecta tu servidor C++ para ejecutar comandos reales."
        );
        continue;
      }

      try {
        const response = await executeCommand(trimmedLine);

        if (response.success) {
          const raw = response.output || "Comando ejecutado exitosamente";
          const lines = raw.split('\n').filter((l: string) => l.trim() !== '');
          lines.forEach((line: string) => addOutputLine("success", line));
          handleMountResponse(trimmedLine, response.output);
          handleReportResponse(trimmedLine, response.output);
        } else {
          const raw = response.error || response.output || "Error desconocido";
          const lines = raw.split('\n').filter((l: string) => l.trim() !== '');
          lines.forEach((line: string) => addOutputLine("error", line));
        }
      } catch (error) {
        addOutputLine(
          "error",
          `Error de conexion: ${error instanceof Error ? error.message : "Error desconocido"}`
        );
      }
    }

    setIsLoading(false);
  };

  const handleClearOutput = () => {
    setOutput([]);
  };

  const handleRemoveReport = (id: string) => {
    setReports((prev) => prev.filter((r) => r.id !== id));
  };

  return (
    <div className="min-h-screen bg-transparent">
      {/* Header */}
      <header className="border-b bg-background/80 backdrop-blur-sm">
        <div className="container mx-auto px-4 py-4">
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-3">
              <div className="h-10 w-10 rounded-lg bg-primary flex items-center justify-center">
                <HardDrive className="h-6 w-6 text-primary-foreground" />
              </div>
              <div>
                <h1 className="text-xl font-bold">ExtreamFS</h1>
                <p className="text-sm text-muted-foreground">
                  Simulador de Sistema de Archivos EXT2
                </p>
              </div>
            </div>
            <div className="flex items-center gap-4">
              <ConnectionStatus onStatusChange={setIsConnected} />
              <Button variant="outline" size="sm" asChild>
                <a
                  href="https://github.com/Pablo-Velasquez04/MIA_1S2026_P1_2023002232.git"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="gap-2"
                >
                  <Github className="h-4 w-4" />
                  GitHub
                </a>
              </Button>
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="container mx-auto px-4 py-6 space-y-6">
        {/* Top Section - Editor & Console */}
        <div className="grid grid-cols-1 lg:grid-cols-[1.5fr_1fr] gap-6 items-stretch">
          {/* Left Column - Command Editor */}
          <div className="w-full h-[450px]">
            <CommandEditor onExecute={handleExecute} isLoading={isLoading} />
          </div>

          {/* Right Column - Output Console */}
          <div className="w-full h-[450px]">
            <OutputConsole output={output} onClear={handleClearOutput} />
          </div>
        </div>

        {/* Bottom Section - Info Panels */}
        <div className="w-full">
          <Tabs defaultValue="partitions" className="w-full">
            <TabsList className="grid w-full grid-cols-2 lg:w-[400px]">
              <TabsTrigger value="partitions" className="gap-1">
                <HardDrive className="h-4 w-4" />
                Particiones
              </TabsTrigger>
              <TabsTrigger value="reference" className="gap-1">
                <BookOpen className="h-4 w-4" />
                Referencia
              </TabsTrigger>
            </TabsList>
            <TabsContent value="partitions" className="mt-4 space-y-4">
              <MountedPartitions partitions={partitions} />
              <ReportViewer
                reports={reports}
                onRemoveReport={handleRemoveReport}
              />
            </TabsContent>
            <TabsContent value="reference" className="mt-4">
              <CommandReference />
            </TabsContent>
          </Tabs>
        </div>
      </main>

      {/* Footer */}
      <footer className="border-t mt-auto">
        <div className="container mx-auto px-4 py-4">
          <div className="flex items-center justify-between text-sm text-muted-foreground">
          </div>
        </div>
      </footer>
    </div>
  );
}
