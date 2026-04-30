# Web Build Guide for Python Simulations

## Quick Start

To build web versions of the simulations, use the batch script or Python build tool from the project root.

### Using the Batch Script (Windows)

```powershell
.\build_web.bat laser
.\build_web.bat ant
.\build_web.bat flight
```

### Using Python Directly (Windows, macOS, Linux)

```powershell
# Windows
.\.venv\Scripts\python.exe tools\build_web_sim.py laser
.\.venv\Scripts\python.exe tools\build_web_sim.py ant
.\.venv\Scripts\python.exe tools\build_web_sim.py flight
```

```bash
# macOS/Linux
./.venv/bin/python tools/build_web_sim.py laser
./.venv/bin/python tools/build_web_sim.py ant
./.venv/bin/python tools/build_web_sim.py flight
```

## List Available Builds

To see all available web builds:

```powershell
# Windows
.\.venv\Scripts\python.exe tools\build_web_sim.py --list
```

```bash
# macOS/Linux
./.venv/bin/python tools/build_web_sim.py --list
```

Output:
```
Supported web builds:
  - laser: Laser Puzzle -> build/laser_Sim webbuild
  - ant: Ant Simulation -> build/ant_Sim webbuild
  - flight: Flight Simulation -> build/flight_Sim webbuild
```

## Available Simulations

### 1. **Laser Puzzle** (laser)
- **Location:** `build/laser_Sim webbuild/`
- **Description:** Aim a laser pulse, bounce it off mirrors, hit green targets, and avoid red drones
- **Controls:**
  - Mouse move: Aim the laser
  - Click in arena: Move the laser emitter
  - Buttons: Shuffle mirrors, reset round
- **Features:**
  - Progressive difficulty levels
  - Timer-based gameplay
  - Real-time score tracking

### 2. **Ant Simulation** (ant) - NEW
- **Location:** `build/ant_Sim webbuild/`
- **Description:** Watch autonomous ants navigate obstacles and reach colored targets
- **Controls:**
  - **Keyboard:**
    - R: Reset simulation
    - P: Pause/Resume
    - T: Toggle path visualization
  - **Mouse:**
    - Click in arena: Add a new target

- **Features:**
  - 30 autonomous ants with individual behaviors
  - Seek, wander, and obstacle avoidance behaviors
  - 3 colored targets (Red, Blue, Green)
  - Real-time path tracking
  - Arrival statistics and diagnostics
  - Frame rate and performance monitoring

### 3. **Flight Simulation** (flight) - NEW
- **Location:** `build/flight_Sim webbuild/`
- **Description:** Pilot a responsive aircraft with throttle control and optional autopilot
- **Controls:**
  - **Keyboard:**
    - W / Arrow Up: Increase throttle
    - S / Arrow Down: Decrease throttle / Brake
    - A / Arrow Left: Turn left
    - D / Arrow Right: Turn right
    - Space: Toggle autopilot
  - **Mouse:**
    - Click in arena: Set waypoint target

- **Features:**
  - Realistic flight physics (drag, acceleration, max speed)
  - Manual and autonomous flight modes
  - Heading and speed control
  - Waypoint navigation
  - Real-time diagnostics panel
  - Performance telemetry

## Output Structure

After building, each simulation generates a complete static web package:

```
build/
  ant_Sim webbuild/
    index.html        # Landing page with instructions
    app.js           # Game logic and rendering
    styles.css       # Visual styling
    README.md        # Build documentation
  
  flight_Sim webbuild/
    index.html
    app.js
    styles.css
    README.md
  
  laser_Sim webbuild/
    index.html
    app.js
    styles.css
    README.md
```

## Deploying to GitHub Pages

1. Generate the web build:
   ```powershell
   .\build_web.bat ant
   ```

2. Copy the output folder contents to your GitHub Pages repository root or publish directory:
   ```powershell
   Copy-Item build\ant_Sim* webbuild -Recurse
   ```

3. Commit and push:
   ```bash
   git add .
   git commit -m "Add ant simulation web build"
   git push
   ```

4. Your simulation is now live at `https://yourusername.github.io/`

## Technical Details

### Canvas-Based Rendering
- All simulations use HTML5 Canvas for real-time rendering
- requestAnimationFrame for smooth 60 FPS animation
- Vector2 math utilities for positioning and movement

### Input Handling
- **Keyboard:** Direct event listeners for game controls
- **Mouse:** Canvas coordinate mapping with DPI scaling

### Performance Monitoring
- Frame time tracking (average over 120 frames)
- Update and render timings
- FPS counter in diagnostics panel
- No external dependencies (pure vanilla JavaScript)

### Browser Compatibility
- Works on modern browsers (Chrome, Firefox, Safari, Edge)
- Responsive design for different screen sizes
- Mobile-friendly with touch event support (can be extended)

## Build Script Details

The `build_web_sim.py` script:
1. Validates template directories exist
2. Copies `app.js` and `styles.css` from templates
3. Generates `index.html` with game instructions
4. Creates `README.md` with deployment instructions
5. Outputs to `build/<simulation>_Sim webbuild/`

All builds use **relative paths only**, so no Python runtime or build tools are required after export.

## Customization

To modify a simulation's web version:

1. Edit files in `web_templates/<slug>/`:
   - `app.js` - Game logic and rendering
   - `styles.css` - Visual styling

2. Rebuild the web version:
   ```powershell
   .\build_web.bat <slug>
   ```

3. The generated HTML is automatically created from the configuration in `build_web_sim.py`

## Troubleshooting

### Build fails with "Missing template asset"
- Ensure `web_templates/<slug>/app.js` and `web_templates/<slug>/styles.css` exist
- Check file names are exactly correct

### Canvas doesn't appear in browser
- Clear browser cache (Ctrl+Shift+Delete)
- Check console for JavaScript errors (F12 → Console)
- Verify `app.js` and `styles.css` are in the same folder as `index.html`

### Game runs slowly
- Check FPS counter in diagnostics panel
- Close other browser tabs
- Check browser developer tools for performance bottlenecks

## Key Implementation Patterns

### Vector Math
All simulations use a Vec2 class for position/velocity calculations:
```javascript
class Vec2 {
  constructor(x = 0, y = 0) { ... }
  add(other) { ... }
  subtract(other) { ... }
  scale(value) { ... }
  normalize() { ... }
  distanceTo(other) { ... }
}
```

### Game Loop
Standard requestAnimationFrame pattern:
```javascript
function tick() {
  updateSimulation();
  render();
  requestAnimationFrame(tick);
}

tick();
```

### Event Handling
- Keyboard: `document.addEventListener('keydown', ...)`
- Mouse: `canvas.addEventListener('click', ...)`
- Pointer position: DPI-aware coordinate conversion

## Next Steps

- Test the simulations at `build/*/index.html`
- Customize colors, physics, or game rules in the source files
- Deploy to GitHub Pages or any static web host
- Share with others - no server or backend required!
