import ctypes
import math
import tkinter as tk
import time
import os
import json
import threading
import websocket

# V2 Result Structure
class PointerResultV2(ctypes.Structure):
    _fields_ = [
        ("u", ctypes.c_float),
        ("v", ctypes.c_float),
        ("worldX", ctypes.c_float),
        ("worldY", ctypes.c_float),
        ("worldZ", ctypes.c_float),
        ("dirX", ctypes.c_float),
        ("dirY", ctypes.c_float),
        ("dirZ", ctypes.c_float),
        ("yaw", ctypes.c_float),
        ("pitch", ctypes.c_float),
        ("isValid", ctypes.c_int),
    ]

# Callback type for logging
LOG_CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

def dll_log(message):
    print(f"[DLL LOG] {message.decode('utf-8')}")

class PointerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("UniversalPointerCore V2 Pro")
        
        # Screen settings
        self.width = 1280
        self.height = 720
        self.canvas = tk.Canvas(root, width=self.width, height=self.height, bg="black")
        self.canvas.pack()
        
        # Start Minimized
        self.root.iconify()
        
        # Control Panel
        self.panel = tk.Frame(root, bg="#222")
        self.panel.pack(fill="x")
        
        self.status_label = tk.Label(self.panel, text="Esperando...", fg="yellow", bg="#222")
        self.status_label.pack(side="left", padx=10)
        
        self.ver_label = tk.Label(self.panel, text="Ver: ---", fg="#666", bg="#222", font=("Consolas", 8))
        self.ver_label.pack(side="left", padx=5)

        self.tel_label = tk.Label(self.panel, text="U: 0.00 | V: 0.00", fg="#aaa", bg="#222", font=("Consolas", 9))
        self.tel_label.pack(side="left", padx=20)
        
        # Calibration state indicator
        self.cal_state_label = tk.Label(self.panel, text="Calibración: Centro", fg="#fff", bg="#222")
        self.cal_state_label.pack(side="right", padx=10)

        # Circle setup
        self.radius = 30
        self.circle = self.canvas.create_oval(0, 0, 0, 0, fill="cyan", outline="white", width=3)
        
        # Data storage
        self.current_q = {"x": 0.0, "y": 0.0, "z": 0.0, "w": 1.0}
        self.tl_q = None
        self.last_update_time = 0
        self.current_color = "cyan"
        
        # Config state
        self.screen_width = 1.2
        self.screen_height = 0.9
        self.distance = 1.5
        self.offX = 0.0
        self.offY = 0.0
        self.smooth = 0.8
        
        # Load DLL
        dll_path = os.path.abspath(os.path.join("dll", "UniversalPointerCore_V2.dll"))
        try:
            self.lib = ctypes.CDLL(dll_path)
            print(f"Loaded: {dll_path}")
        except Exception as e:
            print(f"Error: {e}"); self.root.destroy(); return

        # V2 Signatures
        self.lib.Pointer_GetVersion.restype = ctypes.c_int
        self.lib.Pointer_Create.restype = ctypes.c_void_p
        self.lib.Pointer_SetLogCallback.argtypes = [ctypes.c_void_p, LOG_CALLBACK]
        self.lib.Pointer_SetQuaternionMode.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.lib.Pointer_Configure.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]
        self.lib.Pointer_CalibrateCenter.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]
        self.lib.Pointer_CalibrateCorners.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]
        self.lib.Pointer_Process.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float, ctypes.c_float, ctypes.c_float]
        self.lib.Pointer_Process.restype = PointerResultV2
        self.lib.Pointer_Destroy.argtypes = [ctypes.c_void_p]

        # Initialize
        self.pointer_ptr = self.lib.Pointer_Create()
        
        # Setup Logging
        self.log_callback_ref = LOG_CALLBACK(dll_log)
        self.lib.Pointer_SetLogCallback(self.pointer_ptr, self.log_callback_ref)
        
        # Display Version
        version = self.lib.Pointer_GetVersion()
        self.ver_label.config(text=f"Ver: {version}")

        self.update_config()
        self.lib.Pointer_CalibrateCenter(self.pointer_ptr, 0.0, 0.0, 0.0, 1.0)
        
        # Start Threads
        threading.Thread(target=self.start_websocket, daemon=True).start()
        self.update_loop()

    def update_config(self):
        self.lib.Pointer_Configure(
            self.pointer_ptr, self.screen_width, self.screen_height, self.distance,
            self.offX, self.offY, self.smooth
        )

    def start_websocket(self):
        def on_open(ws):
            self.ws = ws
            print("Python connected to Bridge")

        def on_message(ws, message):
            data = json.loads(message)
            if data['type'] == 'move':
                self.current_q = data['q']
                if 'roll' in data:
                    self.current_color = self.hue_to_hex((data['roll'] + 90) / 180)
                self.status_label.config(text="Live Sensors", fg="#00ff00")
            elif data['type'] == 'calibrate':
                self.calibrate_center()
            elif data['type'] == 'calibrate_tl':
                self.tl_q = self.current_q
                self.cal_state_label.config(text="TL Guardado, apunta a BR", fg="cyan")
            elif data['type'] == 'calibrate_br':
                self.calibrate_corners()
            elif data['type'] == 'set_mode':
                self.lib.Pointer_SetQuaternionMode(self.pointer_ptr, int(data['mode']))
            elif data['type'] == 'config':
                if 'distance' in data: self.distance = float(data['distance'])
                if 'smooth' in data: self.smooth = float(data['smooth'])
                if 'offX' in data: self.offX = float(data['offX'])
                self.update_config()

        def on_error(ws, error):
            print(f"WS Error: {error}")

        ws_url = "ws://localhost:3000/?type=python"
        self.ws_app = websocket.WebSocketApp(ws_url, on_open=on_open, on_message=on_message, on_error=on_error)
        self.ws_app.run_forever()

    def hue_to_hex(self, h):
        import colorsys
        rgb = colorsys.hls_to_rgb(h % 1.0, 0.5, 1.0)
        return '#%02x%02x%02x' % (int(rgb[0]*255), int(rgb[1]*255), int(rgb[2]*255))

    def calibrate_center(self):
        q = self.current_q
        self.lib.Pointer_CalibrateCenter(self.pointer_ptr, float(q['x']), float(q['y']), float(q['z']), float(q['w']))
        self.cal_state_label.config(text="Modo: Centro", fg="white")

    def calibrate_corners(self):
        if self.tl_q:
            q_tl = self.tl_q
            q_br = self.current_q
            self.lib.Pointer_CalibrateCorners(
                self.pointer_ptr,
                float(q_tl['x']), float(q_tl['y']), float(q_tl['z']), float(q_tl['w']),
                float(q_br['x']), float(q_br['y']), float(q_br['z']), float(q_br['w'])
            )
            self.cal_state_label.config(text="Modo: Frustum (Corners)", fg="#28a745")
            self.tl_q = None

    def update_loop(self):
        q = self.current_q
        result = self.lib.Pointer_Process(self.pointer_ptr, float(q['x']), float(q['y']), float(q['z']), float(q['w']))
        
        # 3D BOX LOGIC
        # We use the direction vector (dirX, dirY, dirZ) from V2
        # dirZ points "forward" to the screen at Z=distance
        dx, dy, dz = result.dirX, result.dirY, result.dirZ
        
        # Define virtual 3D box limits (Meters)
        box_w = 4.0   # Total width
        box_h = 3.0   # Total height from floor
        box_d = 5.0   # Distance to front screen
        
        # Room boundaries relative to origin (User at 0,0,0)
        # Front Wall (Screen): Z = distance
        # Side Walls: X = +/- box_w/2
        # Floor: Y = -box_h/2
        
        hit_x, hit_y, hit_z = 0, 0, 0
        hit_wall = "None"
        
        # 1. Try Front Wall (Z = distance)
        # t = distance / dz
        if dz > 0.01:
            t = self.distance / dz
            hit_x = dx * t
            hit_y = dy * t
            hit_z = self.distance

            # Check if it hits the front wall using ROOM dimensions (not screen dimensions)
            if abs(hit_x) <= box_w/2 and abs(hit_y) <= box_h/2:
                hit_wall = "Front"
            else:
                # 2. Check Side Walls (X = +/- box_w/2)
                side_x = (box_w/2) if dx > 0 else (-box_w/2)
                if abs(dx) > 0.01:
                    t_side = side_x / dx
                    hit_x = side_x
                    hit_y = dy * t_side
                    hit_z = dz * t_side
                    hit_wall = "Right" if dx > 0 else "Left"

                # 3. Check Floor (Y = -box_h/2)
                floor_y = -box_h/2  # Floor is at bottom of room
                if dy < -0.01:
                    t_floor = floor_y / dy
                    if hit_wall == "None" or t_floor < t: # Floor hit is closer
                        hit_x = dx * t_floor
                        hit_y = floor_y
                        hit_z = dz * t_floor
                        hit_wall = "Floor"
        
        # Send to Bridge for Viewer
        if hasattr(self, 'ws') and self.ws and self.ws.sock and self.ws.sock.connected:
            self.ws.send(json.dumps({
                "type": "3d_move",
                "x": hit_x, "y": hit_y, "z": -hit_z, # Z-inverted for Three.js depth
                "wall": hit_wall
            }))

        if result.isValid:
            x, y = result.u * self.width, (1.0 - result.v) * self.height
            self.canvas.coords(self.circle, x - self.radius, y - self.radius, x + self.radius, y + self.radius)
            self.canvas.itemconfig(self.circle, state="normal", fill=self.current_color)
            self.tel_label.config(text=f"Prop: {hit_wall} | U: {result.u:.2f} | V: {result.v:.2f}")
        else:
            self.canvas.itemconfig(self.circle, state="hidden")
        self.root.after(16, self.update_loop)

if __name__ == "__main__":
    root = tk.Tk()
    app = PointerApp(root)
    root.mainloop()
