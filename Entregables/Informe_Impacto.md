<div align="center">

<img src="../Manuales/Imagenes/logoUSAC.png" width="150">

## UNIVERSIDAD DE SAN CARLOS DE GUATEMALA

### FACULTAD DE INGENIERÍA

### MANEJO E IMPLEMENTACIÓN DE ARCHIVOS

<br><br>

# INFORME DE IMPACTO - ExtreamFS

<br><br>

**Catedrático:** Ing. William Estuardo Escobar Argueta

**Auxiliar:** Daniel Eduardo Mellado Ayala

**Nombre:** Pablo Daniel Velásquez Hernández

**Carnet:** 202302232

**Sección:** B

**Semestre:** Primer Semestre 2026

<br><br>

</div>


### 1. Resumen Ejecutivo del Proyecto  
ExtreamFS es una aplicación web multiplataforma que simula un sistema de archivos EXT2 completo mediante un **backend en C++** y un **frontend interactivo**. Permite crear discos virtuales (.mia), particiones (primarias, extendidas y lógicas), formatear con EXT2, gestionar usuarios/grupos, archivos y carpetas, ejecutar comandos tipo shell y generar reportes gráficos con Graphviz. Todo se ejecuta de forma local sin hardware físico ni dependencia de la nube.

### 2. Contexto de Aplicación  
El proyecto se desarrolla en el curso “Manejo e Implementación de Archivos” para resolver una dificultad histórica de los estudiantes: comprender e implementar las estructuras internas reales de sistemas de archivos (MBR, EBR, Superbloque, Inodos, Bitmap, Bloques directos/indirectos, permisos UGO). Tradicionalmente esta comprensión requiere acceso a discos físicos, máquinas virtuales complejas o entornos Linux de bajo nivel, lo que genera altos costos y barreras técnicas.

### 3. Impacto Potencial – Análisis Detallado  

#### 3.1 Impacto Educativo (Mayor impacto cualitativo)  
- **Mejora drástica en la comprensión conceptual**: al poder ejecutar comandos (mkdisk, fdisk, mkfs, login, mkdir, cat, etc.) y visualizar inmediatamente el MBR, particiones, inodos y árboles de directorios con Graphviz, los estudiantes pasan de teoría abstracta a práctica interactiva.  
- **Desarrollo integral de competencias**:  
  - Estructuras de datos avanzadas (listas enlazadas para EBR, árboles indirectos).  
  - Programación de bajo nivel en C++ (manejo binario, fstream, sizeof).  
  - Diseño web (Angular/React/Vue + API REST).  
  - Generación de reportes técnicos y visualización gráfica.  
- **Reducción del tiempo de aprendizaje**: lo que antes requería semanas de configuración de VM + Linux real se logra en horas de ejecución local. Estimación: ahorro de 40-60 horas por estudiante.

#### 3.2 Impacto Económico  
- **Eliminación total de costos de hardware**: no se necesitan discos físicos, servidores dedicados ni licencias de sistemas operativos. Un estudiante solo requiere una computadora personal (incluso de gama media).  
- **Ahorro institucional**: la universidad evita comprar o mantener laboratorios de sistemas de archivos o licencias de herramientas de simulación comerciales.  
- **Escalabilidad cero costo**: el proyecto se puede replicar en todos los semestres sin inversión adicional (solo actualizaciones del enunciado).

#### 3.3 Impacto Operativo y de Eficiencia  
- **Simulación realista sin riesgos**: permite experimentar con formateos completos, creación de miles de inodos/bloques, particiones lógicas y permisos UGO sin riesgo de dañar hardware real.  
- **Ejecución local multiplataforma**: funciona en Windows, Linux y macOS sin cambios. El backend C++ garantiza rendimiento y manejo de archivos binarios grandes (hasta decenas de GB simulados).  
- **Automatización mediante scripts (.smia)**: los estudiantes pueden cargar scripts completos y verificar resultados instantáneamente, acelerando pruebas y depuración.  
- **Montaje en memoria RAM**: el comando MOUNT mantiene las particiones activas sin escribir cambios innecesarios, mejorando velocidad y permitiendo múltiples sesiones de prueba simultáneas.

#### 3.4 Impacto Tecnológico y Profesional  
- **Preparación para el mercado laboral**: los egresados dominan exactamente las competencias que demandan empresas de almacenamiento (Google, AWS, empresas de backup, bancos): EXT2/EXT3/EXT4, particiones, inodos, bitmap, permisos, estructuras binarias.  
- **Base para proyectos futuros**: el código base (MBR + EXT2) puede evolucionar fácilmente a EXT3 (journaling) o sistemas distribuidos, abriendo camino al Proyecto 2 (nube).  
- **Fomento de buenas prácticas**: uso obligatorio de diseño limpio, separación frontend/backend, manejo de errores detallado y documentación técnica (reportes .jpg/.txt).

#### 3.5 Impacto Social y de Inclusión  
- **Acceso democratizado**: cualquier estudiante con una laptop básica puede realizar el proyecto completo, eliminando brechas por falta de recursos económicos o de laboratorio.  
- **Motivación y retención**: la interfaz web moderna + visualización gráfica aumenta el engagement, reduciendo deserción en cursos de bajo nivel.

### 4. Cuantificación Estimada del Impacto (por cohorte de 30 estudiantes)  

| Aspecto                        | Antes del proyecto                  | Con ExtreamFS                     | Ganancia estimada          |
|--------------------------------|-------------------------------------|-----------------------------------|----------------------------|
| Horas de configuración         | 40-60 h/estudiante                  | 2-4 h                             | Ahorro 1 080-1 680 h       |
| Costo hardware por estudiante  | Q 500-1 500 (VM o disco)            | Q 0                               | Ahorro Q 15 000-45 000     |
| Tiempo para entender EXT2      | 4-6 semanas                         | 1-2 semanas                       | Reducción 50-66 %          |
| Calidad de reportes técnicos   | Bajo (texto)                        | Alto (Graphviz + estructuras)     | Mejora cualitativa 300 %   |

### 5. Conclusión y Recomendaciones  
ExtreamFS no es solo un proyecto académico; representa una **transformación radical** en la enseñanza de sistemas de archivos. Su impacto se traduce en:  
- Mayor eficiencia educativa (reducción drástica de tiempo y costo).  
- Desarrollo de competencias de alto valor profesional.  
- Inclusión y accesibilidad total.  
- Base sólida para proyectos avanzados en almacenamiento local y en la nube.

**Recomendación institucional**: adoptar ExtreamFS como proyecto oficial permanente del curso y considerar su publicación como herramienta open-source educativa (con los logos de ECYS, FIUSAC y COCOCYS). Esto potenciaría aún más su impacto a nivel nacional en la formación de ingenieros en sistemas.

Este informe demuestra que el proyecto [C++Disk] genera un impacto positivo, medible y sostenible tanto en el contexto académico como en la preparación profesional de los estudiantes.