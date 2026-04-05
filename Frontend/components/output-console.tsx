"use client";

import { useRef, useEffect } from "react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { ScrollArea } from "@/components/ui/scroll-area";
import { Terminal, Trash2, Download, CheckCircle2, XCircle, Clock } from "lucide-react";

export interface OutputLine {
  id: string;
  type: "command" | "success" | "error" | "info";
  content: string;
  timestamp: Date;
}

interface OutputConsoleProps {
  output: OutputLine[];
  onClear: () => void;
}

export function OutputConsole({ output, onClear }: OutputConsoleProps) {
  const scrollRef = useRef<HTMLDivElement>(null);

  // Auto-scroll al final cuando hay nuevo output
  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [output]);

  const handleDownload = () => {
    const content = output
      .map((line) => `[${line.timestamp.toLocaleTimeString()}] ${line.content}`)
      .join("\n");
    const blob = new Blob([content], { type: "text/plain" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `extreamfs-output-${Date.now()}.txt`;
    a.click();
    URL.revokeObjectURL(url);
  };

  const getIcon = (type: OutputLine["type"]) => {
    switch (type) {
      case "success":
        return <CheckCircle2 className="h-4 w-4 text-green-500 shrink-0" />;
      case "error":
        return <XCircle className="h-4 w-4 text-red-500 shrink-0" />;
      case "command":
        return <Terminal className="h-4 w-4 text-blue-500 shrink-0" />;
      default:
        return <Clock className="h-4 w-4 text-muted-foreground shrink-0" />;
    }
  };

  const getTextColor = (type: OutputLine["type"]) => {
    switch (type) {
      case "success":
        return "text-green-600 dark:text-green-400";
      case "error":
        return "text-red-600 dark:text-red-400";
      case "command":
        return "text-blue-600 dark:text-blue-400 font-semibold";
      default:
        return "text-foreground";
    }
  };

  return (
    <Card className="h-full flex flex-col">
      <CardHeader className="pb-3">
        <div className="flex items-center justify-between">
          <CardTitle className="flex items-center gap-2 text-lg">
            <Terminal className="h-5 w-5" />
            Consola de Salida
          </CardTitle>
          <div className="flex gap-2">
            <Button
              variant="ghost"
              size="sm"
              onClick={handleDownload}
              disabled={output.length === 0}
            >
              <Download className="h-4 w-4" />
            </Button>
            <Button
              variant="ghost"
              size="sm"
              onClick={onClear}
              disabled={output.length === 0}
            >
              <Trash2 className="h-4 w-4" />
            </Button>
          </div>
        </div>
      </CardHeader>
      <CardContent className="flex-1 p-0 relative">
        <div className="absolute inset-0">
          <ScrollArea className="h-full px-4 pb-4" ref={scrollRef}>
            {output.length === 0 ? (
            <div className="flex items-center justify-center h-full text-muted-foreground">
              <p>Los resultados de los comandos apareceran aqui...</p>
            </div>
          ) : (
            <div className="space-y-2 font-mono text-sm">
              {output.map((line) => (
                <div key={line.id} className="flex gap-2 items-start">
                  {getIcon(line.type)}
                  <div className="flex-1 min-w-0">
                    <span className="text-xs text-muted-foreground mr-2">
                      [{line.timestamp.toLocaleTimeString()}]
                    </span>
                    <span className={`${getTextColor(line.type)} whitespace-pre-wrap`}>
                      {line.content}
                    </span>
                  </div>
                </div>
              ))}
            </div>
          )}
        </ScrollArea>
        </div>
      </CardContent>
    </Card>
  );
}
