import tkinter as tk
from tkinter import messagebox
import subprocess
import os
import webbrowser
import sys
import ctypes
import socket

# Windows API Constants
SW_MOVE = 5
SW_RESTORE = 9

def focus_window(title_part):
    """Finds a window with title_part in its title and brings it to focus."""
    EnumWindows = ctypes.windll.user32.EnumWindows
    EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int))
    GetWindowText = ctypes.windll.user32.GetWindowTextW
    GetWindowTextLength = ctypes.windll.user32.GetWindowTextLengthW
    IsWindowVisible = ctypes.windll.user32.IsWindowVisible
    SetForegroundWindow = ctypes.windll.user32.SetForegroundWindow
    ShowWindow = ctypes.windll.user32.ShowWindow

    found_hwnd = [None]

    def foreach_window(hwnd, lParam):
        if IsWindowVisible(hwnd):
            length = GetWindowTextLength(hwnd)
            buff = ctypes.create_unicode_buffer(length + 1)
            GetWindowText(hwnd, buff, length + 1)
            if title_part.lower() in buff.value.lower():
                found_hwnd[0] = hwnd
                return False # Stop enumerating
        return True

    # Use a thread-safe way to store the procedure reference
    proc = EnumWindowsProc(foreach_window)
    EnumWindows(proc, 0)
    
    if found_hwnd[0]:
        ShowWindow(found_hwnd[0], SW_RESTORE)
        SetForegroundWindow(found_hwnd[0])
        return True
    return False

def reload_browser_window():
    """Sends F5 key press to reload the focused browser window."""
    import time
    time.sleep(0.3)  # Wait for window to be fully focused
    
    # Virtual key code for F5
    VK_F5 = 0x74
    
    # Key event flags
    KEYEVENTF_KEYUP = 0x0002
    
    # Send F5 key press
    ctypes.windll.user32.keybd_event(VK_F5, 0, 0, 0)  # Key down
    time.sleep(0.05)
    ctypes.windll.user32.keybd_event(VK_F5, 0, KEYEVENTF_KEYUP, 0)  # Key up

def is_server_running(port=3000):
    """Checks if a server is already listening on the given port."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(0.5)
        return s.connect_ex(('localhost', port)) == 0

class AppController:
    def __init__(self, root):
        self.root = root
        self.root.title("Control de Prueba")
        self.root.geometry("300x230")
        self.root.attributes('-topmost', True)
        self.root.configure(bg='#2c3e50')

        self.label = tk.Label(root, text="Estado: Detenido", fg="white", bg='#2c3e50', font=("Arial", 12, "bold"))
        self.label.pack(pady=20)

        self.start_button = tk.Button(root, text="Arrancar Prueba", command=self.start_app, 
                                      bg='#27ae60', fg='white', font=("Arial", 10, "bold"), width=20, height=2)
        self.start_button.pack(pady=5)

        self.stop_button = tk.Button(root, text="Detener Prueba", command=self.stop_app, 
                                     bg='#c0392b', fg='white', font=("Arial", 10, "bold"), width=20, height=2)
        self.stop_button.pack(pady=5)

    def start_app(self):
        try:
            # Check if window already exists (using the exact name provided by the user)
            window_found = focus_window("UniversalPointer 3D Viewer") or focus_window("Viewer")
            if window_found:
                self.label.config(text="Estado: Reenfocado", fg="#3498db")
                reload_browser_window()  # Reload the page after focusing
                if is_server_running():
                    return

            # Get current working directory
            cwd = os.getcwd()
            
            # Start Node.js Server in its own folder
            server_dir = os.path.join(cwd, "sensor_server")
            if not is_server_running():
                subprocess.Popen('node server.js', cwd=server_dir, shell=True, creationflags=subprocess.CREATE_NEW_CONSOLE)
            
            # Start Python App
            subprocess.Popen('python main.py', cwd=cwd, shell=True, creationflags=subprocess.CREATE_NEW_CONSOLE)
            
            # Open Browser only if not found
            if not window_found:
                webbrowser.open("http://localhost:3000/viewer.html")

            self.label.config(text="Estado: En ejecución", fg="#2ecc71")
        except Exception as e:
            messagebox.showerror("Error", f"No se pudo iniciar la prueba: {e}")

    def stop_app(self):
        try:
            # Kill Node and Python processes
            subprocess.run('taskkill /F /IM node.exe /T', shell=True, capture_output=True)
            subprocess.run('taskkill /F /IM python.exe /T', shell=True, capture_output=True)
            
            self.label.config(text="Estado: Detenido", fg="#e74c3c")
        except Exception as e:
            messagebox.showerror("Error", f"Error al detener procesos: {e}")

if __name__ == "__main__":
    root = tk.Tk()
    app = AppController(root)
    root.mainloop()
