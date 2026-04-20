const cv = document.getElementById("cv");
const ctx = cv.getContext("2d");
const scoreEl = document.getElementById("score");
const difficultyEl = document.getElementById("difficulty");
const restartBtn = document.getElementById("restart");
const nameEl = document.getElementById("name");
const submitBtn = document.getElementById("submitScore");
const msgEl = document.getElementById("msg");
const leaderboardEl = document.getElementById("leaderboard");

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

function randCell() {
  return { x: Math.floor(Math.random() * grid), y: Math.floor(Math.random() * grid) };
}

function setDifficulty(key) {
  const d = DIFFICULTIES[key] ?? DIFFICULTIES.normal;
  grid = d.grid;
  tickMs = d.tickMs;
  size = cv.width / grid;
  startNewGame();
}

function startTimer() {
  if (timer) clearInterval(timer);
  timer = setInterval(() => {
    step();
    draw();
  }, tickMs);
}

function startNewGame() {
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

  if (snake.some((p) => p.x === next.x && p.y === next.y)) {
    alive = false;
    return;
  }

  snake.unshift(next);

  if (next.x === food.x && next.y === food.y) {
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

  drawCell(food.x, food.y, "#f44");

  for (let i = 0; i < snake.length; i++) {
    drawCell(snake[i].x, snake[i].y, i === 0 ? "#7CFF7C" : "#4f4");
  }

  if (paused && alive) {
    ctx.fillStyle = "rgba(0,0,0,0.55)";
    ctx.fillRect(0, 0, cv.width, cv.height);
    ctx.fillStyle = "#fff";
    ctx.font = "18px system-ui";
    ctx.fillText("Paused (Space)", 135, 210);
  }

  if (!alive) {
    ctx.fillStyle = "rgba(0,0,0,0.65)";
    ctx.fillRect(0, 0, cv.width, cv.height);
    ctx.fillStyle = "#fff";
    ctx.font = "18px system-ui";
    ctx.fillText("Game Over - Press R", 118, 210);
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
  else if (k === "r") startNewGame();
  else if (k === " ") {
    if (!alive) return;
    paused = !paused;
    draw();
  }
});

restartBtn?.addEventListener("click", () => startNewGame());
difficultyEl?.addEventListener("change", () => setDifficulty(difficultyEl.value));

async function refreshLeaderboard() {
  try {
    const res = await fetch("/api/scores");
    const data = await res.json();
    const scores = Array.isArray(data?.scores) ? data.scores : [];
    scores.sort((a, b) => (b.score ?? 0) - (a.score ?? 0));
    const top = scores.slice(0, 10);
    leaderboardEl.textContent =
      top.length === 0
        ? "Leaderboard: (empty)\n"
        : "Leaderboard (Top 10)\n\n" +
          top.map((s, i) => `${String(i + 1).padStart(2, " ")}. ${String(s.name ?? "anon").padEnd(16, " ")}  ${s.score ?? 0}`).join("\n");
  } catch {
    leaderboardEl.textContent = "Leaderboard: failed to load\n";
  }
}

submitBtn?.addEventListener("click", async () => {
  const name = (nameEl?.value ?? "").trim() || "anon";
  msgEl.textContent = "送出中...";
  try {
    const res = await fetch("/api/scores", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ name, score }),
    });
    if (!res.ok) throw new Error("bad status");
    msgEl.textContent = "已送出";
    await refreshLeaderboard();
  } catch {
    msgEl.textContent = "送出失敗";
  }
  setTimeout(() => (msgEl.textContent = ""), 1200);
});

// init
setDifficulty(difficultyEl?.value ?? "normal");
refreshLeaderboard();

