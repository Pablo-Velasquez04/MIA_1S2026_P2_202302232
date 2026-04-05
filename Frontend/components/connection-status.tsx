"use client";

import { useEffect, useState } from "react";
import { Badge } from "@/components/ui/badge";
import { Wifi, WifiOff, RefreshCw } from "lucide-react";
import { Button } from "@/components/ui/button";
import { checkConnection } from "@/lib/api";

interface ConnectionStatusProps {
  onStatusChange?: (connected: boolean) => void;
}

export function ConnectionStatus({ onStatusChange }: ConnectionStatusProps) {
  const [isConnected, setIsConnected] = useState<boolean | null>(null);
  const [isChecking, setIsChecking] = useState(false);

  const checkBackendConnection = async () => {
    setIsChecking(true);
    try {
      const connected = await checkConnection();
      setIsConnected(connected);
      onStatusChange?.(connected);
    } catch {
      setIsConnected(false);
      onStatusChange?.(false);
    } finally {
      setIsChecking(false);
    }
  };

  useEffect(() => {
    checkBackendConnection();
    
    // Verificar conexión cada 30 segundos
    const interval = setInterval(checkBackendConnection, 30000);
    return () => clearInterval(interval);
  }, []);

  return (
    <div className="flex items-center gap-2">
      <Badge
        variant={isConnected === null ? "secondary" : isConnected ? "default" : "destructive"}
        className="gap-1"
      >
        {isConnected === null ? (
          <>
            <RefreshCw className="h-3 w-3 animate-spin" />
            Verificando...
          </>
        ) : isConnected ? (
          <>
            <Wifi className="h-3 w-3" />
            Backend Conectado
          </>
        ) : (
          <>
            <WifiOff className="h-3 w-3" />
            Sin Conexion
          </>
        )}
      </Badge>
      <Button
        variant="ghost"
        size="sm"
        onClick={checkBackendConnection}
        disabled={isChecking}
        className="h-7 w-7 p-0"
      >
        <RefreshCw className={`h-3 w-3 ${isChecking ? "animate-spin" : ""}`} />
      </Button>
    </div>
  );
}
