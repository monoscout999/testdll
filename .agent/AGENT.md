# Agent Instructions

> **Primary Reference**: Read `Vision.md` before any action.

## Project Goal
Testing and exploring the capabilities of **UniversalPointerCore_V2.dll** - a C++ library that converts 3D orientation (quaternions) into 2D screen coordinates.

## Key Files
| File | Purpose |
|------|---------|
| `Vision.md` | Project vision and objectives |
| `dll/Documentacion.txt` | DLL API reference |
| `main.py` | Python integration example |
| `sensor_server/public/viewer.html` | 3D visualization testbed |

## Current State
- DLL V2 functional and documented
- 3D viewer with CSG-based projections
- Video reveal effect implemented

## Rules
1. Always consult `Vision.md` for project context.
2. DLL documentation is authoritative for API questions.
3. Test changes in `viewer.html` before modifying `main.py`.
