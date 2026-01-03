# Visión del Proyecto – Validación de UniversalPointerCore DLL

## Objetivo Principal
**Probar y explorar los alcances de la DLL `UniversalPointerCore_V2`** para validar su uso en aplicaciones interactivas 2D y 3D.

La DLL transforma orientación 3D (quaterniones de sensores móviles) en coordenadas de pantalla precisas, con filtrado de ruido y calibración avanzada.

---

## Estado Actual

### ✅ Completado
- **Integración Python**: `main.py` consume la DLL vía `ctypes`.
- **Servidor WebSocket**: Transmite datos del móvil al visualizador 3D.
- **Visor 3D (Three.js)**: Escena con paredes de cemento realistas.
- **Efecto Video Reveal**: La "linterna" revela un video oculto en las paredes.
- **Documentación DLL**: `dll/Documentacion.txt` actualizada con API V2.

### 🔬 Experimentos Activos
- CSG para proyección de luz sobre geometría compleja.
- Shaders personalizados para efectos visuales (bordes suaves, VJ patterns).

---

## Métricas de Éxito
| Métrica | Objetivo |
|---------|----------|
| Error de posición | < 2 cm en escenarios 3D |
| Latencia | < 30 ms por actualización |
| Compatibilidad | Funcionar en web (Three.js) + nativo (Unity/Unreal) |

---

## Archivos Clave
- `dll/Documentacion.txt` – Referencia técnica de la DLL.
- `main.py` – Uso de la DLL en Python.
- `sensor_server/public/viewer.html` – Visualizador 3D de pruebas.
- `docs/experiment_docs.md` – Notas técnicas de experimentos CSG.

---

*Este documento guía el desarrollo. Actualizar conforme avancen las pruebas.*
