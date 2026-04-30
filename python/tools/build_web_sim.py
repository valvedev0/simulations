from __future__ import annotations

import argparse
import shutil
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD_ROOT = ROOT / "build"
TEMPLATES_ROOT = ROOT / "web_templates"


@dataclass(frozen=True)
class SimBuild:
    slug: str
    display_name: str
    output_dir_name: str
    source_python_path: str
    template_dir_name: str
    tagline: str
    objective: str
    play_notes: tuple[str, ...]
    tech_notes: tuple[str, ...]


SIM_BUILDS: dict[str, SimBuild] = {
    "laser": SimBuild(
        slug="laser",
        display_name="Laser Puzzle",
        output_dir_name="laser_Sim webbuild",
        source_python_path="simulations/laser/laser_sim.py",
        template_dir_name="laser",
        tagline="A simple reflection puzzle where you clear green targets while avoiding red hazard drones.",
        objective="Use mirror reflections to hit every green target. Each target disappears on contact, the sidebar updates immediately, and touching a red drone with the reflected beam ends the round.",
        play_notes=(
            "Move the pointer to aim the laser pulse.",
            "Click inside the arena to move the emitter.",
            "Keep the emitter outside the green safety circles around each target.",
            "Use mirror reflections to hit the green targets.",
            "Avoid the moving red drones because hitting one loses the round.",
            "The safety circles grow and the beam pulse slows down as the level increases.",
            "Clear the board before the timer reaches zero.",
        ),
        tech_notes=(
            "Static export with relative asset paths.",
            "No Python runtime or package install required after export.",
            "Ready to copy into a GitHub Pages repository or any static host.",
        ),
    ),
    "ant": SimBuild(
        slug="ant",
        display_name="Ant Simulation",
        output_dir_name="ant_Sim webbuild",
        source_python_path="simulations/ant/ant_sim.py",
        template_dir_name="ant",
        tagline="Watch colonies of ants navigate obstacles and reach multiple colored targets using pathfinding and obstacle avoidance.",
        objective="Observe ants autonomously seek targets while avoiding obstacles. Click to add new targets. Use keyboard controls to pause, reset, or toggle path visualization.",
        play_notes=(
            "Ants automatically navigate toward their assigned colored targets.",
            "Click in the arena to add new targets dynamically.",
            "Press R to reset the entire simulation.",
            "Press P to pause/resume the simulation.",
            "Press T to toggle ant path visualization.",
            "Watch the diagnostics panel to track arrival statistics.",
            "Ants use seek, wander, and obstacle-avoidance behaviors.",
        ),
        tech_notes=(
            "Real-time pathfinding with autonomous behaviors.",
            "Interactive target placement and simulation control.",
            "Detailed performance and diagnostic telemetry.",
            "Static HTML/JS export ready for any web host.",
        ),
    ),
    "flight": SimBuild(
        slug="flight",
        display_name="Flight Simulation",
        output_dir_name="flight_Sim webbuild",
        source_python_path="simulations/flight/flight_sim.py",
        template_dir_name="flight",
        tagline="Pilot a responsive aircraft with throttle control, heading management, and optional autopilot guidance.",
        objective="Control a 2D aircraft to navigate toward waypoints. Manage throttle and heading manually, or engage autopilot for autonomous flight.",
        play_notes=(
            "Use W/↑ and S/↓ to increase or decrease throttle.",
            "Use A/← and D/→ to turn left or right.",
            "Press SPACE to toggle autopilot mode.",
            "Click in the arena to place a new waypoint (yellow square).",
            "In autopilot mode, the aircraft automatically navigates toward the waypoint.",
            "Monitor heading, speed, and throttle in the diagnostics panel.",
            "Bounce off boundaries at reduced velocity to stay in bounds.",
        ),
        tech_notes=(
            "Realistic flight physics with drag, acceleration, and max speed.",
            "Keyboard and mouse input for full flight control.",
            "Optional autonomous autopilot system for waypoint navigation.",
            "Real-time telemetry and performance diagnostics.",
        ),
    ),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate static web builds for supported simulation exports."
    )
    parser.add_argument(
        "sim",
        nargs="?",
        help="Simulation slug to build, for example: laser",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List supported simulation slugs and exit.",
    )
    return parser.parse_args()


