/*
 * UniversalPointerCore_V2.cpp
 * --------------------------------------------------------------------
 * Middleware de Puntero 3D "World Class"
 * Versión: 2.0.0
 * * Mejoras V2:
 * - Quaternion Convention Management
 * - Input Smoothing (Exponential Filter)
 * - Advanced Projection (Offset & 2-Point Frustum)
 * - Developer Logging Callback
 * - Extended Output (Yaw/Pitch)
 */

#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cstdarg>

// =======================================================================
// 1. MACROS & CONSTANTES
// =======================================================================
#if defined(_MSC_VER)
    #define DLL_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
    #define DLL_EXPORT __attribute__((visibility("default")))
#else
    #define DLL_EXPORT
#endif

#define PI 3.14159265359f
#define DEG_TO_RAD (PI / 180.0f)
#define RAD_TO_DEG (180.0f / PI)

// =======================================================================
// 2. TIPOS DE DATOS & ENUMS
// =======================================================================

enum QuaternionMode {
    MODE_W_FIRST = 0, // W, X, Y, Z (Unity, Standards)
    MODE_W_LAST = 1   // X, Y, Z, W (Algunos sensores raw)
};

enum CalibrationMode {
    CALIB_MODE_CENTER = 0, // Un punto (Centro) + Geometría física
    CALIB_MODE_CORNERS = 1 // Dos puntos (TopLeft, BottomRight) - "Frustum"
};

// Callback para logs: void func(const char* msg)
typedef void (*LogCallback)(const char*);

struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& v) const { return {x+v.x, y+v.y, z+v.z}; }
    Vector3 operator-(const Vector3& v) const { return {x-v.x, y-v.y, z-v.z}; }
    Vector3 operator*(float s) const { return {x*s, y*s, z*s}; }
    
    float Magnitude() const { return std::sqrt(x*x + y*y + z*z); }
    
    void Normalize() {
        float m = Magnitude();
        if (m > 1e-6f) { x/=m; y/=m; z/=m; } 
        else { x=0; y=0; z=1; }
    }

    static Vector3 Cross(const Vector3& a, const Vector3& b) {
        return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
    }

    static float Dot(const Vector3& a, const Vector3& b) {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }

    // Interpolación lineal para suavizado
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return { a.x + (b.x - a.x)*t, a.y + (b.y - a.y)*t, a.z + (b.z - a.z)*t };
    }
};

struct Quaternion {
    float x, y, z, w;

    Vector3 Rotate(const Vector3& v) const {
        Vector3 qv = {x, y, z};
        Vector3 uv = Vector3::Cross(qv, v);
        Vector3 uuv = Vector3::Cross(qv, uv);
        return v + (uv * (2.0f * w)) + (uuv * 2.0f);
    }

    Quaternion Inverse() const { return {-x, -y, -z, w}; }

    Quaternion Multiply(const Quaternion& q) const {
        return {
            w*q.x + x*q.w + y*q.z - z*q.y,
            w*q.y - x*q.z + y*q.w + z*q.x,
            w*q.z + x*q.y - y*q.x + z*q.w,
            w*q.w - x*q.x - y*q.y - z*q.z
        };
    }
    
    // Interpolación Esférica (Slerp) simplificada o Lerp para quaterniones cercanos
    static Quaternion Lerp(const Quaternion& a, const Quaternion& b, float t) {
        Quaternion q;
        // Check dot product for shortest path
        float dot = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
        float sign = (dot < 0.0f) ? -1.0f : 1.0f;
        
        q.x = a.x + (b.x * sign - a.x) * t;
        q.y = a.y + (b.y * sign - a.y) * t;
        q.z = a.z + (b.z * sign - a.z) * t;
        q.w = a.w + (b.w * sign - a.w) * t;
        
        // Normalizar
        float m = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
        if (m > 0) { q.x/=m; q.y/=m; q.z/=m; q.w/=m; }
        return q;
    }
};

// =======================================================================
// 3. CLASE PRINCIPAL (CORE V2)
// =======================================================================

