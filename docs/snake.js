const cv = document.getElementById("cv");
const ctx = cv.getContext("2d");
const scoreEl = document.getElementById("score");
const difficultyEl = document.getElementById("difficulty");
const restartBtn = document.getElementById("restart");
const statusEl = document.getElementById("status");

const DIFFICULTIES = {
  easy: { label: "簡單", grid: 18, tickMs: 140 },
  normal: { label: "普通", grid: 20, tickMs: 110 },
  hard: { label: "困難", grid: 25, tickMs: 80 },
};

let grid = DIFFICULTIES.normal.grid;
let tickMs = DIFFICULTIES.normal.tickMs;
let size = cv.width / grid;

let snake = [];
let dir = { x: 1, y: 0 };
let food = { x: 0, y: 0 };
let score = 0;
let alive = true;
let paused = false;
let timer = null;

function setStatus(text) {
  statusEl.textContent = text;
}

function randCell() {
  return { x: Math.floor(Math.random() * grid), y: Math.floor(Math.random() * grid) };
}

function startTimer() {
  if (timer) clearInterval(timer);
  timer = setInterval(() => {
    step();
    draw();
  }, tickMs);
}

function setDifficulty(key) {
  const d = DIFFICULTIES[key] ?? DIFFICULTIES.normal;
  grid = d.grid;
  tickMs = d.tickMs;
  size = cv.width / grid;
  newGame();
}

function newGame() {
  const cx = Math.floor(grid / 2);
  const cy = Math.floor(grid / 2);
  snake = [{ x: cx, y: cy }, { x: cx - 1, y: cy }, { x: cx - 2, y: cy }];
  dir = { x: 1, y: 0 };

  score = 0;
  alive = true;
  paused = false;
  scoreEl.textContent = String(score);

  do {
    food = randCell();
  } while (snake.some((p) => p.x === food.x && p.y === food.y));

  setStatus(`difficulty=${difficultyEl.value}  grid=${grid}x${grid}  tick=${tickMs}ms`);
  startTimer();
  draw();
}

function step() {
  if (!alive || paused) return;

  const head = snake[0];
  const next = { x: head.x + dir.x, y: head.y + dir.y };

  if (next.x < 0 || next.x >= grid || next.y < 0 || next.y >= grid) {
    alive = false;
    return;
  }

  // allow moving into tail when not growing
  const tail = snake[snake.length - 1];
  const willGrow = next.x === food.x && next.y === food.y;
  const hitSelf = snake.some((p, i) => {
    if (!willGrow && i === snake.length - 1) return false;
    return p.x === next.x && p.y === next.y;
  });
  if (hitSelf) {
    alive = false;
    return;
  }

  snake.unshift(next);
  if (willGrow) {
    score += 10;
    scoreEl.textContent = String(score);
    do {
      food = randCell();
    } while (snake.some((p) => p.x === food.x && p.y === food.y));
  } else {
    snake.pop();
  }
}

function drawCell(x, y, color) {
  ctx.fillStyle = color;
  ctx.fillRect(x * size, y * size, size, size);
}

function draw() {
  ctx.clearRect(0, 0, cv.width, cv.height);
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, cv.width, cv.height);

  drawCell(food.x, food.y, "#ff5959");

  for (let i = 0; i < snake.length; i++) {
    const p = snake[i];
    drawCell(p.x, p.y, i === 0 ? "#7CFF7C" : "#4f4");
  }

  if (!alive) {
    ctx.fillStyle = "rgba(0,0,0,0.65)";
    ctx.fillRect(0, 0, cv.width, cv.height);
    ctx.fillStyle = "#fff";
    ctx.font = "18px system-ui";
    ctx.fillText("Game Over - Press R", 120, 220);
  } else if (paused) {
    ctx.fillStyle = "rgba(0,0,0,0.55)";
    ctx.fillRect(0, 0, cv.width, cv.height);
    ctx.fillStyle = "#fff";
    ctx.font = "18px system-ui";
    ctx.fillText("Paused (Space)", 145, 220);
  }
}

function trySetDir(nextDir) {
  if (dir.x + nextDir.x === 0 && dir.y + nextDir.y === 0) return;
  dir = nextDir;
}

document.addEventListener("keydown", (e) => {
  const k = e.key.toLowerCase();
  if (k === "arrowup" || k === "w") trySetDir({ x: 0, y: -1 });
  else if (k === "arrowdown" || k === "s") trySetDir({ x: 0, y: 1 });
  else if (k === "arrowleft" || k === "a") trySetDir({ x: -1, y: 0 });
  else if (k === "arrowright" || k === "d") trySetDir({ x: 1, y: 0 });
  else if (k === "r") newGame();
  else if (k === " ") {
    paused = !paused;
  }
});

restartBtn.addEventListener("click", () => newGame());
difficultyEl.addEventListener("change", () => setDifficulty(difficultyEl.value));

// init
setDifficulty(difficultyEl.value);

