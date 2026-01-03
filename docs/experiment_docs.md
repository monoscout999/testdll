# Documentación del Experimento: Intersección Booleana CSG

Este experimento demuestra una implementación de alto rendimiento para el cálculo de intersecciones booleanas en tiempo real entre un volumen (Cono) y múltiples superficies (Suelo y Paredes) utilizando Three.js.

## Dependencias

El proyecto utiliza las siguientes librerías cargadas vía `importmap` desde CDNs (no requiere instalación local si se corre como HTML):

1.  **Three.js (v0.160.0)**: Motor 3D principal.
2.  **three-mesh-bvh (v0.7.3)**: Implementación de jerarquías de volumen delimitador (BVH) para Three.js. Es fundamental para el rendimiento en cálculos espaciales.
3.  **three-bvh-csg (v0.0.17)**: Motor de CSG (Constructive Solid Geometry) que utiliza BVH para permitir operaciones booleanas (unión, intersección, sustracción) en tiempo real.

## Configuración de la Escena

### Geometría y Coordenadas
-   **Target (Superficie)**: Es una unión (`ADDITION`) de tres cajas extremadamente delgadas que actúan como planos:
    -   **Suelo**: Centrado en $(0, 0, 0)$ con dimensiones $10 \times 10$.
    -   **Pared Trasera**: Ubicada en $z = -5$, extendiéndose de $y=0$ a $y=10$.
    -   **Pared Derecha**: Ubicada en $x = 5$, extendiéndose de $y=0$ a $y=10$.
-   **Herramienta (Cono)**: Un `ConeGeometry` con la punta anclada en un pivote ubicado en $(0, 5, 0)$.

### Lógica de Intersección
Se utiliza la función `evaluator.evaluate()` con la operación `INTERSECTION`. El proceso se repite cada vez que hay un cambio en la rotación o escala del cono:
1.  Se actualizan las matrices del mundo de los objetos.
2.  Se calcula la intersección entre el `targetBrush` (las paredes) y el `coneBrush`.
3.  El resultado es una geometría de líneas verdes (`MeshBasicMaterial` con `wireframe: true`).

## Controles

-   **Navegación**: Los `OrbitControls` de Three.js permiten rotar la cámara con el mouse sin afectar la posición de los objetos.
-   **Cruzeta (D-pad)**: Controla la rotación del pivote del cono en los ejes X y Z. Al estar anclado en la punta, el cono "barre" las superficies.
-   **Deslizador de Diámetro**: Escala uniformemente los ejes X y Z del cono para ajustar su grosor.

## Notas Técnicas Críticas

> [!IMPORTANT]
> Para que el motor CSG reconozca cambios de posición, es obligatorio llamar a `updateMatrixWorld()` en los Brushes antes de cada evaluación si estos han sido movidos.

> [!NOTE]
> La jerarquía de objetos utiliza un `Group` como pivote para facilitar la rotación desde la punta del cono, compensando el offset natural de `ConeGeometry` (cuyo centroide está en la mitad de su altura).
