"use client";

import { useState, useEffect, useCallback, FormEvent } from "react";
import { CommandEditor } from "@/components/command-editor";
import { OutputConsole, OutputLine } from "@/components/output-console";
import { MountedPartitions, Partition } from "@/components/mounted-partitions";
import { ReportViewer, Report } from "@/components/report-viewer";
import { ConnectionStatus } from "@/components/connection-status";
import { CommandReference } from "@/components/command-reference";
import { JournalingView, JournalEntry } from "@/components/journaling-view";
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
  DialogTrigger,
} from "@/components/ui/dialog";
import { executeCommand, getReportUrl } from "@/lib/api";
import { HardDrive, Github, BookOpen } from "lucide-react";
import { Button } from "@/components/ui/button";

export default function Home() {
  const [output, setOutput] = useState<OutputLine[]>([]);
  const [isLoading, setIsLoading] = useState(false);
  const [partitions, setPartitions] = useState<Partition[]>([]);
  const [reports, setReports] = useState<Report[]>([]);
  const [journalEntries, setJournalEntries] = useState<JournalEntry[]>([]);
  const [isConnected, setIsConnected] = useState(false);

  const [loginOpen, setLoginOpen] = useState(false);
  const [partitionId, setPartitionId] = useState("");
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [rememberUser, setRememberUser] = useState(false);
  const [loginError, setLoginError] = useState("");
  const [isLoginLoading, setIsLoginLoading] = useState(false);
  const [isLoggedIn, setIsLoggedIn] = useState(false);
  const [loggedInUser, setLoggedInUser] = useState("");
  const [loggedPartitionId, setLoggedPartitionId] = useState("");

  useEffect(() => {
    if (typeof window === "undefined") return;
    const saved = window.localStorage.getItem("extreamfs-login");
    if (saved) {
      try {
        const data = JSON.parse(saved) as { partitionId: string; username: string };
        setPartitionId(data.partitionId || "");
        setUsername(data.username || "");
        setRememberUser(true);
      } catch {
        window.localStorage.removeItem("extreamfs-login");
      }
    }
  }, []);

  // Conecta el formulario al comando `login` del backend
  const handleLoginSubmit = async (event: FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    setLoginError("");

    const id = partitionId.trim();
    const user = username.trim();
    const pass = password.trim();

    if (!id || !user || !pass) {
      setLoginError("Todos los campos son obligatorios.");
      return;
    }

    if (!isConnected) {
      setLoginError("El backend no está conectado. Inicia el servidor primero.");
      return;
    }

    setIsLoginLoading(true);
    try {
      const cmd = `login -user=${user} -pass=${pass} -id=${id}`;
      const response = await executeCommand(cmd);

      if (response.success) {
        // Login exitoso
        if (rememberUser) {
          window.localStorage.setItem(
            "extreamfs-login",
            JSON.stringify({ partitionId: id, username: user })
          );
        } else {
          window.localStorage.removeItem("extreamfs-login");
        }
        setLoggedInUser(user);
        setLoggedPartitionId(id);
        setIsLoggedIn(true);
        setLoginOpen(false);
        setPassword("");
        addOutputLine("success", `Login exitoso como '${user}' en partición '${id}'.`);
      } else {
        // Error del backend → mostrarlo en el dialog
        const errorText = (response.error || response.output || "Error desconocido")
          .replace(/^Error:\s*/i, "")
          .trim();
        setLoginError(errorText);
      }
    } catch (e) {
      setLoginError(`Error de conexión con el backend: ${e instanceof Error ? e.message : String(e)}`);
    } finally {
      setIsLoginLoading(false);
    }
  };

  // Llama al comando `logout` del backend y limpia el estado local
  const handleLogout = async () => {
    if (isConnected) {
      try {
        await executeCommand("logout");
      } catch { /* ignorar errores de red al cerrar sesión */ }
    }
    setIsLoggedIn(false);
    setLoggedInUser("");
    setLoggedPartitionId("");
    addOutputLine("info", "Sesión cerrada.");
  };

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

  // Detectar si es un comando JOURNALING exitoso y parsear JSON
  const handleJournalingResponse = (command: string, response: string) => {
    const journalMatch = command.toLowerCase().trim().startsWith("journaling");
    if (journalMatch && response.trim().startsWith("[")) {
      try {
        const entries = JSON.parse(response);
        setJournalEntries(entries);
      } catch (e) {
        console.error("Error al parsear Journaling JSON", e);
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
    // Comentado para preservar la sesión del usuario a petición:
    // if (isConnected) {
    //   await executeCommand("execute");
    // }
    
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
          handleJournalingResponse(trimmedLine, response.output);
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

  // Se dispara cuando el backend detecta conexión (incluyendo primer arranque).
  // Limpia el estado de datos del frontend para que no muestre datos de una sesión anterior.
  const handleBackendReconnect = () => {
    setPartitions([]);
    setReports([]);
    setJournalEntries([]);
    addOutputLine("info", "Backend conectado. Estado del frontend limpiado. Particiones montadas y journaling reiniciados.");
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
              <ConnectionStatus onStatusChange={setIsConnected} onReconnect={handleBackendReconnect} />
              <Dialog open={loginOpen} onOpenChange={setLoginOpen}>
                <DialogTrigger asChild>
                  <Button variant="outline" size="sm">
                    Iniciar sesión
                  </Button>
                </DialogTrigger>
                <DialogContent>
                  <DialogHeader>
                    <DialogTitle>Login de usuario</DialogTitle>
                    <DialogDescription>
                      Ingresa ID de partición, usuario y contraseña para acceder al visualizador.  Previamente debío de ejecutar comandos tipo: MKGRP, RMGRP, MKUSR, RMUSR para crear grupos y usuarios en el sistema de archivos simulado.  Este login es solo para habilitar la vista de particiones montadas y journaling, no afecta la ejecución de comandos en el editor.
                    </DialogDescription>
                  </DialogHeader>
                  <form className="space-y-4" onSubmit={handleLoginSubmit}>
                    <div className="grid gap-2">
                      <label className="text-sm font-medium" htmlFor="partitionId">
                        ID Partición
                      </label>
                      <input
                        id="partitionId"
                        type="text"
                        value={partitionId}
                        onChange={(event) => setPartitionId(event.target.value)}
                        className="w-full rounded-md border border-input bg-background px-3 py-2 text-sm outline-none transition-colors focus:border-primary focus:ring-2 focus:ring-primary/20"
                        placeholder="Ej. 01A"
                      />
                    </div>
                    <div className="grid gap-2">
                      <label className="text-sm font-medium" htmlFor="username">
                        Usuario
                      </label>
                      <input
                        id="username"
                        type="text"
                        value={username}
                        onChange={(event) => setUsername(event.target.value)}
                        className="w-full rounded-md border border-input bg-background px-3 py-2 text-sm outline-none transition-colors focus:border-primary focus:ring-2 focus:ring-primary/20"
                        placeholder="Nombre de usuario"
                      />
                    </div>
                    <div className="grid gap-2">
                      <label className="text-sm font-medium" htmlFor="password">
                        Contraseña
                      </label>
                      <input
                        id="password"
                        type="password"
                        value={password}
                        onChange={(event) => setPassword(event.target.value)}
                        className="w-full rounded-md border border-input bg-background px-3 py-2 text-sm outline-none transition-colors focus:border-primary focus:ring-2 focus:ring-primary/20"
                        placeholder="********"
                      />
                    </div>
                    <div className="flex items-center gap-2">
                      <input
                        id="rememberUser"
                        type="checkbox"
                        checked={rememberUser}
                        onChange={(event) => setRememberUser(event.target.checked)}
                        className="h-4 w-4 rounded border-input text-primary focus:ring-primary"
                      />
                      <label htmlFor="rememberUser" className="text-sm">
                        Recordar usuario
                      </label>
                    </div>
                    {loginError ? (
                      <p className="rounded-md border border-destructive/40 bg-destructive/10 px-3 py-2 text-sm text-destructive">
                        {loginError}
                      </p>
                    ) : null}
                    <DialogFooter>
                      <Button type="submit" className="w-full" disabled={isLoginLoading}>
                        {isLoginLoading ? "Verificando..." : "Verificar datos"}
                      </Button>
                    </DialogFooter>
                  </form>
                </DialogContent>
              </Dialog>
              <Button variant="outline" size="sm" asChild>
                <a
                  href="https://github.com/Pablo-Velasquez04/MIA_1S2026_P2_202302232.git"
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
        {isLoggedIn ? (
          <div className="rounded-lg border border-foreground/10 bg-foreground/5 p-4 text-sm text-foreground/90 shadow-sm">
            <div className="flex flex-col gap-3 lg:flex-row lg:items-center lg:justify-between">
              <div>
                <p className="font-semibold">Visualizador de Sistemas de archivos habilitado</p>
                <p className="text-sm text-muted-foreground">
                  Conectado como <span className="font-medium">{loggedInUser}</span> en partición <span className="font-medium">{loggedPartitionId}</span>.
                </p>
              </div>
              <Button variant="secondary" size="sm" onClick={handleLogout}>
                Cerrar sesión
              </Button>
            </div>
          </div>
        ) : null}
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
            <TabsList className="grid w-full grid-cols-3 lg:w-[450px]">
              <TabsTrigger value="partitions" className="gap-1">
                <HardDrive className="h-4 w-4" />
                Particiones
              </TabsTrigger>
              <TabsTrigger value="reference" className="gap-1">
                <BookOpen className="h-4 w-4" />
                Referencia
              </TabsTrigger>
              <TabsTrigger value="journaling" className="gap-1">
                <HardDrive className="h-4 w-4" />
                Journaling
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
            <TabsContent value="journaling" className="mt-4">
              <JournalingView entries={journalEntries} />
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