class UniversalPointerCoreV2 {
private:
    // --- Configuración ---
    float screenWidth, screenHeight, screenDistance;
    float offsetX, offsetY;
    float smoothingFactor; // 0.0 (No filter) a 0.99 (High lag/smooth)
    
    QuaternionMode quatMode;
    CalibrationMode calibMode;
    LogCallback logger;

    // --- Estado ---
    Quaternion calibrationOffset; // Para modo CENTER
    Quaternion lastRawInput;      // Para Smoothing
    bool hasHistory;              // Primer frame check
    
    // --- Estado para Modo 2-Point (Frustum) ---
    // En lugar de calcular geometría, definimos los límites angulares (UV mapping directo)
    // Guardamos la rotación "base" (centro virtual) y los rangos tangenciales.
    Quaternion frustumCenter;
    float frustumTanMinX, frustumTanMaxX;
    float frustumTanMinY, frustumTanMaxY;

    // --- Helpers ---
    void Log(const char* fmt, ...) {
        if (!logger) return;
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        logger(buffer);
    }

public:
    UniversalPointerCoreV2() {
        // Defaults
        screenWidth = 1.0f; screenHeight = 1.0f; screenDistance = 1.0f;
        offsetX = 0.0f; offsetY = 0.0f;
        smoothingFactor = 0.0f;
        quatMode = MODE_W_LAST; // Default genérico
        calibMode = CALIB_MODE_CENTER;
        calibrationOffset = {0,0,0,1};
        frustumCenter = {0,0,0,1};
        hasHistory = false;
        logger = nullptr;
    }

    void SetLogger(LogCallback cb) { 
        logger = cb; 
        Log("Logger conectado. Core V2 inicializado.");
    }

    void SetQuaternionMode(int mode) {
        if(mode < 0 || mode > 1) { Log("Error: Invalid Quaternion Mode"); return; }
        quatMode = (QuaternionMode)mode;
    }

    // Configuración estándar (Pantalla Plana + Offset)
    void Configure(float w, float h, float dist, float offX, float offY, float smooth) {
        if(w <= 0 || h <= 0 || dist <= 0) { Log("Error: Dimensiones invalidas"); return; }
        screenWidth = w; screenHeight = h; screenDistance = dist;
        offsetX = offX; offsetY = offY;
        smoothingFactor = std::max(0.0f, std::min(0.99f, smooth));
        
        calibMode = CALIB_MODE_CENTER; // Asumimos modo físico por defecto al configurar dims
        Log("Configurado: %.2fx%.2f dist:%.2f smooth:%.2f", w, h, dist, smooth);
    }

    // Calibración 1 Punto (Centro)
    void CalibrateCenter(float q1, float q2, float q3, float q4) {
        Quaternion input = NormalizeInput(q1, q2, q3, q4);
        calibrationOffset = input.Inverse();
        calibMode = CALIB_MODE_CENTER;
        Log("Calibrado: CENTRO");
    }

    // Calibración 2 Puntos (Esquinas)
    // Esto define una "Ventana Mágica". No importa la distancia real.
    // Mapea el ángulo de TL a (0,1) y BR a (1,0) (o según coordenadas UV deseadas)
    void CalibrateCorners(float tl1, float tl2, float tl3, float tl4, 
                          float br1, float br2, float br3, float br4) {
        Quaternion qTL = NormalizeInput(tl1, tl2, tl3, tl4);
        Quaternion qBR = NormalizeInput(br1, br2, br3, br4);

        // 1. El "Centro Virtual" es la interpolación media entre las dos esquinas
        frustumCenter = Quaternion::Lerp(qTL, qBR, 0.5f); 
        
        // 2. Calculamos los vectores dirección relativos a este centro
        Quaternion invCenter = frustumCenter.Inverse();
        Vector3 vTL = (invCenter.Multiply(qTL)).Rotate({0,0,1});
        Vector3 vBR = (invCenter.Multiply(qBR)).Rotate({0,0,1});

        // 3. Proyectamos a Tangentes (Plano Z=1)
        // x = dir.x / dir.z, y = dir.y / dir.z
        if (vTL.z < 0.01f || vBR.z < 0.01f) { Log("Error: Calibracion 2 puntos invalida (angulos extremos)"); return; }

        float tlx = vTL.x / vTL.z; 
        float tly = vTL.y / vTL.z;
        float brx = vBR.x / vBR.z; 
        float bry = vBR.y / vBR.z;

        // 4. Guardamos los límites del frustum
        // Nota: Asumiendo TL es (0, 1) en UV y BR es (1, 0) en UV
        // UV Y crece hacia arriba en este motor (0 abajo, 1 arriba).
        // TL debería tener Y positivo, BR Y negativo relativo al centro.
        frustumTanMinX = tlx; // Izquierda
        frustumTanMaxX = brx; // Derecha
        frustumTanMaxY = tly; // Arriba (Top)
        frustumTanMinY = bry; // Abajo (Bottom)

        calibMode = CALIB_MODE_CORNERS;
        Log("Calibrado: CORNERS (Frustum Custom)");
    }

