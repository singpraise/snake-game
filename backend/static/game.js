const cv = document.getElementById("cv");
const ctx = cv.getContext("2d");
const scoreEl = document.getElementById("score");
const difficultyEl = document.getElementById("difficulty");
const restartBtn = document.getElementById("restart");
const statusEl = document.getElementById("status");

let grid = 20;
let tickMs = 110;
let size = cv.width / grid;
let timer = null;
let paused = false;
let alive = true;

function setStatus(text) {
  statusEl.textContent = text;
}

async function api(path, body) {
  const res = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body ?? {}),
  });
  if (!res.ok) throw new Error(`bad status: ${res.status}`);
  return await res.json();
}

function draw(state) {
  grid = state.width;
  size = cv.width / grid;
  alive = !!state.alive;
  scoreEl.textContent = String(state.score ?? 0);

  ctx.clearRect(0, 0, cv.width, cv.height);
  ctx.fillStyle = "#000";
  ctx.fillRect(0, 0, cv.width, cv.height);

  // food
  const [fx, fy] = state.food || [-1, -1];
  if (fx >= 0) {
    ctx.fillStyle = "#ff5959";
    ctx.fillRect(fx * size, fy * size, size, size);
  }

  // snake
  const snake = state.snake || [];
  for (let i = 0; i < snake.length; i++) {
    const [x, y] = snake[i];
    ctx.fillStyle = i === 0 ? "#7CFF7C" : "#4f4";
    ctx.fillRect(x * size, y * size, size, size);
  }

  if (!alive) {
    ctx.fillStyle = "rgba(0,0,0,0.65)";
    ctx.fillRect(0, 0, cv.width, cv.height);
    ctx.fillStyle = "#fff";
    ctx.font = "18px system-ui";
    ctx.fillText("Game Over - click 新局", 118, 220);
  } else if (paused) {
    ctx.fillStyle = "rgba(0,0,0,0.55)";
    ctx.fillRect(0, 0, cv.width, cv.height);
    ctx.fillStyle = "#fff";
    ctx.font = "18px system-ui";
    ctx.fillText("Paused (Space)", 140, 220);
  }
}

function startLoop() {
  if (timer) clearInterval(timer);
  timer = setInterval(async () => {
    if (paused || !alive) return;
    try {
      const state = await api("/api/step");
      draw(state);
    } catch (e) {
      setStatus("API 連線失敗，請確認 Flask 仍在執行。");
    }
  }, tickMs);
}

async function newGame(diff) {
  paused = false;
  setStatus("載入中...");
  const state = await api("/api/new", { difficulty: diff });
  tickMs = state.tickMs ?? tickMs;
  draw(state);
  setStatus(
    `difficulty=${state.difficulty}  grid=${state.width}x${state.height}  tick=${tickMs}ms\n` +
      `C core via ctypes: ok`
  );
  startLoop();
}

function dir(dx, dy) {
  api("/api/dir", { dx, dy }).catch(() => {});
}

document.addEventListener("keydown", (e) => {
  const k = e.key.toLowerCase();
  if (k === "arrowup" || k === "w") dir(0, -1);
  else if (k === "arrowdown" || k === "s") dir(0, 1);
  else if (k === "arrowleft" || k === "a") dir(-1, 0);
  else if (k === "arrowright" || k === "d") dir(1, 0);
  else if (k === " ") {
    paused = !paused;
  }
});

restartBtn.addEventListener("click", () => newGame(difficultyEl.value));
difficultyEl.addEventListener("change", () => newGame(difficultyEl.value));

// init
newGame(difficultyEl.value).catch((e) => {
  setStatus(String(e?.message ?? e));
});

