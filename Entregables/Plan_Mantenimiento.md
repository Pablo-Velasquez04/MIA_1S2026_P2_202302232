<div align="center">

<img src="../Manuales/Imagenes/logoUSAC.png" width="150">

## UNIVERSIDAD DE SAN CARLOS DE GUATEMALA

### FACULTAD DE INGENIERÍA

### MANEJO E IMPLEMENTACIÓN DE ARCHIVOS

<br><br>

# PLAN DE MANTENIMIENTO - ExtreamFS

<br><br>

**Catedrático:** Ing. William Estuardo Escobar Argueta

**Auxiliar:** Daniel Eduardo Mellado Ayala

**Nombre:** Pablo Daniel Velásquez Hernández

**Carnet:** 202302232

**Sección:** B

**Semestre:** Primer Semestre 2026

<br><br>

</div>


### 1. Introducción  
ExtreamFS es una aplicación web completa (frontend + backend C++) que simula un sistema de archivos EXT2 mediante archivos binarios .mia. Al estar ya realizado y entregado, se requiere un **plan de mantenimiento** que garantice su correcto funcionamiento a largo plazo, corrija posibles errores, adapte el sistema a nuevos entornos y mantenga la calidad del código y la documentación.

El plan cubre un período de **12 meses** posteriores a la entrega final (marzo 2026 – marzo 2027) y está diseñado para ser ejecutado por el desarrollador (o equipo de mantenimiento) con recursos mínimos.

### 2. Objetivos del Plan de Mantenimiento  
- Garantizar la estabilidad y disponibilidad del sistema en un 98 % o superior.  
- Corregir errores detectados en producción (comandos, estructuras EXT2, reportes Graphviz).  
- Adaptar la aplicación a nuevos navegadores, versiones de C++ (C++20/23) y sistemas operativos.  
- Optimizar rendimiento para discos > 5 GB y mejorar la experiencia de usuario.  
- Mantener actualizada la documentación técnica y los reportes visuales.  
- Preparar la base para futuras extensiones (Proyecto 2 – nube).

### 3. Alcance  
**Incluido:**  
- Backend C++ (manejo de .mia, MBR, EBR, Superbloque, Inodos, Bloques, Bitmap, usuarios.txt).  
- Frontend web (área de comandos, carga de .smia, ejecución API REST).  
- Generación de reportes (Graphviz .jpg/.txt).  
- Comandos completos (mkdisk, fdisk, mkfs, login, mkusr, cat, etc.).  
- Archivos binarios y estructuras en memoria RAM (MOUNT).  
- Despliegue en la nube: Dicha página web deberá ser desplegada mediante el servicio de bucket S3 de AWS, dicho backend deberá ser desplegado en una instancia EC2 de AWS, y se gestionarán los accesos y permisos mediante IAM.

**Excluido:**  
- Hardware físico real.  
- Soporte a múltiples usuarios simultáneos (fuera del alcance del Proyecto 1).

### 4. Tipos de Mantenimiento y Estrategia  

| Tipo de Mantenimiento | Descripción | Frecuencia | Acciones Principales | Prioridad |
|-----------------------|-------------|------------|----------------------|---------|
| **Correctivo** | Corrección de errores (bugs en parsing de comandos, corrupción de .mia, fallos en inodos indirectos, etc.) | Según reporte de usuario o detección | Análisis de logs, reproducción del error, parche y prueba unitaria | Alta |
| **Preventivo** | Revisión proactiva de código y estructuras | Cada 2 meses | Revisión de memory leaks, validación de tamaños de structs, chequeo de bitmaps | Media |
| **Adaptativo** | Adaptación a cambios externos (nuevos compiladores, navegadores, SO) | Cada 4 meses o ante cambio mayor | Actualización CMake, compatibilidad Angular/React, pruebas en Windows/Linux/macOS | Media |
| **Perfectivo** | Mejoras de rendimiento y funcionalidad | Cada 6 meses | Optimización de lectura/escritura binaria, caché de comandos frecuentes, nuevo tema UI | Baja |

### 5. Responsables  
- **Desarrollador Principal (Estudiante):** Pablo – responsable técnico y ejecución.  
- **Revisor Técnico:** Docente o asistente del curso (opcional).  
- **Usuario Final:** Estudiantes de semestres posteriores (reportan errores).  

