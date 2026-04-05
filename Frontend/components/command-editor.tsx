"use client";

import { useState, useRef, useCallback, UIEvent } from "react";
import { Button } from "@/components/ui/button";
import { Textarea } from "@/components/ui/textarea";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Play, Upload, Trash2, FileCode } from "lucide-react";

interface CommandEditorProps {
  onExecute: (commands: string) => void;
  isLoading: boolean;
}

export function CommandEditor({ onExecute, isLoading }: CommandEditorProps) {
  const [commands, setCommands] = useState("");
  const [fileName, setFileName] = useState<string | null>(null);
  const fileInputRef = useRef<HTMLInputElement>(null);
  const textContainerRef = useRef<HTMLDivElement>(null);
  const textareaRef = useRef<HTMLTextAreaElement>(null);

  const handleScroll = (e: UIEvent<HTMLTextAreaElement>) => {
    if (textContainerRef.current) {
      textContainerRef.current.scrollTop = e.currentTarget.scrollTop;
      textContainerRef.current.scrollLeft = e.currentTarget.scrollLeft;
    }
  };

  const renderHighlightedText = () => {
    if (!commands) {
      return (
        <span className="text-muted-foreground opacity-50">
          {`Ingresa tus comandos aquí...

Ejemplos:
mkdisk -size=50 -unit=M -path="/home/user/disco1.mia"
fdisk -size=10 -unit=M -path="/home/user/disco1.mia" -name="Particion1"
mount -path="/home/user/disco1.mia" -name="Particion1"
mkfs -id=641A
login -user=root -pass=123 -id=641A

# Los comentarios inician con #`}
        </span>
      );
    }

    return commands.split('\n').map((line, i) => {
      const isComment = line.trim().startsWith('#');
      return (
        <span key={i} className={isComment ? "text-green-600 dark:text-green-500 font-semibold" : "text-foreground"}>
          {line || ' '}
          {i < commands.split('\n').length - 1 ? '\n' : ''}
        </span>
      );
    });
  };

  const handleFileUpload = useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      const file = event.target.files?.[0];
      if (file) {
        // Verificar extensión .mia
        if (!file.name.endsWith(".mia")) {
          alert("Solo se permiten archivos con extensión .mia");
          return;
        }

        const reader = new FileReader();
        reader.onload = (e) => {
          const content = e.target?.result as string;
          setCommands(content);
          setFileName(file.name);
        };
        reader.readAsText(file);
      }
    },
    []
  );

  const handleClear = () => {
    setCommands("");
    setFileName(null);
    if (fileInputRef.current) {
      fileInputRef.current.value = "";
    }
  };

  const handleExecute = () => {
    if (commands.trim()) {
      onExecute(commands);
      
      // Convertir líneas ejecutadas en comentarios para que no se vuelvan a ejecutar
      setTimeout(() => {
        setCommands((current) => {
          const lines = current.split('\n').map(line => {
            const trimmed = line.trim();
            if (!trimmed || trimmed.startsWith('#')) return line;
            return `# ${line}`;
          });
          
          // Asegurar que termine en una nueva línea para poder seguir escribiendo
          const newText = lines.join('\n') + '\n';
          
          // Enfocar después de que el estado se actualice
          setTimeout(() => {
            if (textareaRef.current) {
              textareaRef.current.focus();
              textareaRef.current.setSelectionRange(newText.length, newText.length);
            }
          }, 0);
          
          return newText;
        });
      }, 0);
    }
  };

  return (
    <Card className="h-full flex flex-col">
      <CardHeader className="pb-3">
        <div className="flex items-center justify-between">
          <CardTitle className="flex items-center gap-2 text-lg">
            <FileCode className="h-5 w-5" />
            Editor de Comandos
          </CardTitle>
          {fileName && (
            <span className="text-sm text-muted-foreground bg-muted px-2 py-1 rounded">
              {fileName}
            </span>
          )}
        </div>
      </CardHeader>
      <CardContent className="flex-1 flex flex-col gap-3 min-h-0">
        <div className="relative flex-1 min-h-0 rounded-md border border-input bg-transparent overflow-hidden">
          {/* Overlay text */}
          <div 
            ref={textContainerRef}
            aria-hidden="true"
            className={`absolute inset-0 px-3 py-2 font-mono text-sm whitespace-pre-wrap break-words overflow-hidden pointer-events-none md:text-sm ${isLoading ? "opacity-50" : ""}`}
            style={{ tabSize: 4 }}
          >
            {renderHighlightedText()}
          </div>
          
          {/* Real Input */}
          <Textarea
            ref={textareaRef}
            value={commands}
            onChange={(e) => setCommands(e.target.value)}
            onScroll={handleScroll}
            disabled={isLoading}
            spellCheck={false}
            className={`absolute inset-0 w-full h-full font-mono text-sm resize-none overflow-y-auto bg-transparent border-0 text-transparent caret-foreground outline-none focus-visible:ring-0 focus-visible:ring-offset-0 disabled:opacity-50 !shadow-none m-0 ${isLoading ? "opacity-50" : ""}`}
            placeholder="" /* Placeholder is handled by the overlay */
          />
        </div>
        <div className="flex gap-2 flex-wrap shrink-0">
          <input
            type="file"
            accept=".mia"
            onChange={handleFileUpload}
            ref={fileInputRef}
            className="hidden"
          />
          <Button
            variant="outline"
            onClick={() => fileInputRef.current?.click()}
            className="gap-2"
          >
            <Upload className="h-4 w-4" />
            Cargar .mia
          </Button>
          <Button variant="outline" onClick={handleClear} className="gap-2">
            <Trash2 className="h-4 w-4" />
            Limpiar
          </Button>
          <Button
            onClick={handleExecute}
            disabled={isLoading || !commands.trim()}
            className="gap-2 ml-auto"
          >
            <Play className="h-4 w-4" />
            {isLoading ? "Ejecutando..." : "Ejecutar"}
          </Button>
        </div>
      </CardContent>
    </Card>
  );
}
