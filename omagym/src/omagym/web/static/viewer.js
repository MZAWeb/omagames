// The replay viewer. The frames come from the game's own engine
// (og_replay_frames, through /api/frames): the well after the deal and after
// every piece that locks, so the browser only draws, it never plays.
//
// One Player drives one or more boards: put two runs' replays of the same
// deal in one Player and they step together, piece by piece.

const PIECES = {I: "--series-1", L: "--series-2", S: "--series-3", O: "--series-4",
                T: "--series-5", Z: "--series-6", J: "--series-7"};
const color = (letter) => `var(${PIECES[letter]})`;
const CELL = 18;

class Board {
  constructor(container, data, title) {
    this.data = data;
    const visible = data.height - data.hidden_rows;
    container.insertAdjacentHTML("beforeend", `
      <div class="viewer">
        <canvas class="well" width="${data.width * CELL}" height="${visible * CELL}"></canvas>
        <div class="side">
          <div class="title">${title}</div>
          <div class="muted" data-ended></div>
          <h3>Next</h3><canvas data-queue width="${4 * 12}" height="${3 * 3 * 12}"></canvas>
          <h3>Hold</h3><canvas data-hold width="${4 * 12}" height="${3 * 12}"></canvas>
          <h3>Game</h3><div data-stats></div>
        </div>
      </div>`);
    const root = container.lastElementChild;
    this.well = root.querySelector("canvas.well").getContext("2d");
    this.queue = root.querySelector("[data-queue]").getContext("2d");
    this.hold = root.querySelector("[data-hold]").getContext("2d");
    this.stats = root.querySelector("[data-stats]");
    root.querySelector("[data-ended]").textContent = `${data.mode}, ${data.ended}`;
  }

  get length() { return this.data.frames.length; }

  draw(index) {
    const frame = this.data.frames[Math.min(index, this.length - 1)];
    const {width, height, hidden_rows: hidden} = this.data;
    const css = getComputedStyle(document.body);
    const resolve = (v) => css.getPropertyValue(v.slice(4, -1)).trim();
    const ctx = this.well;
    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    const placed = new Set(index < this.length ? frame.placed || [] : []);
    for (let row = hidden; row < height; row++) {
      for (let col = 0; col < width; col++) {
        const i = row * width + col;
        const letter = frame.board[i];
        if (letter === ".") continue;
        ctx.fillStyle = resolve(color(letter));
        ctx.fillRect(col * CELL + 1, (row - hidden) * CELL + 1, CELL - 2, CELL - 2);
        if (placed.has(i)) {
          // The piece that just landed, ringed in ink.
          ctx.strokeStyle = resolve("var(--ink)");
          ctx.lineWidth = 2;
          ctx.strokeRect(col * CELL + 2, (row - hidden) * CELL + 2, CELL - 4, CELL - 4);
        }
      }
    }
    mini(this.queue, frame.queue, resolve);
    mini(this.hold, frame.hold === "." ? [] : [frame.hold], resolve);
    const rows = [["piece", `${Math.min(index, this.length - 1)} / ${this.length - 1}`], ["score", frame.score.toLocaleString()],
                  ["lines", frame.lines], ["level", frame.level]];
    if (frame.dealt_rows_left !== undefined) rows.push(["rows left", frame.dealt_rows_left], ["difficulty", frame.difficulty]);
    this.stats.innerHTML = rows.map(([k, v]) => `<div class="stat"><span class="muted">${k}</span><span>${v}</span></div>`).join("");
  }
}

// Spawn shapes, enough to show what is coming.
const SHAPES = {I: [[0, 1], [1, 1], [2, 1], [3, 1]], O: [[1, 0], [2, 0], [1, 1], [2, 1]], T: [[1, 0], [0, 1], [1, 1], [2, 1]],
                S: [[1, 0], [2, 0], [0, 1], [1, 1]], Z: [[0, 0], [1, 0], [1, 1], [2, 1]], J: [[0, 0], [0, 1], [1, 1], [2, 1]],
                L: [[2, 0], [0, 1], [1, 1], [2, 1]]};
function mini(ctx, letters, resolve) {
  ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
  letters.forEach((letter, n) => {
    ctx.fillStyle = resolve(color(letter));
    for (const [x, y] of SHAPES[letter] || []) ctx.fillRect(x * 12 + 1, (n * 3 + y) * 12 + 1, 10, 10);
  });
}

// A 2048 board: 4 x 4 tiles, shaded by size on the sequential ramp (bigger
// is darker), each with its number.
const TILE = 64;
const RAMP = ["--seq-0", "--seq-1", "--seq-2", "--seq-3", "--seq-4", "--seq-5"];
class Grid2048 {
  constructor(container, data, title) {
    this.data = data;
    const side = data.size * TILE;
    container.insertAdjacentHTML("beforeend", `
      <div class="viewer">
        <canvas class="well" width="${side}" height="${side}"></canvas>
        <div class="side"><div class="title">${title}</div><div class="muted">${data.ended}</div>
          <h3>Game</h3><div data-stats></div></div>
      </div>`);
    const root = container.lastElementChild;
    this.ctx = root.querySelector("canvas").getContext("2d");
    this.stats = root.querySelector("[data-stats]");
  }

