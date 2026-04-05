// Configuración de la API del backend C++
// IMPORTANTE: Cambia esta URL a la dirección donde corre tu servidor C++

const API_BASE_URL = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:8080';

export interface CommandResponse {
  success: boolean;
  output: string;
  error?: string;
  reportPath?: string; // Ruta del reporte generado (si aplica)
}

export interface MountedPartition {
  id: string;
  name: string;
  path: string;
  size: number;
  type: string;
}

export interface DiskInfo {
  path: string;
  size: number;
  partitions: MountedPartition[];
}

// Ejecutar un comando individual
export async function executeCommand(command: string): Promise<CommandResponse> {
  try {
    const response = await fetch(`${API_BASE_URL}/api/execute`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ command }),
    });

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    return await response.json();
  } catch (error) {
    return {
      success: false,
      output: '',
      error: `Error de conexión: ${error instanceof Error ? error.message : 'Unknown error'}`,
    };
  }
}

// Ejecutar múltiples comandos (script .mia)
export async function executeScript(commands: string[]): Promise<CommandResponse[]> {
  try {
    const response = await fetch(`${API_BASE_URL}/api/execute-script`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({ commands }),
    });

    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    return await response.json();
  } catch (error) {
    // Si el endpoint de script no existe, ejecutar comando por comando
    const results: CommandResponse[] = [];
    for (const cmd of commands) {
      const result = await executeCommand(cmd);
      results.push(result);
    }
    return results;
  }
}

// Obtener particiones montadas
export async function getMountedPartitions(): Promise<MountedPartition[]> {
  try {
    const response = await fetch(`${API_BASE_URL}/api/mounted`);
    
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    return await response.json();
  } catch (error) {
    console.error('Error fetching mounted partitions:', error);
    return [];
  }
}

// Obtener un reporte generado (imagen)
export function getReportUrl(reportPath: string): string {
  return `${API_BASE_URL}/api/reports/${encodeURIComponent(reportPath)}`;
}

// Verificar conexión con el backend
export async function checkConnection(): Promise<boolean> {
  try {
    // Agregar timestamp para evitar cache del navegador
    const response = await fetch(`${API_BASE_URL}/api/health?t=${Date.now()}`, {
      method: 'GET',
    });
    
    if (!response.ok) return false;
    
    const data = await response.json();
    // Verificar que sea nuestro backend específicamente
    return data && data.status === "ok" && data.message && data.message.includes("ExtreamFS");
  } catch {
    return false;
  }
}
