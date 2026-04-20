# snake-game

這個 repo 同時有兩種跑法：

## 1) 後端 C 語言 + 前端網頁（符合「後端是 C、前端是網頁」）

- **C 後端**：`c_backend/server.c`（HTTP server + `/api/scores` 排行榜）
- **網頁前端**：`web/`（Canvas Snake + 難度選單 + 送分數）

### 如何執行（Windows / MinGW）

1. 先安裝 MinGW-w64（讓你有 `gcc`）。
2. 在專案根目錄開 PowerShell：

```powershell
cd "C:\Users\TKU\Documents\joycewan\flask-app"
gcc -O2 -std=c11 -D_CRT_SECURE_NO_WARNINGS -o .\server.exe .\c_backend\server.c -lws2_32
.\server.exe 8080
```

然後用瀏覽器開：

- `http://127.0.0.1:8080/`

## 2) Flask（備用）

`app.py`、`templates/`、`static/` 是早期版本，用 Flask 提供網頁。