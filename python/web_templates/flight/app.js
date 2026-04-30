(function () {
  "use strict";

  const SIM_WIDTH = 900;
  const UI_WIDTH = 320;
  const HEIGHT = 650;
  const TOTAL_WIDTH = SIM_WIDTH + UI_WIDTH;

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

  class FlightSim {
    constructor() {
      this.pos = new Vec2(SIM_WIDTH * 0.5, HEIGHT * 0.6);
      this.vel = new Vec2(0, 0);
      this.heading = -90.0; // degrees, up is -90
      this.throttle = 0.25;
      this.max_speed = 380.0;
      this.accel = 240.0;
      this.drag = 0.14;
      this.turn_speed = 120.0;

      this.autopilot = false;
      this.target_pos = new Vec2(SIM_WIDTH * 0.7, HEIGHT * 0.3);

      this.distance_traveled = 0.0;
      this.max_speed_seen = 0.0;
    }

    forward_vec() {
      const rad = (this.heading * Math.PI) / 180;
      return new Vec2(Math.cos(rad), Math.sin(rad));
    }

    update(dt, controls) {
      const diagnostics = {
        waypoint_reached_now: false,
        thrusting_now: false,
        turning_now: false,
        braking_now: false,
      };

      let turn_input = 0.0;
      let throttle_delta = 0.0;
      let braking = false;

      // Keyboard controls
      if (controls.left) {
        turn_input -= 1.0;
      }
      if (controls.right) {
        turn_input += 1.0;
      }
      if (controls.up) {
        throttle_delta += 0.6;
        diagnostics.thrusting_now = true;
      }
      if (controls.down) {
        throttle_delta -= 0.6;
        diagnostics.braking_now = true;
        braking = true;
      }

      // Autopilot
      if (this.autopilot) {
        const to_target = this.target_pos.subtract(this.pos);
        if (to_target.length() > 0) {
          const target_heading = (Math.atan2(to_target.y, to_target.x) * 180) / Math.PI;
          let heading_diff = target_heading - this.heading;

          // Normalize heading difference to [-180, 180]
          while (heading_diff > 180) heading_diff -= 360;
          while (heading_diff < -180) heading_diff += 360;

          if (Math.abs(heading_diff) > 2) {
            turn_input = heading_diff > 0 ? 1.0 : -1.0;
          }

          // Auto-thrust to maintain speed
          if (this.vel.length() < this.max_speed * 0.7) {
            throttle_delta += 0.4;
            diagnostics.thrusting_now = true;
          } else if (this.vel.length() > this.max_speed * 0.8) {
            throttle_delta -= 0.2;
          }
        }

        if (to_target.length() < 20.0) {
          diagnostics.waypoint_reached_now = true;
        }
      }

      // Update heading
      if (Math.abs(turn_input) > 0.01) {
        this.heading += turn_input * this.turn_speed * dt;
        diagnostics.turning_now = true;
      }

      this.heading = ((this.heading % 360) + 360) % 360;

      // Update throttle
      this.throttle = Math.max(0, Math.min(1, this.throttle + throttle_delta * dt));

      // Calculate thrust
      const thrust = this.forward_vec().scale(this.accel * this.throttle);
      this.vel = this.vel.add(thrust.scale(dt));

      // Apply braking
      if (braking) {
        this.vel = this.vel.scale(0.96);
      }

      // Apply drag
      this.vel = this.vel.scale(1 - this.drag * dt);

      // Limit speed
      const speed = this.vel.length();
      if (speed > this.max_speed) {
        this.vel = this.vel.normalize().scale(this.max_speed);
      }

      const old_pos = this.pos.clone();
      this.pos = this.pos.add(this.vel.scale(dt));

      // Boundary bouncing
      if (this.pos.x < 0) {
        this.pos.x = 0;
        this.vel.x *= -0.5;
      } else if (this.pos.x > SIM_WIDTH) {
        this.pos.x = SIM_WIDTH;
        this.vel.x *= -0.5;
      }

      if (this.pos.y < 0) {
        this.pos.y = 0;
        this.vel.y *= -0.5;
      } else if (this.pos.y > HEIGHT) {
        this.pos.y = HEIGHT;
        this.vel.y *= -0.5;
      }

      this.distance_traveled += this.pos.distanceTo(old_pos);
      this.max_speed_seen = Math.max(this.max_speed_seen, speed);

      return diagnostics;
    }

    draw() {
      // Draw waypoint
      ctx.strokeStyle = "rgb(240, 210, 80)";
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(this.target_pos.x, this.target_pos.y, 8, 0, Math.PI * 2);
      ctx.stroke();

      ctx.fillStyle = "rgb(240, 210, 80)";
      ctx.beginPath();
      ctx.arc(this.target_pos.x, this.target_pos.y, 2, 0, Math.PI * 2);
      ctx.fill();

      // Draw heading line
      const nose = this.pos.add(this.forward_vec().scale(18));
      ctx.strokeStyle = "rgb(255, 255, 255)";
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(this.pos.x, this.pos.y);
      ctx.lineTo(nose.x, nose.y);
      ctx.stroke();

      // Draw aircraft triangle
      const right = new Vec2(-this.forward_vec().y, this.forward_vec().x);
      const p1 = nose;
      const p2 = this.pos.subtract(this.forward_vec().scale(10)).add(right.scale(8));
      const p3 = this.pos.subtract(this.forward_vec().scale(10)).subtract(right.scale(8));

      ctx.fillStyle = "rgb(120, 220, 255)";
      ctx.beginPath();
      ctx.moveTo(p1.x, p1.y);
      ctx.lineTo(p2.x, p2.y);
      ctx.lineTo(p3.x, p3.y);
      ctx.closePath();
      ctx.fill();
    }
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

  function drawBackground(surface) {
    ctx.fillStyle = "rgb(16, 20, 28)";
    ctx.fillRect(0, 0, SIM_WIDTH, HEIGHT);

    // Draw star field
    const numStars = 120;
    for (let i = 0; i < numStars; i++) {
      const sx = (Math.sin(i) * SIM_WIDTH + SIM_WIDTH) % SIM_WIDTH;
      const sy = (Math.cos(i * 1.3) * HEIGHT + HEIGHT) % HEIGHT;
      const sr = (i % 2) + 1;
      ctx.fillStyle = "rgb(180, 190, 210)";
      ctx.beginPath();
      ctx.arc(sx, sy, sr, 0, Math.PI * 2);
      ctx.fill();
    }
  }

  let frameIdx = 0;
  const startTime = performance.now();
  let frameHistory = [];
  let updateHistory = [];
  let renderHistory = [];

  const sim = new FlightSim();
  
  let controls = {
    left: false,
    right: false,
    up: false,
    down: false,
  };

  // Keyboard event handling
  document.addEventListener("keydown", (event) => {
    const key = event.key.toLowerCase();
    if (key === "a" || event.key === "ArrowLeft") {
      controls.left = true;
      event.preventDefault();
    }
    if (key === "d" || event.key === "ArrowRight") {
      controls.right = true;
      event.preventDefault();
    }
    if (key === "w" || event.key === "ArrowUp") {
      controls.up = true;
      event.preventDefault();
    }
    if (key === "s" || event.key === "ArrowDown") {
      controls.down = true;
      event.preventDefault();
    }
    if (key === " ") {
      sim.autopilot = !sim.autopilot;
      event.preventDefault();
    }
  });

  document.addEventListener("keyup", (event) => {
    const key = event.key.toLowerCase();
    if (key === "a" || event.key === "ArrowLeft") {
      controls.left = false;
    }
    if (key === "d" || event.key === "ArrowRight") {
      controls.right = false;
    }
    if (key === "w" || event.key === "ArrowUp") {
      controls.up = false;
    }
    if (key === "s" || event.key === "ArrowDown") {
      controls.down = false;
    }
  });

  // Mouse click to set waypoint
  canvas.addEventListener("click", (event) => {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    const x = (event.clientX - rect.left) * scaleX;
    const y = (event.clientY - rect.top) * scaleY;

    if (x < SIM_WIDTH) {
      sim.target_pos = new Vec2(x, y);
    }
  });

  function tick() {
    const frameStart = performance.now();

    const updateStart = performance.now();
    sim.update(1 / 60, controls);
    const updateEnd = performance.now();
    updateHistory.push(updateEnd - updateStart);
    if (updateHistory.length > 120) updateHistory.shift();

    const renderStart = performance.now();

    drawBackground();
    sim.draw();

    // Draw UI partition
    ctx.fillStyle = "rgb(12, 12, 16)";
    ctx.fillRect(SIM_WIDTH, 0, UI_WIDTH, HEIGHT);
    ctx.strokeStyle = "rgb(90, 90, 100)";
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(SIM_WIDTH, 0);
    ctx.lineTo(SIM_WIDTH, HEIGHT);
    ctx.stroke();

    const elapsed = (performance.now() - startTime) / 1000;
    const speed = sim.vel.length();
    const heading = sim.heading.toFixed(1);
    const dist_to_waypoint = sim.pos.distanceTo(sim.target_pos).toFixed(1);

    const avgFrameMs = frameHistory.length > 0 ? frameHistory.reduce((a, b) => a + b) / frameHistory.length : 0;
    const avgUpdateMs = updateHistory.length > 0 ? updateHistory.reduce((a, b) => a + b) / updateHistory.length : 0;
    const avgRenderMs = renderHistory.length > 0 ? renderHistory.reduce((a, b) => a + b) / renderHistory.length : 0;

    let y = 16;
    drawText("FLIGHT DIAGNOSTICS", SIM_WIDTH + 14, y, {
      color: "rgb(120, 220, 255)",
      font: "bold 18px Consolas, Menlo, Monaco, monospace",
    });
    y += 32;

    const lines = [
      { text: `Elapsed: ${elapsed.toFixed(2)}s`, color: "rgb(200, 200, 200)" },
      { text: `Frame: ${frameIdx}`, color: "rgb(200, 200, 200)" },
      { text: `FPS: ${(1000 / avgFrameMs).toFixed(1)}`, color: "rgb(150, 200, 255)" },
      { text: `Speed: ${speed.toFixed(1)} px/s`, color: "rgb(255, 200, 120)" },
      { text: `Max Speed: ${sim.max_speed_seen.toFixed(1)}`, color: "rgb(255, 150, 80)" },
      { text: `Heading: ${heading}°`, color: "rgb(200, 255, 200)" },
      { text: `Throttle: ${(sim.throttle * 100).toFixed(0)}%`, color: "rgb(255, 255, 100)" },
      { text: `Distance Traveled: ${sim.distance_traveled.toFixed(1)}`, color: "rgb(200, 200, 200)" },
      { text: `Waypoint Dist: ${dist_to_waypoint}px`, color: "rgb(240, 210, 80)" },
      { text: `Frame ms: ${avgFrameMs.toFixed(2)}`, color: "rgb(180, 200, 180)" },
      { text: `Update ms: ${avgUpdateMs.toFixed(2)}`, color: "rgb(180, 200, 180)" },
      { text: `Render ms: ${avgRenderMs.toFixed(2)}`, color: "rgb(180, 200, 180)" },
      { text: `Autopilot: ${sim.autopilot ? "ON" : "OFF"}`, color: sim.autopilot ? "rgb(100, 255, 100)" : "rgb(200, 100, 100)" },
    ];

    lines.forEach((line) => {
      drawText(line.text, SIM_WIDTH + 14, y, {
        color: line.color,
        font: "12px Consolas, Menlo, Monaco, monospace",
      });
      y += 18;
    });

    y += 8;
    drawText("KEYBOARD CONTROLS", SIM_WIDTH + 14, y, {
      color: "rgb(200, 200, 200)",
      font: "bold 14px Consolas, Menlo, Monaco, monospace",
    });
    y += 20;

    const controls_list = [
      "W/↑ - Throttle Up",
      "S/↓ - Throttle Down",
      "A/← - Turn Left",
      "D/→ - Turn Right",
      "SPACE - Autopilot Toggle",
      "CLICK - Set Waypoint",
    ];
    controls_list.forEach((ctrl) => {
      drawText(ctrl, SIM_WIDTH + 14, y, {
        color: "rgb(150, 150, 180)",
        font: "11px Consolas, Menlo, Monaco, monospace",
      });
      y += 15;
    });

    const renderEnd = performance.now();
    renderHistory.push(renderEnd - renderStart);
    if (renderHistory.length > 120) renderHistory.shift();

    const frameEnd = performance.now();
    frameHistory.push(frameEnd - frameStart);
    if (frameHistory.length > 120) frameHistory.shift();

    frameIdx++;
    requestAnimationFrame(tick);
  }

  tick();
})();