    // Estructura de Salida V2
    struct ResultV2 {
        float u, v;
        float worldX, worldY, worldZ;
        float dirX, dirY, dirZ;
        float yawRel, pitchRel; // Developer Experience++
        bool isValid;
    };

    ResultV2 Process(float q1, float q2, float q3, float q4) {
        ResultV2 res = {0};
        
        // 1. Ingesta y Normalización
        Quaternion raw = NormalizeInput(q1, q2, q3, q4);

        // 2. Smoothing (Filtro Pasa-Bajos Exponencial)
        if (hasHistory && smoothingFactor > 0.0f) {
            raw = Quaternion::Lerp(lastRawInput, raw, 1.0f - smoothingFactor);
        }
        lastRawInput = raw;
        hasHistory = true;

        // 3. Generar Dirección
        // Dependiendo del modo de calibración, la "base" es diferente
        Quaternion corrected;
        Vector3 dir;

        if (calibMode == CALIB_MODE_CENTER) {
            corrected = calibrationOffset.Multiply(raw);
            dir = corrected.Rotate({0,0,1}); // Z+ Forward
        } else {
            // En modo corners, rotamos relativo al "centro del frustum"
            corrected = frustumCenter.Inverse().Multiply(raw);
            dir = corrected.Rotate({0,0,1});
        }

        res.dirX = dir.x; res.dirY = dir.y; res.dirZ = dir.z;

        // 4. Calcular Ángulos Relativos (Yaw/Pitch) para Gimbals/Cámaras
        // yaw = atan2(x, z), pitch = asin(y)
        res.yawRel = std::atan2(dir.x, dir.z) * RAD_TO_DEG;
        res.pitchRel = std::asin(dir.y) * RAD_TO_DEG;

        // 5. Proyección y UV
        if (dir.z <= 0.01f) {
            res.isValid = false;
            return res; // Mirando atrás
        }

        if (calibMode == CALIB_MODE_CENTER) {
            // --- MODO FÍSICO CON OFFSET ---
            // Raycast a plano Z = distance
            float t = screenDistance / dir.z;
            Vector3 hit = dir * t;
            
            res.worldX = hit.x; res.worldY = hit.y; res.worldZ = hit.z;

            // Offset: El usuario no está en (0,0) de la pantalla, sino en (offsetX, offsetY)
            // Esto significa que el centro de la pantalla está en (-offsetX, -offsetY) relativo al rayo.
            // UV = (Hit - ScreenCenter) / Size + 0.5
            // ScreenCenterCoords = (0,0) físico si el usuario está centrado.
            // Si usuario está a la derecha (+X), el centro pantalla está a la izquierda (-X).
            
            float relX = hit.x - offsetX;
            float relY = hit.y - offsetY;

            res.u = (relX / screenWidth) + 0.5f;
            res.v = (relY / screenHeight) + 0.5f;

        } else {
            // --- MODO CORNERS (FRUSTUM) ---
            // Proyección puramente angular
            float tx = dir.x / dir.z;
            float ty = dir.y / dir.z;

            // Mapeo (Remap) de [min, max] a [0, 1]
            // u = (val - min) / (max - min)
            res.u = (tx - frustumTanMinX) / (frustumTanMaxX - frustumTanMinX);
            
            // Nota: Asumiendo que TL era Top (MaxY) y BR era Bottom (MinY)
            // Si queremos V=0 abajo y V=1 arriba:
            res.v = (ty - frustumTanMinY) / (frustumTanMaxY - frustumTanMinY);
            
            // En este modo, WorldPos es virtual (proyectado a 1 metro)
            res.worldX = tx; res.worldY = ty; res.worldZ = 1.0f;
        }

        res.isValid = true;
        return res;
    }

private:
    // Maneja la convención de entrada
    Quaternion NormalizeInput(float a, float b, float c, float d) {
        Quaternion q;
        if (quatMode == MODE_W_FIRST) {
            q = {b, c, d, a}; // WXYZ -> XYZW
        } else {
            q = {a, b, c, d}; // XYZW -> XYZW
        }
        // Normalizar siempre por seguridad
        float m = std::sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
        if (m > 0) { q.x/=m; q.y/=m; q.z/=m; q.w/=m; }
        return q;
    }
};

