import json
import os
import subprocess
import sys
import tkinter as tk
from pathlib import Path
from tkinter import font, messagebox


def load_launcher_config(base_dir):
    config_path = base_dir / "configs" / "launcher_config.json"
    default_config = {
        "title": "Simulation Workspace",
        "subtitle": "Select a module to launch the interactive environment",
        "window_width": 800,
        "window_height": 540,
    }
    try:
        if config_path.exists():
            with open(config_path, "r", encoding="utf-8") as config_file:
                loaded = json.load(config_file)
                if isinstance(loaded, dict):
                    default_config.update(loaded)
    except Exception:
        pass
    return default_config


def discover_simulations(base_dir):
    sims = []
    simulations_dir = base_dir / "simulations"
    if not simulations_dir.is_dir():
        return sims

    for sim_folder in sorted(simulations_dir.iterdir()):
        if not sim_folder.is_dir():
            continue
        entry_file = sim_folder / "run.py"
        if not entry_file.is_file():
            continue

        folder_name = sim_folder.name
        display_name = folder_name.replace("_", " ").title()

        # Determine a category based on common naming or folder structure
        category = "General"
        if folder_name in ["ant", "ants", "birds"]:
            category = "Biological"
        elif folder_name in ["flight", "laser", "fluid"]:
            category = "Physics"

        sims.append(
            {
                "display_name": display_name,
                "folder_name": folder_name,
                "category": category,
                "entry_file": str(entry_file),
            }
        )
    return sims


class SimLauncherApp:
    def __init__(self, root):
        self.root = root
        self.base_dir = Path(__file__).resolve().parent
        self.config = load_launcher_config(self.base_dir)

        # Theme Colors (Deep Dark / Slate)
        self.colors = {
            "bg": "#0f172a",  # Deep Blue-Gray
            "sidebar": "#1e293b",  # Lighter Slate
            "card": "#334155",  # UI Slate
            "accent": "#38bdf8",  # Sky Blue
            "text": "#f8fafc",  # Ghost White
            "muted": "#94a3b8",  # Muted Slate
            "button": "#0ea5e9",  # Bright Blue
            "button_hover": "#0284c7",
        }

        self.root.title(self.config["title"])
        self.root.geometry(
            f"{self.config['window_width']}x{self.config['window_height']}"
        )
        self.root.configure(bg=self.colors["bg"])
        self.root.minsize(700, 480)

        self.status_var = tk.StringVar(value="System Ready")
        self.sim_entries = []

        self.setup_fonts()
        self.build_ui()
        self.refresh_list()

    def setup_fonts(self):
        self.fonts = {
            "title": font.Font(family="Segoe UI", size=20, weight="bold"),
            "subtitle": font.Font(family="Segoe UI", size=10),
            "card_title": font.Font(family="Segoe UI", size=12, weight="bold"),
            "card_meta": font.Font(family="Consolas", size=9),
            "status": font.Font(family="Consolas", size=9),
        }

    def build_ui(self):
        # Header Section
        header = tk.Frame(self.root, bg=self.colors["bg"], padx=30, pady=25)
        header.pack(fill=tk.X)

        tk.Label(
            header,
            text=self.config["title"].upper(),
            font=self.fonts["title"],
            fg=self.colors["accent"],
            bg=self.colors["bg"],
        ).pack(anchor="w")

        tk.Label(
            header,
            text=self.config["subtitle"],
            font=self.fonts["subtitle"],
            fg=self.colors["muted"],
            bg=self.colors["bg"],
        ).pack(anchor="w", pady=(2, 0))

        # Main Content Area (Scrollable List)
        main_frame = tk.Frame(self.root, bg=self.colors["bg"], padx=30)
        main_frame.pack(fill=tk.BOTH, expand=True)

        list_container = tk.Frame(main_frame, bg=self.colors["sidebar"], bd=0)
        list_container.pack(fill=tk.BOTH, expand=True)

        scrollbar = tk.Scrollbar(
            list_container, orient=tk.VERTICAL, bg=self.colors["sidebar"]
        )
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.listbox = tk.Listbox(
            list_container,
            selectmode=tk.SINGLE,
            yscrollcommand=scrollbar.set,
            font=self.fonts["card_title"],
            bg=self.colors["sidebar"],
            fg=self.colors["text"],
            selectbackground=self.colors["accent"],
            selectforeground=self.colors["bg"],
            activestyle="none",
            bd=0,
            highlightthickness=0,
        )
        self.listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=10, pady=10)
        scrollbar.config(command=self.listbox.yview)

        self.listbox.bind("<Double-Button-1>", lambda _: self.run_selected())
        self.listbox.bind("<<ListboxSelect>>", self.on_select)

        # Footer / Controls
        footer = tk.Frame(self.root, bg=self.colors["bg"], padx=30, pady=20)
        footer.pack(fill=tk.X)

        self.run_btn = tk.Button(
            footer,
            text="LAUNCH SIMULATION",
            command=self.run_selected,
            bg=self.colors["button"],
            fg=self.colors["text"],
            activebackground=self.colors["button_hover"],
            font=self.fonts["card_title"],
            padx=20,
            pady=8,
            bd=0,
            cursor="hand2",
        )
        self.run_btn.pack(side=tk.LEFT)

        tk.Button(
            footer,
            text="REFRESH",
            command=self.refresh_list,
            bg=self.colors["card"],
            fg=self.colors["text"],
            padx=15,
            pady=8,
            bd=0,
            cursor="hand2",
        ).pack(side=tk.LEFT, padx=10)

        tk.Button(
            footer,
            text="EXIT",
            command=self.root.destroy,
            bg="#ef4444",
            fg=self.colors["text"],
            padx=15,
            pady=8,
            bd=0,
            cursor="hand2",
        ).pack(side=tk.RIGHT)

        # Status Bar
        self.status_bar = tk.Label(
            self.root,
            textvariable=self.status_var,
            font=self.fonts["status"],
            bg=self.colors["sidebar"],
            fg=self.colors["muted"],
            anchor="w",
            padx=10,
            pady=3,
        )
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    def refresh_list(self):
        self.listbox.delete(0, tk.END)
        self.sim_entries = discover_simulations(self.base_dir)
        for sim in self.sim_entries:
            # Add some spacing using spaces in the string for a cleaner look
            entry_text = f"  {sim['display_name']}   [{sim['category'].upper()}]"
            self.listbox.insert(tk.END, entry_text)

        if self.sim_entries:
            self.listbox.selection_set(0)
            self.status_var.set(
                f"Workspace initialized. {len(self.sim_entries)} simulations detected."
            )
        else:
            self.status_var.set("Warning: No simulations detected in workspace.")

    def on_select(self, event):
        selection = self.listbox.curselection()
        if selection:
            sim = self.sim_entries[selection[0]]
            self.status_var.set(
                f"Target: {sim['folder_name']}/run.py ready for deployment."
            )

    def run_selected(self):
        selection = self.listbox.curselection()
        if not selection:
            return

        sim = self.sim_entries[selection[0]]
        file_path = Path(sim["entry_file"])

        if not file_path.is_file():
            messagebox.showerror("IO Error", f"Entry point missing:\n{file_path}")
            return

        try:
            # Use the environment python if possible
            python_exe = sys.executable
            subprocess.Popen([python_exe, str(file_path)], cwd=str(file_path.parent))
            self.status_var.set(f"Success: {sim['display_name']} instance started.")
        except Exception as exc:
            messagebox.showerror(
                "Runtime Error", f"Failed to initialize simulation:\n\n{exc}"
            )


def main():
    root = tk.Tk()
    SimLauncherApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
