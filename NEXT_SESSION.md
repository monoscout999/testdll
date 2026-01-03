# Tareas pendientes para próxima sesión

## 1. Implementar funciones del DLL en el móvil

### Suavizado (smooth)
- **Ubicación actual**: DLL (UniversalPointerCore_V2.dll) - parámetro `smooth` (actualmente 0.8)
- **Objetivo**: Implementar este filtro de suavizado exponencial en el móvil
- **Beneficio**: Reducir ruido de sensores directamente en origen
- **Archivo a modificar**: Aplicación móvil que envía datos de orientación

### Distancia (distance)
- **Ubicación actual**: DLL - parámetro `distance` (actualmente 1.5m - distancia usuario a pantalla)
- **Objetivo**: Implementar el cálculo de distancia en el móvil
- **Beneficio**: Pre-calcular coordenadas en el móvil antes de enviar
- **Archivo a modificar**: Aplicación móvil que envía datos de orientación

## Notas técnicas

### Parámetros DLL actuales (main.py):
```python
self.smooth = 0.8      # Filtro de suavizado exponencial
self.distance = 1.5    # Distancia usuario-pantalla en metros
self.screen_width = 1.2   # Ancho pantalla virtual
self.screen_height = 0.9  # Alto pantalla virtual
```

### Estructura de datos actual:
- **Del móvil**: quaternion (orientación)
- **DLL procesa**: quaternion → dirX/Y/Z + worldX/Y/Z
- **Objetivo**: Mover parte del procesamiento al móvil

## Estado actual del proyecto

### Completado ✓
- Sistema de decal 3D con radio 0.6m
- Mesh que sigue geometría de paredes
- Interpolación suave en transiciones entre superficies (INTERPOLATION_SPEED = 0.08)
- Modo wireframe para visualización
- Panel de información en tiempo real
- Sistema de raycasting multi-superficie

### Funcionando bien
- Transiciones entre paredes (con smoothstep easing)
- Detección de esquinas y bordes
- Generación de geometría adaptativa
- Visualización 3D con Three.js
