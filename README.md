# UniversalPointer DLL Test Environment

Entorno de pruebas para validar **UniversalPointerCore_V2.dll** - una librería C++ que transforma orientación 3D en coordenadas de pantalla.

## 🚀 Quick Start

```bash
# 1. Iniciar servidor
cd sensor_server && npm install && npm start

# 2. Iniciar aplicación Python
python main.py

# 3. Abrir visualizador
# http://localhost:3000/viewer.html
```

## 📁 Estructura

```
├── dll/                    # DLL y documentación
│   ├── UniversalPointerCore_V2.dll
│   └── Documentacion.txt   # API Reference
├── sensor_server/          # Servidor WebSocket + Viewer
│   └── public/viewer.html  # Visualizador 3D
├── docs/                   # Documentación técnica
├── main.py                 # Integración Python
└── Vision.md               # Objetivos del proyecto
```

## 📖 Documentación

- **[Vision.md](Vision.md)** - Objetivo y estado del proyecto
- **[DLL API](dll/Documentacion.txt)** - Referencia técnica completa
- **[Experimentos CSG](docs/experiment_docs.md)** - Notas de implementación 3D

## 🎯 Objetivo

Validar que la DLL puede ser utilizada en aplicaciones interactivas con:
- Precisión < 2cm
- Latencia < 30ms
- Compatibilidad web y nativa
