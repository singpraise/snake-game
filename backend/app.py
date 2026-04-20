import ctypes
import os
import time
from dataclasses import dataclass
from typing import Any

from flask import Flask, jsonify, render_template, request


BASE_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(BASE_DIR)
DLL_PATH = os.path.join(PROJECT_ROOT, "c_core", "build", "snake.dll")


class SnakeCore:
    def __init__(self, dll_path: str):
        if not os.path.exists(dll_path):
            raise FileNotFoundError(
                f"找不到 C DLL：{dll_path}\n"
                "請先在專案根目錄執行：c_core\\build.bat 產生 snake.dll"
            )

        self.dll = ctypes.CDLL(dll_path)

        self.dll.snake_create.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_uint]
        self.dll.snake_create.restype = ctypes.c_void_p

        self.dll.snake_destroy.argtypes = [ctypes.c_void_p]
        self.dll.snake_destroy.restype = None

        self.dll.snake_reset.argtypes = [ctypes.c_void_p, ctypes.c_uint]
        self.dll.snake_reset.restype = None

        self.dll.snake_set_dir.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int]
        self.dll.snake_set_dir.restype = None

        self.dll.snake_step.argtypes = [ctypes.c_void_p]
        self.dll.snake_step.restype = ctypes.c_int

        self.dll.snake_get_width.argtypes = [ctypes.c_void_p]
        self.dll.snake_get_width.restype = ctypes.c_int

        self.dll.snake_get_height.argtypes = [ctypes.c_void_p]
        self.dll.snake_get_height.restype = ctypes.c_int

        self.dll.snake_get_score.argtypes = [ctypes.c_void_p]
        self.dll.snake_get_score.restype = ctypes.c_int

        self.dll.snake_get_alive.argtypes = [ctypes.c_void_p]
        self.dll.snake_get_alive.restype = ctypes.c_int

        self.dll.snake_get_food_x.argtypes = [ctypes.c_void_p]
        self.dll.snake_get_food_x.restype = ctypes.c_int

        self.dll.snake_get_food_y.argtypes = [ctypes.c_void_p]
        self.dll.snake_get_food_y.restype = ctypes.c_int

        self.dll.snake_get_snake_len.argtypes = [ctypes.c_void_p]
        self.dll.snake_get_snake_len.restype = ctypes.c_int

        self.dll.snake_get_snake_xy.argtypes = [
            ctypes.c_void_p,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
        ]
        self.dll.snake_get_snake_xy.restype = None

    def create(self, width: int, height: int, seed: int) -> ctypes.c_void_p:
        ptr = self.dll.snake_create(width, height, ctypes.c_uint(seed))
        if not ptr:
            raise RuntimeError("snake_create failed")
        return ctypes.c_void_p(ptr)

    def destroy(self, ptr: ctypes.c_void_p) -> None:
        if ptr and ptr.value:
            self.dll.snake_destroy(ptr)

    def reset(self, ptr: ctypes.c_void_p, seed: int) -> None:
        self.dll.snake_reset(ptr, ctypes.c_uint(seed))

    def set_dir(self, ptr: ctypes.c_void_p, dx: int, dy: int) -> None:
        self.dll.snake_set_dir(ptr, dx, dy)

    def step(self, ptr: ctypes.c_void_p) -> int:
        return int(self.dll.snake_step(ptr))

    def state(self, ptr: ctypes.c_void_p) -> dict[str, Any]:
        w = int(self.dll.snake_get_width(ptr))
        h = int(self.dll.snake_get_height(ptr))
        score = int(self.dll.snake_get_score(ptr))
        alive = bool(self.dll.snake_get_alive(ptr))
        food = [int(self.dll.snake_get_food_x(ptr)), int(self.dll.snake_get_food_y(ptr))]

        n = int(self.dll.snake_get_snake_len(ptr))
        snake: list[list[int]] = []
        x = ctypes.c_int()
        y = ctypes.c_int()
        for i in range(n):
            self.dll.snake_get_snake_xy(ptr, i, ctypes.byref(x), ctypes.byref(y))
            snake.append([int(x.value), int(y.value)])

        return {"width": w, "height": h, "score": score, "alive": alive, "food": food, "snake": snake}


DIFFICULTIES: dict[str, dict[str, int]] = {
    "easy": {"grid": 18, "tickMs": 140},
    "normal": {"grid": 20, "tickMs": 110},
    "hard": {"grid": 25, "tickMs": 80},
}


@dataclass
class GameHolder:
    ptr: ctypes.c_void_p | None = None
    difficulty: str = "normal"


core = SnakeCore(DLL_PATH)
holder = GameHolder()

app = Flask(__name__)


def new_seed() -> int:
    return int(time.time() * 1000) & 0xFFFFFFFF


def ensure_game(difficulty: str | None = None) -> None:
    diff = difficulty or holder.difficulty or "normal"
    if diff not in DIFFICULTIES:
        diff = "normal"
    holder.difficulty = diff

    grid = DIFFICULTIES[diff]["grid"]
    if holder.ptr is None or holder.ptr.value is None:
        holder.ptr = core.create(grid, grid, new_seed())
    else:
        # recreate if size changes
        w = core.dll.snake_get_width(holder.ptr)
        h = core.dll.snake_get_height(holder.ptr)
        if int(w) != grid or int(h) != grid:
            core.destroy(holder.ptr)
            holder.ptr = core.create(grid, grid, new_seed())
        else:
            core.reset(holder.ptr, new_seed())


@app.get("/")
def home():
    return render_template("index.html")


@app.post("/api/new")
def api_new():
    data = request.get_json(silent=True) or {}
    diff = str(data.get("difficulty") or "normal")
    ensure_game(diff)
    state = core.state(holder.ptr)  # type: ignore[arg-type]
    state["difficulty"] = holder.difficulty
    state["tickMs"] = DIFFICULTIES[holder.difficulty]["tickMs"]
    return jsonify(state)


@app.get("/api/state")
def api_state():
    if holder.ptr is None or holder.ptr.value is None:
        ensure_game("normal")
    state = core.state(holder.ptr)  # type: ignore[arg-type]
    state["difficulty"] = holder.difficulty
    state["tickMs"] = DIFFICULTIES[holder.difficulty]["tickMs"]
    return jsonify(state)


@app.post("/api/dir")
def api_dir():
    if holder.ptr is None or holder.ptr.value is None:
        ensure_game("normal")
    data = request.get_json(silent=True) or {}
    dx = int(data.get("dx") or 0)
    dy = int(data.get("dy") or 0)
    core.set_dir(holder.ptr, dx, dy)  # type: ignore[arg-type]
    return jsonify({"ok": True})


@app.post("/api/step")
def api_step():
    if holder.ptr is None or holder.ptr.value is None:
        ensure_game("normal")
    core.step(holder.ptr)  # type: ignore[arg-type]
    state = core.state(holder.ptr)  # type: ignore[arg-type]
    return jsonify(state)


if __name__ == "__main__":
    app.run(host="127.0.0.1", port=5000, debug=True)