def render_index(sim: SimBuild) -> str:
    how_to_play = "\n".join(f"            <li>{note}</li>" for note in sim.play_notes)

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>{sim.display_name}</title>
  <meta
    name="description"
    content="{sim.tagline}"
  >
  <link rel="stylesheet" href="styles.css">
</head>
<body>
  <main class="page-shell">
    <header class="masthead">
      <div class="masthead-copy">
        <p class="kicker">Interactive Simulation</p>
        <h1>{sim.display_name}</h1>
        <p class="lede">{sim.tagline}</p>
      </div>
    </header>

    <section class="info-grid" aria-label="How to play and objective">
      <article class="info-card">
        <h2>How to Play</h2>
        <ul>
{how_to_play}
        </ul>
      </article>

      <article class="info-card">
        <h2>Objective</h2>
        <p>{sim.objective}</p>
      </article>
    </section>

    <section class="sim-card">
      <canvas
        id="gameCanvas"
        width="1180"
        height="650"
        aria-label="{sim.display_name} simulation"
      ></canvas>
    </section>
  </main>

  <script src="app.js"></script>
</body>
</html>
"""


def render_readme(sim: SimBuild) -> str:
    play_notes = "\n".join(f"- {note}" for note in sim.play_notes)
    tech_notes = "\n".join(f"- {note}" for note in sim.tech_notes)

    return f"""# {sim.display_name}

This folder contains the generated static web package for `{sim.source_python_path}`.

## Files

- `index.html`
- `styles.css`
- `app.js`

## How to play

{play_notes}

## Publish

1. Copy the contents of this folder into the repo root or publish directory used by your static hosting setup.
2. Commit and push the files.
3. Open the published URL. The export uses only relative paths, so no extra build step is required.

## Regenerate locally

Run this from the project root:

```powershell
.\\build_web.bat {sim.slug}
```

## Notes

{tech_notes}
"""


def copy_template_assets(source_dir: Path, target_dir: Path) -> None:
    for asset_name in ("app.js", "styles.css"):
        source = source_dir / asset_name
        if not source.exists():
            raise FileNotFoundError(f"Missing template asset: {source}")
        shutil.copy2(source, target_dir / asset_name)


def build_sim(sim: SimBuild) -> Path:
    template_dir = TEMPLATES_ROOT / sim.template_dir_name
    if not template_dir.exists():
        raise FileNotFoundError(f"Missing template directory: {template_dir}")

    output_dir = BUILD_ROOT / sim.output_dir_name
    output_dir.mkdir(parents=True, exist_ok=True)

    copy_template_assets(template_dir, output_dir)
    (output_dir / "index.html").write_text(render_index(sim), encoding="utf-8")
    (output_dir / "README.md").write_text(render_readme(sim), encoding="utf-8")
    return output_dir


def print_supported_sims() -> None:
    print("Supported web builds:")
    for sim in SIM_BUILDS.values():
        print(f"  - {sim.slug}: {sim.display_name} -> build/{sim.output_dir_name}")


def main() -> int:
    args = parse_args()

    if args.list:
        print_supported_sims()
        return 0

    if not args.sim:
        print("Please provide a simulation slug or use --list.")
        return 1

    sim = SIM_BUILDS.get(args.sim)
    if sim is None:
        print(f"Unsupported simulation: {args.sim}")
        print_supported_sims()
        return 1

    output_dir = build_sim(sim)
    relative_output = output_dir.relative_to(ROOT)
    print(f"Built {sim.display_name} to {relative_output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
