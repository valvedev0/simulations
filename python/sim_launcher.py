import os
import subprocess
import sys
import tkinter as tk
import json
from tkinter import messagebox


def load_launcher_config(base_dir):
    config_path = os.path.join(base_dir, "configs", "launcher_config.json")
    default_config = {
        "title": "Python Simulation Launcher",
        "subtitle": "Pick a simulation and click Run.",
        "window_width": 700,
        "window_height": 460,
    }
    try:
        with open(config_path, "r", encoding="utf-8") as config_file:
            loaded = json.load(config_file)
            if isinstance(loaded, dict):
                default_config.update(loaded)
    except Exception:
        # Keep launcher robust even when config is missing/corrupt.
        pass
    return default_config


def discover_simulations(base_dir):
    sims = []
    simulations_dir = os.path.join(base_dir, "simulations")
    if not os.path.isdir(simulations_dir):
        return sims

    for folder_name in sorted(os.listdir(simulations_dir)):
        sim_folder = os.path.join(simulations_dir, folder_name)
        if not os.path.isdir(sim_folder):
            continue
        entry_file = os.path.join(sim_folder, "run.py")
        if not os.path.isfile(entry_file):
            continue
        display_name = folder_name.replace("_", " ").title()
        sims.append(
            {
                "display_name": display_name,
                "folder_name": folder_name,
                "entry_file": entry_file,
            }
        )
    return sims


class SimLauncherApp:
    def __init__(self, root):
        self.root = root
        self.base_dir = os.path.dirname(os.path.abspath(__file__))
        self.config = load_launcher_config(self.base_dir)

        self.root.title(self.config["title"])
        self.root.geometry(f"{self.config['window_width']}x{self.config['window_height']}")
        self.root.minsize(540, 360)

        self.status_var = tk.StringVar(value="Select a simulation and click Run.")
        self.sim_entries = []

        self.build_ui()
        self.refresh_list()

    def build_ui(self):
        container = tk.Frame(self.root, padx=12, pady=12)
        container.pack(fill=tk.BOTH, expand=True)

        title = tk.Label(container, text=self.config["title"], font=("Segoe UI", 14, "bold"))
        title.pack(anchor="w")

        subtitle = tk.Label(
            container,
            text=self.config["subtitle"],
            font=("Segoe UI", 10),
            fg="#444444",
        )
        subtitle.pack(anchor="w", pady=(2, 10))

        list_frame = tk.Frame(container)
        list_frame.pack(fill=tk.BOTH, expand=True)

        scrollbar = tk.Scrollbar(list_frame, orient=tk.VERTICAL)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.listbox = tk.Listbox(
            list_frame,
            selectmode=tk.SINGLE,
            yscrollcommand=scrollbar.set,
            font=("Consolas", 11),
            activestyle="dotbox",
        )
        self.listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.listbox.yview)

        self.listbox.bind("<Double-Button-1>", lambda _event: self.run_selected())

        controls = tk.Frame(container)
        controls.pack(fill=tk.X, pady=(10, 4))

        run_btn = tk.Button(controls, text="Run Selected", command=self.run_selected, width=16)
        run_btn.pack(side=tk.LEFT)

        refresh_btn = tk.Button(controls, text="Refresh", command=self.refresh_list, width=12)
        refresh_btn.pack(side=tk.LEFT, padx=(8, 0))

        quit_btn = tk.Button(controls, text="Close", command=self.root.destroy, width=12)
        quit_btn.pack(side=tk.RIGHT)

        status = tk.Label(
            container,
            textvariable=self.status_var,
            anchor="w",
            justify=tk.LEFT,
            fg="#1d3557",
            wraplength=650,
        )
        status.pack(fill=tk.X, pady=(8, 0))

    def refresh_list(self):
        self.listbox.delete(0, tk.END)
        self.sim_entries = discover_simulations(self.base_dir)
        for sim in self.sim_entries:
            self.listbox.insert(
                tk.END,
                f"{sim['display_name']}  ({sim['folder_name']})"
            )

        if self.sim_entries:
            self.listbox.selection_set(0)
            self.status_var.set(f"Found {len(self.sim_entries)} simulation(s) in simulations/ folders.")
        else:
            self.status_var.set("No runnable simulations found. Add simulations/<name>/run.py")

    def run_selected(self):
        selection = self.listbox.curselection()
        if not selection:
            messagebox.showwarning("No Selection", "Please select a simulation file first.")
            return

        sim = self.sim_entries[selection[0]]
        file_path = sim["entry_file"]

        if not os.path.isfile(file_path):
            messagebox.showerror("File Missing", f"Cannot find:\n{file_path}")
            self.refresh_list()
            return

        try:
            subprocess.Popen([sys.executable, file_path], cwd=self.base_dir)
            self.status_var.set(f"Started: {sim['display_name']}")
        except Exception as exc:
            messagebox.showerror("Launch Failed", f"Could not run {sim['display_name']}\n\n{exc}")


def main():
    root = tk.Tk()
    SimLauncherApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
