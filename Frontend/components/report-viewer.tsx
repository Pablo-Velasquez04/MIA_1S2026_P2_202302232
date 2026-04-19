"use client";

import { useState } from "react";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogTrigger } from "@/components/ui/dialog";
import { ScrollArea } from "@/components/ui/scroll-area";
import { FileImage, Maximize2, Download, X, ExternalLink } from "lucide-react";

export interface Report {
  id: string;
  name: string;
  type: string;
  path: string;
  imageUrl: string;
  generatedAt: Date;
}

interface ReportViewerProps {
  reports: Report[];
  onRemoveReport: (id: string) => void;
}

export function ReportViewer({ reports, onRemoveReport }: ReportViewerProps) {
  const [selectedReport, setSelectedReport] = useState<Report | null>(null);

  const handleDownload = (report: Report) => {
    const a = document.createElement("a");
    a.href = report.imageUrl;
    a.download = `${report.name}-${Date.now()}.png`;
    a.click();
  };

  const getReportTypeLabel = (type: string) => {
    const labels: Record<string, string> = {
      mbr: "MBR",
      disk: "Disco",
      inode: "Inodos",
      block: "Bloques",
      bm_inode: "Bitmap Inodos",
      bm_block: "Bitmap Bloques",
      tree: "Arbol EXT2",
      sb: "SuperBloque",
      file: "Archivo",
      ls: "Listado",
    };
    return labels[type] || type.toUpperCase();
  };

  return (
    <Card className="h-full">
      <CardHeader className="pb-3">
        <CardTitle className="flex items-center gap-2 text-lg">
          <FileImage className="h-5 w-5" />
          Reportes Generados
        </CardTitle>
      </CardHeader>
      <CardContent>
        <ScrollArea className="h-[200px]">
          {reports.length === 0 ? (
            <div className="flex flex-col items-center justify-center h-full text-muted-foreground py-8">
              <FileImage className="h-8 w-8 mb-2 opacity-50" />
              <p className="text-sm">No hay reportes generados</p>
              <p className="text-xs mt-1">Usa el comando REP para generar reportes</p>
            </div>
          ) : (
            <div className="grid grid-cols-2 gap-3">
              {reports.map((report) => (
                <div
                  key={report.id}
                  className="relative group rounded-lg border overflow-hidden bg-muted/30"
                >
                  <div className="aspect-video bg-muted flex items-center justify-center">
                    {/* eslint-disable-next-line @next/next/no-img-element */}
                    <img
                      src={`${report.imageUrl}?t=${new Date(report.generatedAt).getTime()}`}
                      alt={report.name}
                      className="w-full h-full object-contain"
                      onError={(e) => {
                        e.currentTarget.style.display = "none";
                      }}
                    />
                  </div>
                  <div className="absolute inset-0 bg-black/60 opacity-0 group-hover:opacity-100 transition-opacity flex items-center justify-center gap-2">
                    <Dialog>
                      <DialogTrigger asChild>
                        <Button
                          size="sm"
                          variant="secondary"
                          onClick={() => setSelectedReport(report)}
                        >
                          <Maximize2 className="h-4 w-4" />
                        </Button>
                      </DialogTrigger>
                      <DialogContent className="max-w-4xl max-h-[90vh]">
                        <DialogHeader>
                          <DialogTitle>
                            {report.name} - {getReportTypeLabel(report.type)}
                          </DialogTitle>
                        </DialogHeader>
                        <ScrollArea className="max-h-[70vh]">
                          {/* eslint-disable-next-line @next/next/no-img-element */}
                          <img
                            src={`${report.imageUrl}?t=${new Date(report.generatedAt).getTime()}`}
                            alt={report.name}
                            className="w-full h-auto"
                          />
                        </ScrollArea>
                      </DialogContent>
                    </Dialog>
                    <Button
                      size="sm"
                      variant="secondary"
                      onClick={() => handleDownload(report)}
                    >
                      <Download className="h-4 w-4" />
                    </Button>
                    <Button
                      size="sm"
                      variant="secondary"
                      onClick={() => window.open(report.imageUrl, "_blank")}
                    >
                      <ExternalLink className="h-4 w-4" />
                    </Button>
                  </div>
                  <button
                    onClick={() => onRemoveReport(report.id)}
                    className="absolute top-1 right-1 h-6 w-6 rounded-full bg-destructive text-destructive-foreground opacity-0 group-hover:opacity-100 transition-opacity flex items-center justify-center"
                  >
                    <X className="h-3 w-3" />
                  </button>
                  <div className="p-2">
                    <p className="text-xs font-medium truncate">{report.name}</p>
                    <p className="text-xs text-muted-foreground">
                      {getReportTypeLabel(report.type)}
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