  get length() { return this.data.frames.length; }

  draw(index) {
    const frame = this.data.frames[Math.min(index, this.length - 1)];
    const css = getComputedStyle(document.body);
    const ctx = this.ctx, n = this.data.size;
    ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    frame.board.forEach((value, i) => {
      if (!value) return;
      const power = Math.log2(value);
      const step = Math.min(RAMP.length - 1, Math.floor((power - 1) / 2));
      const x = (i % n) * TILE, y = Math.floor(i / n) * TILE;
      ctx.fillStyle = css.getPropertyValue(RAMP[step]).trim();
      ctx.fillRect(x + 3, y + 3, TILE - 6, TILE - 6);
      ctx.fillStyle = step >= 3 ? "#fff" : css.getPropertyValue("--ink").trim();
      ctx.font = `600 ${value >= 1024 ? 16 : 20}px system-ui, sans-serif`;
      ctx.fillText(String(value), x + TILE / 2, y + TILE / 2);
    });
    const rows = [["move", `${Math.min(index, this.length - 1)} / ${this.length - 1}`], ["last", frame.move || "–"],
                  ["score", frame.score.toLocaleString()], ["highest", frame.highest]];
    this.stats.innerHTML = rows.map(([k, v]) => `<div class="stat"><span class="muted">${k}</span><span>${v}</span></div>`).join("");
  }
}

// Which board draws which game's frames.
const BOARDS = {omatris: Board, oma2048: Grid2048};

export class Player {
  // replays: [{title, data}] (data from /api/frames)
  constructor(container, replays) {
    container.innerHTML = `
      <div class="player">
        <button data-act="start" title="Back to the deal">⏮</button>
        <button data-act="back" title="Previous piece (←)">◀</button>
        <button data-act="play" class="primary" title="Play / pause (space)">▶</button>
        <button data-act="next" title="Next piece (→)">▶|</button>
        <input type="range" min="0" value="0" data-act="seek" aria-label="piece">
        <label class="muted">speed <select data-act="speed">
          <option value="2">2 moves/s</option><option value="4" selected>4 moves/s</option>
          <option value="10">10 moves/s</option><option value="40">40 moves/s</option></select></label>
      </div>
      <div class="viewers"></div>`;
    const boards = container.querySelector(".viewers");
    this.boards = replays.map((r) => {
      const Kind = BOARDS[r.data.game];
      if (!Kind) throw new Error(`this page can't draw ${r.data.game} yet`);
      return new Kind(boards, r.data, r.title);
    });
    this.length = Math.max(...this.boards.map((b) => b.length));
    this.index = 0;
    this.timer = null;
    this.speed = 4;
    const seek = container.querySelector("[data-act=seek]");
    seek.max = this.length - 1;
    this.seek = seek;
    this.playButton = container.querySelector("[data-act=play]");
    container.querySelector("[data-act=start]").onclick = () => this.go(0);
    container.querySelector("[data-act=back]").onclick = () => this.go(this.index - 1);
    container.querySelector("[data-act=next]").onclick = () => this.go(this.index + 1);
    this.playButton.onclick = () => this.toggle();
    seek.oninput = () => this.go(Number(seek.value));
    container.querySelector("[data-act=speed]").onchange = (e) => { this.speed = Number(e.target.value); if (this.timer) { this.stop(); this.play(); } };
    this.keys = (e) => {
      if (!document.body.contains(container)) { document.removeEventListener("keydown", this.keys); this.stop(); return; }
      if (e.target.closest("input, select")) return;
      if (e.key === " ") { e.preventDefault(); this.toggle(); }
      else if (e.key === "ArrowRight") this.go(this.index + 1);
      else if (e.key === "ArrowLeft") this.go(this.index - 1);
    };
    document.addEventListener("keydown", this.keys);
    this.go(0);
  }

  go(index) {
    this.index = Math.max(0, Math.min(index, this.length - 1));
    this.seek.value = this.index;
    for (const b of this.boards) b.draw(this.index);
    if (this.index >= this.length - 1) this.stop();
  }

  play() {
    if (this.index >= this.length - 1) this.go(0);
    this.playButton.textContent = "⏸";
    this.timer = setInterval(() => this.go(this.index + 1), 1000 / this.speed);
  }

  stop() {
    clearInterval(this.timer);
    this.timer = null;
    this.playButton.textContent = "▶";
  }

  toggle() { this.timer ? this.stop() : this.play(); }
}
