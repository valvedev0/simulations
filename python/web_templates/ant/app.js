(function () {
  "use strict";

  const SIM_WIDTH = 900;
  const UI_WIDTH = 320;
  const HEIGHT = 650;
  const TOTAL_WIDTH = SIM_WIDTH + UI_WIDTH;
  const ANT_COUNT = 30;
  const OBSTACLE_COUNT = 5;
  const TARGET_COUNT = 3;

  const canvas = document.getElementById("gameCanvas");
  const ctx = canvas.getContext("2d");

  class Vec2 {
    constructor(x = 0, y = 0) {
      this.x = x;
      this.y = y;
    }

    clone() {
      return new Vec2(this.x, this.y);
    }

    add(other) {
      return new Vec2(this.x + other.x, this.y + other.y);
    }

    subtract(other) {
      return new Vec2(this.x - other.x, this.y - other.y);
    }

    scale(value) {
      return new Vec2(this.x * value, this.y * value);
    }

    length() {
      return Math.hypot(this.x, this.y);
    }

    normalize() {
      const len = this.length();
      return len > 0 ? this.scale(1 / len) : new Vec2(0, 0);
    }

    distanceTo(other) {
      return this.subtract(other).length();
    }
  }

  class Target {
    constructor(x, y, color) {
      this.pos = new Vec2(x, y);
      this.color = color;
      this.ants_arrived = 0;
    }

    draw() {
      ctx.fillStyle = this.color;
      ctx.beginPath();
      ctx.arc(this.pos.x, this.pos.y, 12, 0, Math.PI * 2);
      ctx.fill();
      
      ctx.strokeStyle = "rgba(255,255,255,0.5)";
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(this.pos.x, this.pos.y, 18, 0, Math.PI * 2);
      ctx.stroke();
    }
  }

  class Obstacle {
    constructor(x, y, radius) {
      this.pos = new Vec2(x, y);
      this.radius = radius;
    }

    draw() {
      ctx.fillStyle = "rgb(120,120,120)";
      ctx.beginPath();
      ctx.arc(this.pos.x, this.pos.y, this.radius, 0, Math.PI * 2);
      ctx.fill();

      ctx.strokeStyle = "rgb(160,160,160)";
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.arc(this.pos.x, this.pos.y, this.radius, 0, Math.PI * 2);
      ctx.stroke();
    }
  }

  class Ant {
    constructor(target, targetIndex) {
      this.pos = new Vec2(Math.random() * 40 + 10, Math.random() * (HEIGHT - 20) + 10);
      this.velocity = new Vec2(0, 0);
      this.speed = 0.7;
      this.target = target;
      this.targetIndex = targetIndex;
      this.path = [this.pos.clone()];
      this.distance_traveled = 0;
      this.arrived = false;
      this.color = target.color;
    }

    update(obstacles) {
      let diagnostics = { arrived_now: false, avoidance_hits: 0 };

      if (this.arrived) {
        return diagnostics;
      }

      const distance_to_target = this.pos.distanceTo(this.target.pos);

      // Check for arrival
      if (distance_to_target < 10) {
        this.arrived = true;
        this.target.ants_arrived += 1;
        diagnostics.arrived_now = true;
        return diagnostics;
      }

      // Seek behavior - aim toward target
      let direction = this.target.pos.subtract(this.pos);
      if (direction.length() > 0) {
        direction = direction.normalize();
      }

      // Wander behavior - random direction changes
      const wander = new Vec2(
        (Math.random() - 0.5) * 0.5,
        (Math.random() - 0.5) * 0.5
      );

      // Obstacle avoidance
      let avoidance = new Vec2(0, 0);
      for (const obs of obstacles) {
        const toObs = this.pos.subtract(obs.pos);
        const distToObs = toObs.length();
        const avoidRadius = obs.radius + 25;

        if (distToObs < avoidRadius && distToObs > 0) {
          diagnostics.avoidance_hits += 1;
          const pushback = toObs.normalize().scale(1.5 * (1 - distToObs / avoidRadius));
          avoidance = avoidance.add(pushback);
        }
      }

      // Combine behaviors
      this.velocity = direction.add(wander).add(avoidance);

      if (this.velocity.length() > 0) {
        this.velocity = this.velocity.normalize();
      }

      const old_pos = this.pos.clone();
      this.pos = this.pos.add(this.velocity.scale(this.speed));

      // Boundary wrapping
      if (this.pos.x < 0) this.pos.x = SIM_WIDTH;
      if (this.pos.x > SIM_WIDTH) this.pos.x = 0;
      if (this.pos.y < 0) this.pos.y = HEIGHT;
      if (this.pos.y > HEIGHT) this.pos.y = 0;

      this.distance_traveled += this.pos.distanceTo(old_pos);

      // Path recording
      if (this.pos.distanceTo(this.path[this.path.length - 1]) > 4) {
        this.path.push(this.pos.clone());
        if (this.path.length > 150) {
          this.path.shift();
        }
      }

      return diagnostics;
    }

    draw() {
      // Draw path
      if (this.path.length > 1) {
        ctx.strokeStyle = `rgba(${
          this.color === "rgb(255, 50, 50)" ? "255,100,100" :
          this.color === "rgb(50, 150, 255)" ? "100,150,255" :
          "100,255,100"
        }, 0.2)`;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(this.path[0].x, this.path[0].y);
        for (let i = 1; i < this.path.length; i++) {
          ctx.lineTo(this.path[i].x, this.path[i].y);
        }
        ctx.stroke();
      }

      // Draw ant
      ctx.fillStyle = this.color;
      ctx.beginPath();
      ctx.arc(this.pos.x, this.pos.y, 3, 0, Math.PI * 2);
      ctx.fill();
    }
  }

  function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
  }

  function randomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
  }

  function drawText(text, x, y, options = {}) {
    const {
      color = "#ffffff",
      font = "14px Consolas, Menlo, Monaco, monospace",
      align = "left",
    } = options;
    ctx.fillStyle = color;
    ctx.font = font;
    ctx.textAlign = align;
    ctx.fillText(text, x, y);
  }

  function generateObstacles(count) {
    const items = [];
    for (let i = 0; i < count; i++) {
      const x = randomInt(100, SIM_WIDTH - 100);
      const y = randomInt(100, HEIGHT - 100);
      const radius = randomInt(20, 45);
      items.push(new Obstacle(x, y, radius));
    }
    return items;
  }

  function generateTargets(count) {
    const colors = ["rgb(255, 50, 50)", "rgb(50, 150, 255)", "rgb(50, 255, 50)"];
    const items = [];
    for (let i = 0; i < count; i++) {
      const x = randomInt(100, SIM_WIDTH - 100);
      const y = randomInt(100, HEIGHT - 100);
      items.push(new Target(x, y, colors[i % colors.length]));
    }
    return items;
  }

  function generateAnts(count, targets) {
    const items = [];
    for (let i = 0; i < count; i++) {
      const targetIndex = i % targets.length;
      items.push(new Ant(targets[targetIndex], targetIndex));
    }
    return items;
  }

  let frameIdx = 0;
  const startTime = performance.now();
  let frameHistory = [];
  let updateHistory = [];
  let renderHistory = [];

  let targets = generateTargets(TARGET_COUNT);
  let obstacles = generateObstacles(OBSTACLE_COUNT);
  let ants = generateAnts(ANT_COUNT, targets);

  let paused = false;
  let showPaths = true;

  function resetSimulation() {
    targets = generateTargets(TARGET_COUNT);
    obstacles = generateObstacles(OBSTACLE_COUNT);
    ants = generateAnts(ANT_COUNT, targets);
  }

  function updateSimulation() {
    let avoidanceThisFrame = 0;

    for (const ant of ants) {
      const diag = ant.update(obstacles);
      avoidanceThisFrame += diag.avoidance_hits;
    }

    for (const obs of obstacles) {
      // Static obstacles, no update
    }

    return avoidanceThisFrame;
  }

  function renderArena() {
    ctx.fillStyle = "rgb(30, 30, 30)";
    ctx.fillRect(0, 0, SIM_WIDTH, HEIGHT);

    // Draw obstacles
    for (const obs of obstacles) {
      obs.draw();
    }

    // Draw targets
    for (const target of targets) {
      target.draw();
    }

    // Draw ants
    for (const ant of ants) {
      if (showPaths) {
        ant.draw();
      } else {
        // Just draw the ant without path
        ctx.fillStyle = ant.color;
        ctx.beginPath();
        ctx.arc(ant.pos.x, ant.pos.y, 3, 0, Math.PI * 2);
        ctx.fill();
      }
    }
  }

  function renderUi() {
    ctx.fillStyle = "rgb(15, 15, 20)";
    ctx.fillRect(SIM_WIDTH, 0, UI_WIDTH, HEIGHT);
    ctx.strokeStyle = "rgb(100, 100, 100)";
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(SIM_WIDTH, 0);
    ctx.lineTo(SIM_WIDTH, HEIGHT);
    ctx.stroke();

    const elapsed = (performance.now() - startTime) / 1000;
    const arrived = ants.filter((a) => a.arrived).length;
    const avgPath = ants.length > 0 ? ants.reduce((sum, a) => sum + a.distance_traveled, 0) / ants.length : 0;

    const avgFrameMs = frameHistory.length > 0 ? frameHistory.reduce((a, b) => a + b) / frameHistory.length : 0;
    const avgUpdateMs = updateHistory.length > 0 ? updateHistory.reduce((a, b) => a + b) / updateHistory.length : 0;
    const avgRenderMs = renderHistory.length > 0 ? renderHistory.reduce((a, b) => a + b) / renderHistory.length : 0;

    let y = 16;
    drawText("ANT SIMULATION", SIM_WIDTH + 14, y, {
      color: "rgb(120, 240, 180)",
      font: "bold 18px Consolas, Menlo, Monaco, monospace",
    });
    y += 32;

    const lines = [
      { text: `Elapsed: ${elapsed.toFixed(2)}s`, color: "rgb(200, 200, 200)" },
      { text: `Frame: ${frameIdx}`, color: "rgb(200, 200, 200)" },
      { text: `FPS: ${(1000 / avgFrameMs).toFixed(1)}`, color: "rgb(150, 200, 150)" },
      { text: `Frame ms: ${avgFrameMs.toFixed(2)}`, color: "rgb(180, 200, 180)" },
      { text: `Update ms: ${avgUpdateMs.toFixed(2)}`, color: "rgb(180, 200, 180)" },
      { text: `Render ms: ${avgRenderMs.toFixed(2)}`, color: "rgb(180, 200, 180)" },
      { text: `Arrived: ${arrived}/${ants.length}`, color: "rgb(100, 255, 150)" },
      { text: `Avg Path: ${avgPath.toFixed(1)} px`, color: "rgb(150, 220, 200)" },
      { text: `Obstacles: ${obstacles.length}`, color: "rgb(180, 150, 150)" },
      { text: `Targets: ${targets.length}`, color: "rgb(150, 150, 200)" },
    ];

    lines.forEach((line) => {
      drawText(line.text, SIM_WIDTH + 14, y, {
        color: line.color,
        font: "12px Consolas, Menlo, Monaco, monospace",
      });
      y += 20;
    });

    y += 8;
    drawText("KEYBOARD CONTROLS", SIM_WIDTH + 14, y, {
      color: "rgb(200, 200, 200)",
      font: "bold 14px Consolas, Menlo, Monaco, monospace",
    });
    y += 20;

    const controls = ["R - Reset Sim", "P - Pause/Play", "T - Toggle Paths"];
    controls.forEach((ctrl) => {
      drawText(ctrl, SIM_WIDTH + 14, y, {
        color: "rgb(150, 150, 180)",
        font: "12px Consolas, Menlo, Monaco, monospace",
      });
      y += 16;
    });

    y += 8;
    drawText(`Status: ${paused ? "PAUSED" : "RUNNING"}`, SIM_WIDTH + 14, y, {
      color: paused ? "rgb(255, 150, 100)" : "rgb(100, 255, 100)",
      font: "bold 12px Consolas, Menlo, Monaco, monospace",
    });
  }

  function tick(timestamp) {
    const frameStart = performance.now();

    if (!paused) {
      const updateStart = performance.now();
      updateSimulation();
      const updateEnd = performance.now();
      updateHistory.push(updateEnd - updateStart);
      if (updateHistory.length > 120) updateHistory.shift();
    }

    const renderStart = performance.now();
    renderArena();
    renderUi();
    const renderEnd = performance.now();
    renderHistory.push(renderEnd - renderStart);
    if (renderHistory.length > 120) renderHistory.shift();

    const frameEnd = performance.now();
    frameHistory.push(frameEnd - frameStart);
    if (frameHistory.length > 120) frameHistory.shift();

    frameIdx++;
    requestAnimationFrame(tick);
  }

  // Keyboard event handling
  document.addEventListener("keydown", (event) => {
    const key = event.key.toUpperCase();
    if (key === "R") {
      resetSimulation();
    } else if (key === "P") {
      paused = !paused;
    } else if (key === "T") {
      showPaths = !showPaths;
    }
  });

  // Mouse click to add targets
  canvas.addEventListener("click", (event) => {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    const x = (event.clientX - rect.left) * scaleX;
    const y = (event.clientY - rect.top) * scaleY;

    if (x < SIM_WIDTH) {
      // Add a new target
      const colors = ["rgb(255, 50, 50)", "rgb(50, 150, 255)", "rgb(50, 255, 50)"];
      const newTarget = new Target(x, y, colors[targets.length % colors.length]);
      targets.push(newTarget);

      // Reassign ants to new target
      for (const ant of ants) {
        if (!ant.arrived) {
          ant.target = targets[Math.floor(Math.random() * targets.length)];
        }
      }
    }
  });

  tick();
})();
