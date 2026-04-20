"use client";

import { useEffect, useRef, useState } from "react";
import { Badge } from "@/components/ui/badge";
import { Wifi, WifiOff, RefreshCw } from "lucide-react";
import { Button } from "@/components/ui/button";
import { checkConnection } from "@/lib/api";

interface ConnectionStatusProps {
  onStatusChange?: (connected: boolean) => void;
  onReconnect?: () => void; // Dispara cuando el backend vuelve tras haber estado caído
}

export function ConnectionStatus({ onStatusChange, onReconnect }: ConnectionStatusProps) {
  const [isConnected, setIsConnected] = useState<boolean | null>(null);
  const [isChecking, setIsChecking] = useState(false);
  // Ref para saber el estado ANTERIOR sin causar re-renders extras
  const prevConnectedRef = useRef<boolean | null>(null);

  const checkBackendConnection = async () => {
    setIsChecking(true);
    try {
      const connected = await checkConnection();

      // Disparar cleanup en cualquier transición hacia conectado
      // Esto cubre tanto reconexión (false→true) como primer arranque (null→true)
      if (connected && prevConnectedRef.current !== true) {
        onReconnect?.();
      }

      prevConnectedRef.current = connected;
      setIsConnected(connected);
      onStatusChange?.(connected);
    } catch {
      prevConnectedRef.current = false;
      setIsConnected(false);
      onStatusChange?.(false);
    } finally {
      setIsChecking(false);
    }
  };

  useEffect(() => {
    checkBackendConnection();
    
    // Verificar conexión cada 15 segundos
    const interval = setInterval(checkBackendConnection, 15000);
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