// =======================================================================
// 4. API EXPORTADA (C INTERFACE)
// =======================================================================

struct C_ResultV2 {
    float u, v;
    float wx, wy, wz;
    float dx, dy, dz;
    float yaw, pitch;
    int isValid;
};

extern "C" {
    
    DLL_EXPORT UniversalPointerCoreV2* Pointer_Create() {
        return new UniversalPointerCoreV2();
    }

    DLL_EXPORT void Pointer_Destroy(UniversalPointerCoreV2* ptr) {
        if (ptr) delete ptr;
    }

    // --- Developer Experience API ---
    
    DLL_EXPORT int Pointer_GetVersion() {
        return 200; // v2.0.0
    }

    DLL_EXPORT void Pointer_SetLogCallback(UniversalPointerCoreV2* ptr, LogCallback cb) {
        if(ptr) ptr->SetLogger(cb);
    }

    DLL_EXPORT void Pointer_SetQuaternionMode(UniversalPointerCoreV2* ptr, int mode) {
        if(ptr) ptr->SetQuaternionMode(mode);
    }

    // --- Configuración y Calibración ---

    DLL_EXPORT void Pointer_Configure(UniversalPointerCoreV2* ptr, 
                                      float w, float h, float dist, 
                                      float offX, float offY, float smooth) {
        if(ptr) ptr->Configure(w, h, dist, offX, offY, smooth);
    }

    DLL_EXPORT void Pointer_CalibrateCenter(UniversalPointerCoreV2* ptr, float a, float b, float c, float d) {
        if(ptr) ptr->CalibrateCenter(a, b, c, d);
    }

    DLL_EXPORT void Pointer_CalibrateCorners(UniversalPointerCoreV2* ptr, 
                                             float tl1, float tl2, float tl3, float tl4,
                                             float br1, float br2, float br3, float br4) {
        if(ptr) ptr->CalibrateCorners(tl1, tl2, tl3, tl4, br1, br2, br3, br4);
    }

    // --- Runtime ---

    DLL_EXPORT C_ResultV2 Pointer_Process(UniversalPointerCoreV2* ptr, float a, float b, float c, float d) {
        C_ResultV2 c_res = {0,0,0,0,0,0,0,0,0,0,0};
        if (!ptr) return c_res;

        auto res = ptr->Process(a, b, c, d);

        c_res.u = res.u; c_res.v = res.v;
        c_res.wx = res.worldX; c_res.wy = res.worldY; c_res.wz = res.worldZ;
        c_res.dx = res.dirX; c_res.dy = res.dirY; c_res.dz = res.dirZ;
        c_res.yaw = res.yawRel; c_res.pitch = res.pitchRel;
        c_res.isValid = res.isValid ? 1 : 0;
        
        return c_res;
    }
}
