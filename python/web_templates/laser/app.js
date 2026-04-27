(function () {
  "use strict";

  const SIM_WIDTH = 800;
  const UI_WIDTH = 380;
  const HEIGHT = 650;
  const TOTAL_WIDTH = SIM_WIDTH + UI_WIDTH;
  const MAX_BOUNCES = 12;
  const TARGET_SAFE_RADIUS = 54;
  const GAME_STATE_PLAYING = "playing";
  const GAME_STATE_LEVEL_COMPLETE = "level_complete";
  const GAME_STATE_GAME_OVER = "game_over";

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

    dot(other) {
      return this.x * other.x + this.y * other.y;
    }

    length() {
      return Math.hypot(this.x, this.y);
    }

    lengthSquared() {
      return this.x * this.x + this.y * this.y;
    }

    normalize() {
      const len = this.length();
      return len > 0 ? this.scale(1 / len) : new Vec2(0, 0);
    }

    distanceTo(other) {
      return this.subtract(other).length();
    }
  }

  class Mirror {
    constructor(p1, p2) {
      this.p1 = new Vec2(p1.x, p1.y);
      this.p2 = new Vec2(p2.x, p2.y);

      const lineVec = this.p2.subtract(this.p1);
      const rawNormal = new Vec2(-lineVec.y, lineVec.x);
      this.normal = rawNormal.length() > 0 ? rawNormal.normalize() : new Vec2(0, 0);
    }

    draw() {
      ctx.strokeStyle = "rgb(200,255,255)";
      ctx.lineWidth = 4;
      ctx.beginPath();
      ctx.moveTo(this.p1.x, this.p1.y);
      ctx.lineTo(this.p2.x, this.p2.y);
      ctx.stroke();
    }
  }

  class Target {
    constructor(x, y, points = 20, radius = 12) {
      this.pos = new Vec2(x, y);
      this.points = points;
      this.radius = radius;
      this.active = true;
    }

    draw() {
      if (!this.active) {
        return;
      }
      drawCircle(this.pos.x, this.pos.y, gameState.targetSafeRadius(), "rgba(90,255,170,0.16)");
      ctx.strokeStyle = "rgb(100,255,150)";
      ctx.lineWidth = 3;
      ctx.beginPath();
      ctx.arc(this.pos.x, this.pos.y, this.radius, 0, Math.PI * 2);
      ctx.stroke();

      ctx.strokeStyle = "rgb(180,255,210)";
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.arc(this.pos.x, this.pos.y, Math.max(2, this.radius - 5), 0, Math.PI * 2);
      ctx.stroke();
    }
  }

  class Enemy {
    constructor(x, y, vx, vy, radius = 14) {
      this.pos = new Vec2(x, y);
      this.vel = new Vec2(vx, vy);
      this.radius = radius;
      this.active = true;
    }

    update() {
      if (!this.active) {
        return;
      }

      this.pos = this.pos.add(this.vel);

      if (this.pos.x < this.radius || this.pos.x > SIM_WIDTH - this.radius) {
        this.vel.x *= -1;
        this.pos.x = clamp(this.pos.x, this.radius, SIM_WIDTH - this.radius);
      }

      if (this.pos.y < this.radius || this.pos.y > HEIGHT - this.radius) {
        this.vel.y *= -1;
        this.pos.y = clamp(this.pos.y, this.radius, HEIGHT - this.radius);
      }
    }

    draw() {
      if (!this.active) {
        return;
      }
      drawCircle(this.pos.x, this.pos.y, this.radius, "rgb(255,90,90)");
      drawCircle(this.pos.x, this.pos.y, this.radius, "rgb(255,210,210)", 2);
    }
  }

  class GameState {
    constructor() {
      this.score = 0;
      this.highScore = 0;
      this.level = 1;
      this.state = GAME_STATE_PLAYING;
      this.totalTargets = 0;
      this.targetsHit = 0;
      this.enemyCount = 0;
      this.levelFlash = 0;
      this.gameOverFlash = 0;
      this.gameOverReason = "";
      this.timeRemaining = 0;
    }

    startLevel(totalTargets, enemyCount) {
      this.state = GAME_STATE_PLAYING;
      this.totalTargets = totalTargets;
      this.targetsHit = 0;
      this.enemyCount = enemyCount;
      this.levelFlash = 0;
      this.gameOverFlash = 0;
      this.gameOverReason = "";
      this.timeRemaining = this.levelTimeLimit();
    }

    recordTargetHit(points) {
      this.score += points;
      this.targetsHit += 1;
    }

    remainingTargets() {
      return this.totalTargets - this.targetsHit;
    }

    beamSpeed() {
      return Math.max(4, 14 - (this.level - 1) * 1.2);
    }

    targetSafeRadius() {
      return Math.min(96, TARGET_SAFE_RADIUS + (this.level - 1) * 6);
    }

    levelTimeLimit() {
      return Math.max(12, 28 - (this.level - 1) * 1.4);
    }

    update(dt) {
      this.highScore = Math.max(this.highScore, this.score);
      if (this.state === GAME_STATE_PLAYING) {
        this.timeRemaining = Math.max(0, this.timeRemaining - dt);
        if (this.timeRemaining <= 0) {
          this.triggerGameOver("Time Up");
        }
      } else if (this.state === GAME_STATE_LEVEL_COMPLETE) {
        this.levelFlash += 0.1;
      } else if (this.state === GAME_STATE_GAME_OVER) {
        this.gameOverFlash += 0.1;
      }
    }

    checkLevelComplete() {
      if (this.state === GAME_STATE_PLAYING && this.remainingTargets() === 0) {
        this.state = GAME_STATE_LEVEL_COMPLETE;
      }
    }

    triggerGameOver(reason) {
      if (this.state === GAME_STATE_PLAYING) {
        this.state = GAME_STATE_GAME_OVER;
        this.gameOverReason = reason;
      }
    }

    restartGame() {
      this.score = 0;
      this.level = 1;
      this.state = GAME_STATE_PLAYING;
      this.gameOverReason = "";
    }
  }

  function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
  }

  function randomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
  }

  function randomRange(min, max) {
    return min + Math.random() * (max - min);
  }

  function getIntersection(rayOrigin, rayDir, mirror) {
    const x1 = rayOrigin.x;
    const y1 = rayOrigin.y;
    const x2 = rayOrigin.x + rayDir.x;
    const y2 = rayOrigin.y + rayDir.y;
    const x3 = mirror.p1.x;
    const y3 = mirror.p1.y;
    const x4 = mirror.p2.x;
    const y4 = mirror.p2.y;

    const denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (denominator === 0) {
      return null;
    }

    const t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denominator;
    const u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denominator;

    if (t > 0 && u >= 0 && u <= 1) {
      return new Vec2(x1 + t * (x2 - x1), y1 + t * (y2 - y1));
    }
    return null;
  }

  function calculateReflection(incomingVec, normalVec) {
    const dotProduct = incomingVec.dot(normalVec);
    return incomingVec.subtract(normalVec.scale(2 * dotProduct)).normalize();
  }

  function pointToSegmentDistance(point, segStart, segEnd) {
    const segment = segEnd.subtract(segStart);
    const segLenSq = segment.lengthSquared();
    if (segLenSq === 0) {
      return point.distanceTo(segStart);
    }

    const t = clamp(point.subtract(segStart).dot(segment) / segLenSq, 0, 1);
    const closest = segStart.add(segment.scale(t));
    return point.distanceTo(closest);
  }

  function segmentHitsCircle(segStart, segEnd, center, radius) {
    return pointToSegmentDistance(center, segStart, segEnd) <= radius;
  }

  function canPlaceEmitter(position) {
    for (const target of targets) {
      if (target.active && position.distanceTo(target.pos) < gameState.targetSafeRadius()) {
        return false;
      }
    }
    return true;
  }

  function generateMirrors(count) {
    const items = [];
    for (let i = 0; i < count; i += 1) {
      const x = randomInt(80, SIM_WIDTH - 180);
      const y = randomInt(80, HEIGHT - 120);
      const angle = randomRange(0, Math.PI * 2);
      const length = randomInt(90, 130);
      const p2 = new Vec2(x + Math.cos(angle) * length, y + Math.sin(angle) * length);
      items.push(new Mirror(new Vec2(x, y), p2));
    }
    return items;
  }

  function generateTargets(count) {
    const items = [];
    for (let i = 0; i < count; i += 1) {
      items.push(new Target(randomInt(120, SIM_WIDTH - 120), randomInt(110, HEIGHT - 110)));
    }
    return items;
  }

  function generateEnemies(count) {
    const items = [];
    for (let i = 0; i < count; i += 1) {
      const vx = (Math.random() < 0.5 ? -1 : 1) * randomRange(1.4, 2.2);
      const vy = (Math.random() < 0.5 ? -1 : 1) * randomRange(1.2, 2.0);
      items.push(
        new Enemy(
          randomInt(150, SIM_WIDTH - 150),
          randomInt(120, HEIGHT - 120),
          vx,
          vy
        )
      );
    }
    return items;
  }

  function buildLaserPath(origin, direction) {
    const segments = [];
    const reflections = [];
    let totalDistance = 0;
    let curOrigin = origin.clone();
    let curDir = direction.clone();

    for (let bounce = 0; bounce < MAX_BOUNCES; bounce += 1) {
      let closestPoint = null;
      let hitMirror = null;
      let minDistance = Infinity;

      for (const mirror of mirrors) {
        const point = getIntersection(curOrigin, curDir, mirror);
        if (!point) {
          continue;
        }

        const distance = curOrigin.distanceTo(point);
        if (distance > 0.1 && distance < minDistance) {
          minDistance = distance;
          closestPoint = point;
          hitMirror = mirror;
        }
      }

      if (closestPoint && hitMirror) {
        segments.push([curOrigin.clone(), closestPoint.clone()]);
        reflections.push({ distance: minDistance });
        totalDistance += minDistance;

        const nextDir = calculateReflection(curDir, hitMirror.normal);
        curOrigin = closestPoint.add(nextDir.scale(0.2));
        curDir = nextDir;
      } else {
        const endPoint = curOrigin.add(curDir.scale(1200));
        segments.push([curOrigin.clone(), endPoint]);
        totalDistance += curOrigin.distanceTo(endPoint);
        break;
      }
    }

    return { segments, reflections, totalDistance };
  }

  function drawCircle(x, y, radius, color, lineWidth = 0) {
    ctx.beginPath();
    ctx.arc(x, y, radius, 0, Math.PI * 2);
    if (lineWidth > 0) {
      ctx.strokeStyle = color;
      ctx.lineWidth = lineWidth;
      ctx.stroke();
    } else {
      ctx.fillStyle = color;
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

  function roundRect(x, y, width, height, radius) {
    ctx.beginPath();
    ctx.moveTo(x + radius, y);
    ctx.lineTo(x + width - radius, y);
    ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
    ctx.lineTo(x + width, y + height - radius);
    ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
    ctx.lineTo(x + radius, y + height);
    ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
    ctx.lineTo(x, y + radius);
    ctx.quadraticCurveTo(x, y, x + radius, y);
    ctx.closePath();
  }

  function pointInRect(point, rect) {
    return (
      point.x >= rect.x &&
      point.x <= rect.x + rect.width &&
      point.y >= rect.y &&
      point.y <= rect.y + rect.height
    );
  }

  function drawButton(rect, label) {
    const hovered = hoveredButton === rect;
    ctx.fillStyle = hovered ? "rgb(46,58,72)" : "rgb(36,45,57)";
    roundRect(rect.x, rect.y, rect.width, rect.height, 6);
    ctx.fill();
    ctx.strokeStyle = "rgb(80,105,128)";
    ctx.lineWidth = 1;
    ctx.stroke();

    drawText(label, rect.x + rect.width / 2, rect.y + rect.height / 2 + 5, {
      color: "#f5f5f5",
      font: "bold 15px Consolas, Menlo, Monaco, monospace",
      align: "center",
    });
  }

  function getPointerPosition(event) {
    const rect = canvas.getBoundingClientRect();
    const scaleX = canvas.width / rect.width;
    const scaleY = canvas.height / rect.height;
    return new Vec2((event.clientX - rect.left) * scaleX, (event.clientY - rect.top) * scaleY);
  }

  const gameState = new GameState();
  let mirrorCount = 4;
  let mirrors = [];
  let targets = [];
  let enemies = [];
  let laserOrigin = new Vec2(100, HEIGHT / 2);
  let mousePos = new Vec2(100, HEIGHT / 2);
  let hoveredButton = null;
  let layoutRevision = 0;
  let beamProgress = 0;
  let lastBeamSignature = null;
  let latestReflections = [];
  let latestDistance = 0;

  const buttons = {
    shuffle: { x: SIM_WIDTH + 20, y: 270, width: UI_WIDTH - 40, height: 34 },
    reset: { x: SIM_WIDTH + 20, y: 314, width: UI_WIDTH - 40, height: 34 },
    nextLevel: { x: SIM_WIDTH + 80, y: 300, width: 220, height: 40 },
    restart: { x: SIM_WIDTH + 80, y: 352, width: 220, height: 40 },
  };

  function resetBeam() {
    beamProgress = 0;
    lastBeamSignature = null;
  }

  function initLevel() {
    mirrorCount = Math.min(6, 3 + gameState.level);
    const targetCount = Math.min(5, 2 + gameState.level);
    const enemyCount = Math.min(4, 1 + Math.floor(gameState.level / 2));

    mirrors = generateMirrors(mirrorCount);
    targets = generateTargets(targetCount);
    enemies = generateEnemies(enemyCount);
    laserOrigin = new Vec2(100, HEIGHT / 2);
    gameState.startLevel(targetCount, enemyCount);
    layoutRevision += 1;
    latestReflections = [];
    latestDistance = 0;
    resetBeam();
  }

  function handleSegmentHits(segStart, segEnd) {
    if (gameState.state !== GAME_STATE_PLAYING) {
      return;
    }

    for (const target of targets) {
      if (target.active && segmentHitsCircle(segStart, segEnd, target.pos, target.radius + 3)) {
        target.active = false;
        gameState.recordTargetHit(target.points);
      }
    }

    for (const enemy of enemies) {
      if (enemy.active && segmentHitsCircle(segStart, segEnd, enemy.pos, enemy.radius + 2)) {
        gameState.triggerGameOver("Drone Hit");
        return;
      }
    }
  }

  function updateHoveredButton(point) {
    hoveredButton = null;
    const activeButtons =
      gameState.state === GAME_STATE_PLAYING
        ? [buttons.shuffle, buttons.reset]
        : gameState.state === GAME_STATE_LEVEL_COMPLETE
          ? [buttons.nextLevel, buttons.restart]
          : [buttons.restart];

    for (const rect of activeButtons) {
      if (pointInRect(point, rect)) {
        hoveredButton = rect;
        break;
      }
    }
  }

  function handleClick(point) {
    if (gameState.state === GAME_STATE_PLAYING) {
      if (pointInRect(point, buttons.shuffle)) {
        mirrors = generateMirrors(mirrorCount);
        layoutRevision += 1;
        resetBeam();
      } else if (pointInRect(point, buttons.reset)) {
        initLevel();
      } else if (point.x < SIM_WIDTH) {
        if (canPlaceEmitter(point)) {
          laserOrigin = point.clone();
          resetBeam();
        }
      }
    } else if (gameState.state === GAME_STATE_LEVEL_COMPLETE) {
      if (pointInRect(point, buttons.nextLevel)) {
        gameState.level += 1;
        initLevel();
      } else if (pointInRect(point, buttons.restart)) {
        gameState.restartGame();
        initLevel();
      }
    } else if (gameState.state === GAME_STATE_GAME_OVER) {
      if (pointInRect(point, buttons.restart)) {
        gameState.restartGame();
        initLevel();
      }
    }
  }

  function updateSimulation() {
    gameState.update(1 / 60);

    if (gameState.state === GAME_STATE_PLAYING) {
      enemies.forEach((enemy) => enemy.update());
    }
  }

  function renderArena() {
    ctx.fillStyle = "rgb(6,8,12)";
    ctx.fillRect(0, 0, TOTAL_WIDTH, HEIGHT);
    ctx.fillStyle = "rgb(10,14,20)";
    ctx.fillRect(0, 0, SIM_WIDTH, HEIGHT);

    mirrors.forEach((mirror) => mirror.draw());
    targets.forEach((target) => target.draw());
    enemies.forEach((enemy) => enemy.draw());

    drawCircle(laserOrigin.x, laserOrigin.y, 10, "rgb(255,120,120)", 2);
    drawCircle(laserOrigin.x, laserOrigin.y, 4, "rgb(255,70,70)");
  }

  function renderLaser() {
    let laserDir =
      mousePos.x < SIM_WIDTH ? mousePos.subtract(laserOrigin) : new Vec2(1, 0);
    laserDir = laserDir.length() === 0 ? new Vec2(1, 0) : laserDir.normalize();

    const beamSignature = [
      Math.round(laserOrigin.x),
      Math.round(laserOrigin.y),
      Number(laserDir.x.toFixed(2)),
      Number(laserDir.y.toFixed(2)),
      layoutRevision,
      gameState.level,
    ].join("|");

    if (beamSignature !== lastBeamSignature) {
      beamProgress = 0;
      lastBeamSignature = beamSignature;
    }

    const path = buildLaserPath(laserOrigin, laserDir);
    if (gameState.state === GAME_STATE_PLAYING) {
      beamProgress = Math.min(path.totalDistance, beamProgress + gameState.beamSpeed());
    }

    let visibleRemaining = beamProgress;
    let visibleDistance = 0;
    const visibleReflections = [];

    for (const [segStart, segEnd] of path.segments) {
      const segmentVec = segEnd.subtract(segStart);
      const segmentLength = segmentVec.length();
      if (segmentLength === 0 || visibleRemaining <= 0) {
        break;
      }

      const drawLength = Math.min(segmentLength, visibleRemaining);
      const drawEnd = segStart.add(segmentVec.normalize().scale(drawLength));

      ctx.strokeStyle = "rgb(255,70,70)";
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(segStart.x, segStart.y);
      ctx.lineTo(drawEnd.x, drawEnd.y);
      ctx.stroke();

      handleSegmentHits(segStart, drawEnd);
      visibleDistance += drawLength;

      if (drawLength === segmentLength) {
        visibleReflections.push({ distance: segmentLength });
        drawCircle(segEnd.x, segEnd.y, 3, "rgb(255,220,220)");
      }

      visibleRemaining -= drawLength;
      if (gameState.state === GAME_STATE_GAME_OVER) {
        break;
      }
    }

    latestReflections = visibleReflections;
    latestDistance = visibleDistance;
    gameState.checkLevelComplete();
  }

  function renderUi() {
    ctx.fillStyle = "rgb(24,28,38)";
    ctx.fillRect(SIM_WIDTH, 0, UI_WIDTH, HEIGHT);
    ctx.strokeStyle = "rgb(68,74,88)";
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(SIM_WIDTH, 0);
    ctx.lineTo(SIM_WIDTH, HEIGHT);
    ctx.stroke();

    let y = 24;
    drawText("LASER PUZZLE", SIM_WIDTH + 20, y, {
      color: "rgb(120,245,210)",
      font: "bold 20px Consolas, Menlo, Monaco, monospace",
    });
    y += 34;

    drawText(
      "Clear green targets. Touching a red drone with the beam ends the round.",
      SIM_WIDTH + 20,
      y,
      {
        color: "rgb(170,185,200)",
        font: "12px Consolas, Menlo, Monaco, monospace",
      }
    );
    y += 34;

    const lines = [
      { text: `Score: ${gameState.score}`, color: "rgb(255,210,90)", font: "bold 15px Consolas, Menlo, Monaco, monospace" },
      { text: `High Score: ${gameState.highScore}`, color: "rgb(180,180,130)", font: "12px Consolas, Menlo, Monaco, monospace" },
      { text: `Level: ${gameState.level}`, color: "rgb(220,220,220)", font: "14px Consolas, Menlo, Monaco, monospace" },
      { text: `Targets Left: ${gameState.remainingTargets()}`, color: "rgb(120,255,170)", font: "14px Consolas, Menlo, Monaco, monospace" },
      { text: `Hazard Drones: ${gameState.enemyCount}`, color: "rgb(255,120,120)", font: "14px Consolas, Menlo, Monaco, monospace" },
      { text: `Safe Radius: ${gameState.targetSafeRadius()} px`, color: "rgb(150,230,190)", font: "14px Consolas, Menlo, Monaco, monospace" },
      { text: `Time Left: ${gameState.timeRemaining.toFixed(1)} s`, color: "rgb(255,205,145)", font: "14px Consolas, Menlo, Monaco, monospace" },
      { text: `Beam Speed: ${gameState.beamSpeed().toFixed(1)} px/frame`, color: "rgb(190,210,255)", font: "14px Consolas, Menlo, Monaco, monospace" },
      { text: `Visible Reflections: ${latestReflections.length}`, color: "rgb(200,200,200)", font: "14px Consolas, Menlo, Monaco, monospace" },
      { text: `Beam Distance: ${Math.floor(latestDistance)} px`, color: "rgb(200,200,200)", font: "14px Consolas, Menlo, Monaco, monospace" },
    ];

    lines.forEach((line) => {
      drawText(line.text, SIM_WIDTH + 20, y, {
        color: line.color,
        font: line.font,
      });
      y += 22;
    });

    y += 12;
    drawButton(buttons.shuffle, "Shuffle Mirrors");
    drawButton(buttons.reset, "Reset Round");

    y = 388;
    drawText("How To Play", SIM_WIDTH + 20, y, {
      color: "#f5f5f5",
      font: "bold 15px Consolas, Menlo, Monaco, monospace",
    });
    y += 24;

    [
      "1. Move the mouse to aim the pulse.",
      "2. Click in the arena to move the emitter.",
      "3. Let the pulse travel across the mirrors.",
      "4. Safety circles grow every level.",
      "5. Beat the timer before it reaches zero.",
      "6. Hit every green target and avoid red drones.",
    ].forEach((line) => {
      drawText(line, SIM_WIDTH + 20, y, {
        color: "rgb(175,185,198)",
        font: "12px Consolas, Menlo, Monaco, monospace",
      });
      y += 20;
    });

    y += 8;
    drawText("Reflection Distances", SIM_WIDTH + 20, y, {
      color: "#f5f5f5",
      font: "bold 15px Consolas, Menlo, Monaco, monospace",
    });
    y += 22;

    if (latestReflections.length > 0) {
      latestReflections.slice(0, 5).forEach((reflection, index) => {
        drawText(`#${index + 1}: ${Math.floor(reflection.distance)} px`, SIM_WIDTH + 20, y, {
          color: "rgb(165,170,182)",
          font: "12px Consolas, Menlo, Monaco, monospace",
        });
        y += 18;
      });
    } else {
      drawText("No mirror hit yet.", SIM_WIDTH + 20, y, {
        color: "rgb(120,128,140)",
        font: "12px Consolas, Menlo, Monaco, monospace",
      });
    }

    if (gameState.state === GAME_STATE_LEVEL_COMPLETE) {
      ctx.fillStyle = "rgba(8,12,18,0.8)";
      ctx.fillRect(0, 0, SIM_WIDTH, HEIGHT);

      const pulse = 80 + Math.floor(50 * (0.5 + 0.5 * Math.sin(gameState.levelFlash)));
      drawText("ROUND CLEAR", SIM_WIDTH / 2, HEIGHT / 2 - 90, {
        color: `rgb(${pulse},255,140)`,
        font: "bold 20px Consolas, Menlo, Monaco, monospace",
        align: "center",
      });

      drawText(`Score: ${gameState.score}`, SIM_WIDTH / 2, HEIGHT / 2 - 30, {
        color: "rgb(255,220,120)",
        font: "14px Consolas, Menlo, Monaco, monospace",
        align: "center",
      });

      drawText(`Ready for Level ${gameState.level + 1}?`, SIM_WIDTH / 2, HEIGHT / 2 + 8, {
        color: "rgb(210,210,210)",
        font: "14px Consolas, Menlo, Monaco, monospace",
        align: "center",
      });

      drawButton(buttons.nextLevel, "Next Level");
      drawButton(buttons.restart, "Restart Game");
    } else if (gameState.state === GAME_STATE_GAME_OVER) {
      ctx.fillStyle = "rgba(22,10,10,0.82)";
      ctx.fillRect(0, 0, SIM_WIDTH, HEIGHT);

      const flash = 140 + Math.floor(50 * (0.5 + 0.5 * Math.sin(gameState.gameOverFlash)));
      drawText(gameState.gameOverReason || "ROUND LOST", SIM_WIDTH / 2, HEIGHT / 2 - 90, {
        color: `rgb(255,${flash},${flash})`,
        font: "bold 20px Consolas, Menlo, Monaco, monospace",
        align: "center",
      });

      drawText(
        gameState.gameOverReason === "Time Up"
          ? "The timer ran out before you cleared the targets."
          : "The reflected beam touched a red drone.",
        SIM_WIDTH / 2,
        HEIGHT / 2 - 24,
        {
          color: "rgb(230,210,210)",
          font: "14px Consolas, Menlo, Monaco, monospace",
          align: "center",
        }
      );

      drawText(`Score: ${gameState.score}`, SIM_WIDTH / 2, HEIGHT / 2 + 10, {
        color: "rgb(255,220,120)",
        font: "14px Consolas, Menlo, Monaco, monospace",
        align: "center",
      });

      drawButton(buttons.restart, "Restart Game");
    }
  }

  function tick() {
    updateSimulation();
    renderArena();
    renderLaser();
    renderUi();
    requestAnimationFrame(tick);
  }

  canvas.addEventListener("mousemove", (event) => {
    mousePos = getPointerPosition(event);
    updateHoveredButton(mousePos);
  });

  canvas.addEventListener("mouseleave", () => {
    hoveredButton = null;
  });

  canvas.addEventListener("click", (event) => {
    const point = getPointerPosition(event);
    handleClick(point);
    updateHoveredButton(point);
  });

  initLevel();
  tick();
})();