### 6. Cronograma de Mantenimiento (12 meses)  

| Mes | Tipo de Mantenimiento | Actividades Principales | Entregable |
|-----|-----------------------|-------------------------|------------|
| 1-2 | Correctivo + Preventivo | Revisión inicial post-entrega, corrección de bugs críticos, backup de estructuras de prueba | Reporte de bugs corregidos + versión 1.1 |
| 3-4 | Adaptativo | Actualización a C++23 y pruebas en nuevos navegadores (Chrome 130+, Edge, Firefox) | Versión 1.2 + documentación actualizada |
| 5-6 | Perfectivo + Preventivo | Optimización de mkfs y reportes Graphviz, revisión de permisos UGO | Versión 1.3 + 3 mejoras de rendimiento |
| 7-8 | Correctivo | Atención a reportes de usuarios (semestre 2S2026) | Parches 1.3.x |
| 9-10 | Adaptativo + Preventivo | Soporte a macOS Sequoia y Angular 19 (si aplica) | Versión 1.4 |
| 11-12 | Perfectivo + Cierre | Mejoras finales, generación de manual de mantenimiento, evaluación de impacto | Versión final 2.0 + Informe de cierre |

### 7. Procedimientos Detallados  
1. **Recepción de reporte** → Usuario reporta vía GitHub Issues o formulario web.  
2. **Reproducción** → Se recrea el error con el mismo .mia y comando exacto.  
3. **Diagnóstico** → Uso de herramientas: Valgrind (memory), GDB (debug), Hex editor para .mia.  
4. **Corrección** → Código modular (archivos separados por estructura: mbr.h, inode.h, etc.).  
5. **Pruebas** → 100 % cobertura de comandos críticos + scripts de prueba (.smia).  
6. **Despliegue** → Nueva versión en repositorio + actualización del frontend.  
7. **Documentación** → Actualizar README.md y manual técnico (incluye estructuras EXT2).  

**Herramientas recomendadas:**  
- Git + GitHub (control de versiones)  
- CMake + Make (compilación C++)  
- Postman / Insomnia (pruebas API REST)  
- Graphviz (validación de reportes)  
- Valgrind + GDB (depuración)  
- Google Test (unit tests)

### 8. Recursos Necesarios  
- **Hardware:** Laptop personal (mínimo 8 GB RAM, 50 GB disco).  
- **Software:** Gratuito (VS Code, CMake, Angular/React, Graphviz, Git).  
- **Infraestructura Cloud (AWS):** Bucket S3 (frontend), Instancia EC2 (backend) e IAM (gestión de identidades y accesos).
- **Tiempo estimado:** 8-12 horas/mes (2-3 horas/semana).  
- **Costo estimado:** Q 0 (aprovechando la capa gratuita de AWS, herramientas open-source y local).

### 9. Métricas de Monitoreo y Éxito  
- **Disponibilidad:** ≥ 98 % de comandos ejecutados sin error.  
- **Tiempo de respuesta:** mkfs y reportes < 5 segundos en disco de 1 GB.  
- **Bugs abiertos:** máximo 2 críticos pendientes.  
- **Satisfacción usuario:** encuesta semestral (nota ≥ 4.5/5).  
- **Cobertura de código:** ≥ 85 % en backend C++.

### 10. Riesgos y Mitigación  
- Riesgo: Corrupción de archivos .mia grandes → Mitigación: backups automáticos antes de cada operación crítica.  
- Riesgo: Cambio de framework frontend → Mitigación: mantener API REST estable.  
- Riesgo: Pérdida de conocimiento → Mitigación: documentación exhaustiva y comentarios en código.

### 11. Conclusión  
El Plan de Mantenimiento de ExtreamFS asegura que el proyecto no solo cumpla con los requisitos del enunciado del curso, sino que permanezca funcional, estable y listo para ser utilizado por futuras generaciones de estudiantes. Con una inversión mínima de tiempo y cero costo adicional, se garantiza la sostenibilidad del sistema durante al menos 12 meses, facilitando su evolución hacia el Proyecto 2 (integración en la nube).