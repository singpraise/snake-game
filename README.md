# 貪食蛇 Snake (Flask × C)

本專題採用「前後端分離 + 跨語言整合」：
- **前端（Web UI）**：HTML + CSS + Vanilla JS（Canvas 畫面 + fetch API）
- **Web 後端**：Python 3 + Flask（HTTP 路由、模板渲染、REST API）
- **遊戲核心引擎**：C 語言（蛇移動、碰撞、吃食物、分數、難度）透過 `ctypes` 供 Flask 呼叫

> 參考你提供的範例（踩地雷 Flask × C）架構：Flask 負責 Web、核心邏輯由 C 完成。

---

## 0) GitHub 上直接玩（GitHub Pages：純前端版本）

此 repo 另外提供 `docs/` 的純前端版本，可用 GitHub Pages 直接遊玩：

1. 到 GitHub repo → **Settings → Pages**
2. Source 選 **Deploy from a branch**
3. Branch 選 **main**
4. Folder 選 **/docs**
5. Save 後等待 1–2 分鐘，GitHub 會給你一個 Pages 網址

檔案位置：
- `docs/index.html`
- `docs/snake.js`
- `docs/style.css`

---

## 1) 專案結構

- `c_core/`
  - `snake.c` / `snake.h`：C 遊戲核心
  - `build.bat`：一鍵編譯出 `c_core/build/snake.dll`
- `backend/`
  - `app.py`：Flask + ctypes 載入 DLL，提供 `/api/*`
  - `templates/index.html`：網頁頁面
  - `static/game.js`：前端控制（鍵盤、難度、每 tick 呼叫 `/api/step`）
  - `static/style.css`：版面風格

---

## 2) 如何執行（Windows）

### 2.1 先編譯 C DLL

在專案根目錄開 PowerShell：

```powershell
cd "C:\Users\TKU\Documents\joycewan\flask-app"
.\c_core\build.bat
```

成功後會產生：
- `c_core/build/snake.dll`

> 若顯示找不到編譯器，請安裝 **MinGW-w64 (gcc)** 或 **Visual Studio Build Tools (cl)**。

### 2.2 安裝 Python 依賴並啟動 Flask

```powershell
cd "C:\Users\TKU\Documents\joycewan\flask-app\backend"
python -m pip install -r requirements.txt
python app.py
```

瀏覽器開：
- `http://127.0.0.1:5000/`

---

## 3) API 介面（前端呼叫）

- `POST /api/new`：`{ "difficulty": "easy|normal|hard" }`
- `POST /api/dir`：`{ "dx": -1|0|1, "dy": -1|0|1 }`
- `POST /api/step`：遊戲走一步，回傳 state
- `GET /api/state`：取得目前 state

回傳 state 範例：

```json
{
  "width": 20,
  "height": 20,
  "score": 30,
  "alive": true,
  "food": [12, 8],
  "snake": [[10,10],[9,10],[8,10]]
}
```

---

## 4) 舊版本（保留）

專案根目錄的 `app.py`、`templates/`、`static/` 以及 `web/`、`c_backend/` 為過程中保留的早期版本，不影響目前主版本（`backend/ + c_core/`）執行。