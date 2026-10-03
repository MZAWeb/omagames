// omagym's read-only web view: rankings, runs, one run, and comparing runs.
// A hash router over four views; everything comes from the JSON API.

import {SERIES, fmt, hideTip, lineChart, scatter, showTip} from "./charts.js";
import {Player} from "./viewer.js";

const view = document.getElementById("view");
const api = async (path) => {
  const response = await fetch(`/api/${path}`);
  const body = await response.json();
  if (!response.ok) throw new Error(body.error || response.statusText);
  return body;
};
const esc = (s) => String(s ?? "").replace(/[&<>"]/g, (c) => ({"&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;"})[c]);
const label = (run) => run.name || run.id;

// -- the runs picked for comparing, kept per viewer -----------------------------

const picked = new Set(load("picked", []));
function load(key, fallback) {
  try { return JSON.parse(localStorage.getItem(`omagym.${key}`)) ?? fallback; } catch { return fallback; }
}
function save(key, value) {
  try { localStorage.setItem(`omagym.${key}`, JSON.stringify(value)); } catch { /* private window: forget it */ }
}
function togglePick(id, on) {
  on ? picked.add(id) : picked.delete(id);
  save("picked", [...picked]);
  document.getElementById("picked-count").textContent = picked.size || "";
}
document.getElementById("picked-count").textContent = picked.size || "";

// -- the runs, fetched once a page --------------------------------------------

let runsCache = null;
let groupsCache = null;
async function runs() {
  runsCache ??= await api("runs");
  return runsCache;
}
// The games that have runs: their tests, their measures, what Rankings shows.
let gamesCache = null;
async function games() {
  gamesCache ??= await api("games");
  return gamesCache;
}
const gameOf = (name) => (gamesCache || []).find((g) => g.name === name);
const measureLabel = (game, key) => gameOf(game)?.measures.find((m) => m.key === key)?.label || key.replace(/^sum_/, "");
// Groups of seeds (`run --seeds`), each as one row: its seeds' mean results.
async function groups() {
  groupsCache ??= await api("groups");
  return groupsCache;
}
const isGroup = (r) => String(r.id).startsWith("group:");
const href = (r) => isGroup(r) ? `#/compare?runs=${r.members.join(",")}` : `#/run/${r.id}`;
const tagBadges = (r) => (r.tags || []).map((t) => `<span class="badge tag">★ ${esc(t)}</span>`).join(" ");
const seedsBadge = (r) => isGroup(r) ? `<span class="badge">${r.seeds} seeds</span>` : "";
const testsOf = (list) => {
  const names = [];
  for (const r of list) for (const t of Object.keys(r.results)) if (!names.includes(t)) names.push(t);
  return names;
};
const mainValue = (r, test) => r.results[test]?.mean ?? null;

// -- filters shared by the list views -------------------------------------------

const filters = {game: "", agents: [], mix: "any", text: "", tags: [], combine: false, ...load("filters", {})};
// The game the lists show: the one picked, else the one with most runs.
function currentGame(list) {
  const counts = {};
  for (const r of list) counts[r.game] = (counts[r.game] || 0) + 1;
  if (filters.game && counts[filters.game]) return filters.game;
  return Object.keys(counts).sort((a, b) => counts[b] - counts[a])[0] || "";
}
function filterBar(list, onChange) {
  const game = currentGame(list);
  const gameNames = [...new Set(list.map((r) => r.game))].sort();
  const agents = [...new Set(list.filter((r) => r.game === game).map((r) => r.agent))].sort();
  const bar = document.createElement("div");
  bar.className = "filters";
  const tags = [...new Set(list.flatMap((r) => r.tags || []))].sort();
  bar.innerHTML = (gameNames.length > 1 ? `<label>game</label>` + gameNames.map((g) =>
      `<span class="chip ${g === game ? "on" : ""}" data-game="${g}">${g}</span>`).join("") + `<label style="margin-left:12px">agent</label>` : `<label>agent</label>`) + agents.map((a) =>
      `<span class="chip ${filters.agents.includes(a) ? "on" : ""}" data-agent="${a}">${a}</span>`).join("") +
    `<label style="margin-left:12px">training</label>
     <select data-mix><option value="any">any</option><option value="mix">on a mix</option><option value="plain">main test only</option></select>
     <input type="search" placeholder="name or notes" value="${esc(filters.text)}" data-text>
     ${tags.length ? `<label style="margin-left:12px">tag</label>` + tags.map((t) =>
       `<span class="chip ${filters.tags.includes(t) ? "on" : ""}" data-tag="${esc(t)}">★ ${esc(t)}</span>`).join("") : ""}
     <label style="margin-left:12px" title="One row per group of seeds (run --seeds), their mean">
       <input type="checkbox" data-combine ${filters.combine ? "checked" : ""}> combine seeds</label>`;
  bar.querySelector("[data-mix]").value = filters.mix;
  bar.addEventListener("click", (e) => {
    const pickGame = e.target.closest("[data-game]");
    if (pickGame) {
      // Another game: its own agents, tests and columns, so the page starts over.
      filters.game = pickGame.dataset.game;
      filters.agents = [];
      save("filters", filters); route();
      return;
    }
    const tag = e.target.closest("[data-tag]");
    if (tag) {
      const t = tag.dataset.tag;
      filters.tags = filters.tags.includes(t) ? filters.tags.filter((x) => x !== t) : [...filters.tags, t];
      tag.classList.toggle("on");
      save("filters", filters); onChange();
      return;
    }
    const chip = e.target.closest("[data-agent]");
    if (!chip) return;
    const a = chip.dataset.agent;
    filters.agents = filters.agents.includes(a) ? filters.agents.filter((x) => x !== a) : [...filters.agents, a];
    chip.classList.toggle("on");
    save("filters", filters); onChange();
  });
  bar.querySelector("[data-mix]").onchange = (e) => { filters.mix = e.target.value; save("filters", filters); onChange(); };
  bar.querySelector("[data-text]").oninput = (e) => { filters.text = e.target.value; save("filters", filters); onChange(); };
  bar.querySelector("[data-combine]").onchange = (e) => { filters.combine = e.target.checked; save("filters", filters); onChange(); };
  return bar;
}
function applyFilters(list, groupList = []) {
  const text = filters.text.toLowerCase();
  const game = currentGame(list);
  list = list.filter((r) => r.game === game);
  groupList = groupList.filter((r) => r.game === game);
  // Combined, a group's seeds give way to the group's own row.
  if (filters.combine) list = [...list.filter((r) => !r.group), ...groupList];
  return list.filter((r) =>
    (!filters.tags.length || filters.tags.some((t) => (r.tags || []).includes(t))) &&
    (!filters.agents.length || filters.agents.includes(r.agent)) &&
    (filters.mix === "any" || (filters.mix === "mix") === Object.keys(r.train_mix).length > 0) &&
    (!text || `${r.name} ${r.notes} ${r.id}`.toLowerCase().includes(text)));
}

const trained = (r) => r.trained_steps ? `${fmt(r.trained_steps)} steps` : "–";
const mixBadge = (r) => Object.keys(r.train_mix).length
  ? `<span class="badge mix" title="trained on a mix">${Object.entries(r.train_mix).map(([t, s]) => `${Math.round(s * 100)}% ${t}`).join(", ")}</span>` : "";
const pickBox = (r) => `<input type="checkbox" data-pick="${r.id}" ${picked.has(r.id) ? "checked" : ""} aria-label="pick for comparing">`;
function wirePicks(root) {
  root.querySelectorAll("[data-pick]").forEach((box) => { box.onchange = () => togglePick(box.dataset.pick, box.checked); });
}

// -- rankings ---------------------------------------------------------------------

// One table, every run, a column per thing worth ranking by; a heading sorts
// by it (best first, then the other way), and the rank follows the sort.
const rankSort = load("rankSort", {key: "marathon.mean", dir: -1});
async function rankings() {
  const all = await runs();
  const groupList = await groups();
  await games();
  view.innerHTML = `<h1>Rankings</h1><p class="muted">Every run on every test. Click a heading to rank by it; click again to reverse. Tick runs to compare them.</p>`;
  const body = document.createElement("div");
  view.appendChild(filterBar(all, () => draw()));
  view.appendChild(body);
  function draw() {
    const list = applyFilters(all.filter((r) => r.status !== "running"), groupList);
    const game = gameOf(currentGame(all));
    const tests = testsOf(list);
    const [main, other] = tests;
    const metricOf = (test) => list.find((r) => r.results[test])?.results[test];
    // [key, heading, how to read it from a run, lower is better, how to show it]
    const columns = [];
    if (main) {
      columns.push([`${main}.mean`, `${main} ${metricOf(main).metric}`, (r) => r.results[main]?.mean, metricOf(main).lower,
                    (r) => `${fmt(r.results[main].mean, 2)} <span class="muted">± ${fmt(isGroup(r) ? r.seed_spread[main] : r.results[main].std, 1)}</span>`]);
      // The game's own measures worth a column (Tetris: lines, Tetrises; 2048: highest tile).
      for (const key of game?.ranking || []) {
        const lower = game.measures.find((m) => m.key === key)?.lower;
        columns.push([`${main}.${key}`, measureLabel(game.name, key), (r) => r.results[main]?.measures?.[key], lower,
                      (r) => fmt(r.results[main].measures[key], 2)]);
      }
      columns.push([`${main}.survived`, "survived", (r) => r.results[main]?.survived, false,
                    (r) => `${fmt(r.results[main].survived)} <span class="muted">/ ${fmt(r.results[main].games)}</span>`]);
    }
    if (other) {
      columns.push([`${other}.mean`, `${other} ${metricOf(other).metric}`, (r) => r.results[other]?.mean, metricOf(other).lower,
                    (r) => `${fmt(r.results[other].mean, 2)} <span class="muted">± ${fmt(isGroup(r) ? r.seed_spread[other] : r.results[other].std, 1)}</span>`]);
      columns.push([`${other}.won`, `${other} won`, (r) => r.results[other]?.won, false,
                    (r) => `${fmt(r.results[other].won)} <span class="muted">/ ${fmt(r.results[other].games)}</span>`]);
    }
    columns.push(["trained", "trained", (r) => r.trained_steps, false, (r) => trained(r)]);
    const column = columns.find((c) => c[0] === rankSort.key) || columns[0];
    const [, , value] = column;
    const ranked = [...list].sort((a, b) => {
      const va = value(a), vb = value(b);
      if (va === vb) return 0;
      if (va === null || va === undefined) return 1;   // no value: last, whichever way
      if (vb === null || vb === undefined) return -1;
      return (va > vb ? 1 : -1) * rankSort.dir;
    });
    const otherMetric = other ? metricOf(other) : null;
    body.innerHTML = (other ? `<div class="card"><h2>${main} against ${other}</h2>
      <p class="muted">Right is a higher ${main} score; up is ${otherMetric.lower ? "a lower" : "a higher"} ${otherMetric.metric} on ${other}. Both scales are logarithmic. Click a run to open it.</p>
      <div id="map"></div></div>` : "") + `
      <div class="card"><table><thead><tr><th></th><th class="rank">#</th><th>run</th><th>agent</th>
        ${columns.map(([key, heading, , lower]) => `<th class="sort num" data-sort="${key}" title="${lower ? "lower is better" : "higher is better"}">${heading}${key === column[0] ? (rankSort.dir < 0 ? " ↓" : " ↑") : ""}</th>`).join("")}
        <th>notes</th></tr></thead><tbody>
      ${ranked.map((r, i) => `<tr><td>${pickBox(r)}</td><td class="rank">${value(r) === null || value(r) === undefined ? "" : i + 1}</td>
        <td><a href="${href(r)}">${esc(label(r))}</a> ${seedsBadge(r)} ${mixBadge(r)} ${tagBadges(r)}</td><td>${r.agent}</td>
        ${columns.map(([, , read, , show]) => `<td class="num">${read(r) === null || read(r) === undefined ? '<span class="muted">–</span>' : show(r)}</td>`).join("")}
        <td class="notes">${esc(r.notes)}</td></tr>`).join("")}
      </tbody></table></div>`;
    wirePicks(body);
    body.querySelectorAll("[data-sort]").forEach((th) => {
      th.onclick = () => {
        const [key, , , lower] = columns.find((c) => c[0] === th.dataset.sort);
        // First click: best first. Again: the other way.
        rankSort.dir = rankSort.key === key ? -rankSort.dir : (lower ? 1 : -1);
        rankSort.key = key;
        save("rankSort", rankSort); draw();
      };
    });
    if (main && other) {
      const points = list.filter((r) => mainValue(r, main) > 0 && mainValue(r, other) > 0).map((r) => ({
        x: mainValue(r, main), y: mainValue(r, other), label: label(r), href: href(r),
        tip: `<b>${esc(label(r))}</b><div class="muted">${esc(r.notes || r.agent)}</div>
              <div>${main}: <b>${fmt(mainValue(r, main))}</b></div><div>${other}: <b>${fmt(mainValue(r, other), 2)}</b></div>`,
      }));
      scatter(body.querySelector("#map"), points, {logX: true, logY: true, invertY: otherMetric.lower,
        xLabel: `${main} score →`, yLabel: `↑ ${other}: ${otherMetric.lower ? "lower" : "higher"} ${otherMetric.metric}`});
    }
  }
  draw();
}

// -- the list of runs ---------------------------------------------------------------

const sort = load("sort", {key: "started", dir: -1});
async function runList() {
  const all = await runs();
  const groupList = await groups();
  await games();
  view.innerHTML = `<div class="row spread"><h1>Runs</h1><button class="primary" data-compare>Compare picked</button></div>`;
  view.querySelector("[data-compare]").onclick = () => { location.hash = `#/compare?runs=${[...picked].join(",")}`; };
  const card = document.createElement("div");
  card.className = "card";
  view.appendChild(filterBar(all, () => draw()));
  view.appendChild(card);
  const tests = testsOf(all);
  const columns = [["name", "run"], ["agent", "agent"], ["status", "status"], ["trained_steps", "trained"],
                   ...tests.map((t) => [`test:${t}`, t]), ["started", "started"]];
  const value = (r, key) => key.startsWith("test:") ? mainValue(r, key.slice(5)) : r[key];
  function draw() {
    const list = applyFilters(all, groupList);
    list.sort((a, b) => {
      const va = value(a, sort.key), vb = value(b, sort.key);
      if (va === vb) return 0;
      if (va === null || va === undefined) return 1;
      if (vb === null || vb === undefined) return -1;
      return (va > vb ? 1 : -1) * sort.dir;
    });
    card.innerHTML = `<p class="muted">${list.length} of ${all.length} runs. Click a heading to sort.</p>
      <table><thead><tr><th><input type="checkbox" data-all aria-label="pick all shown"></th>
        ${columns.map(([k, t]) => `<th class="sort ${k.startsWith("test:") || k === "trained_steps" ? "num" : ""}" data-sort="${k}">${t}${sort.key === k ? (sort.dir > 0 ? " ↑" : " ↓") : ""}</th>`).join("")}
        <th>notes</th></tr></thead><tbody>
      ${list.map((r) => `<tr><td>${pickBox(r)}</td><td><a href="${href(r)}">${esc(label(r))}</a> ${seedsBadge(r)} ${mixBadge(r)} ${tagBadges(r)}</td>
        <td>${r.agent}</td><td>${r.status}</td><td class="num">${trained(r)}</td>
        ${tests.map((t) => `<td class="num">${fmt(mainValue(r, t), 2)}</td>`).join("")}
        <td class="muted">${(r.started || "").slice(0, 16).replace("T", " ")}</td><td class="notes">${esc(r.notes)}</td></tr>`).join("")}
      </tbody></table>`;
    wirePicks(card);
    card.querySelector("[data-all]").onchange = (e) => { list.forEach((r) => togglePick(r.id, e.target.checked)); draw(); };
    card.querySelectorAll("[data-sort]").forEach((th) => {
      th.onclick = () => {
        sort.dir = sort.key === th.dataset.sort ? -sort.dir : -1;
        sort.key = th.dataset.sort; save("sort", sort); draw();
      };
    });
  }
  draw();
}

// -- one run ------------------------------------------------------------------------

const isReward = (k) => /^(reward|fitness|shaping|death|patience)/.test(k);
function settings(config, highlight = () => false) {
  const keys = Object.keys(config).sort((a, b) => (highlight(b) - highlight(a)) || a.localeCompare(b));
  return `<dl class="kv">${keys.map((k) => `<div style="display:contents" class="${highlight(k) ? "reward" : ""}"><dt>${k}</dt><dd>${esc(JSON.stringify(config[k]))}</dd></div>`).join("")}</dl>`;
}
function patchHtml(text) {
  return esc(text).split("\n").map((line) => {
    const cls = line.startsWith("+") && !line.startsWith("+++") ? "add" : line.startsWith("-") && !line.startsWith("---") ? "del" : line.startsWith("@@") ? "hunk" : "";
    return cls ? `<span class="${cls}">${line}</span>` : line;
  }).join("\n");
}

async function runPage(id, query = new URLSearchParams()) {
  const run = await api(`run/${id}`);
  await games();
  const tests = run.tests.filter((t) => run.results[t.name]);
  view.innerHTML = `
    <div class="row spread"><div><h1>${esc(label(run))} ${mixBadge(run)} ${tagBadges(run)}</h1>
      <div class="muted">${run.agent} on ${run.game} · ${run.status} · ${trained(run)} · ${run.id}${run.group ? ` · one seed of <a href="#/compare?runs=group:${encodeURIComponent(run.group)}">${esc(run.group)}</a>` : ""}</div></div>
      <label class="chip">${pickBox(run)} compare</label></div>
    <p>${esc(run.notes) || '<span class="muted">No notes.</span>'}</p>
    <div class="grid3">${tests.map((t) => {
      const r = run.results[t.name];
      return `<div class="card"><h2>${t.name}</h2><div style="font-size:26px">${fmt(r.mean, 2)}</div>
        <div class="muted">${r.metric}, ${r.lower ? "lower" : "higher"} is better · ± ${fmt(r.std, 1)}</div>
        <div>${t.prefix ? `${fmt(r.won)} of ${fmt(r.games)} won` : `${fmt(r.survived)} of ${fmt(r.games)} survived to the cap`}${r.lines !== undefined && r.lines !== null ? ` · ${fmt(r.lines)} lines` : ""}</div></div>`;
    }).join("")}</div>
    <div class="card"><h2>Watch</h2><div class="row" id="replay-picks"></div><div id="player"><p class="muted">Pick a game above.</p></div></div>
    <div class="grid2">
      <div class="card"><h2>Rewards and settings</h2><p class="muted">What the agent was paid for comes first.</p>${settings(run.agent_config, isReward)}
        <h3>Game</h3>${settings(run.env_config)}
        ${Object.keys(run.train_mix).length ? `<h3>Training mix</h3>${settings(run.train_mix)}` : ""}
        <h3>Run</h3>${settings({seed: run.seed, device: run.device, rules_version: run.rules_version, ...run.versions})}</div>
      <div class="card"><h2>Code</h2>
        <div class="mono">${run.commit ? run.commit.slice(0, 10) : "no commit"}${run.dirty ? " + uncommitted changes" : ""}</div>
        <pre class="mono" style="white-space:pre-wrap">${esc(run.commit_message)}</pre>
        ${run.patch ? `<h3>Uncommitted changes it ran with</h3><pre class="patch">${patchHtml(run.patch)}</pre>` : '<p class="muted">It ran on the commit as it is.</p>'}</div>
    </div>
    ${Object.keys(run.curves).length ? `<div class="card"><div class="row spread"><h2>Training</h2>
      <select id="curve-key">${Object.keys(run.curves).sort().map((k) => `<option ${k === "eval/score_mean" ? "selected" : ""}>${k}</option>`).join("")}</select></div>
      <div id="curve"></div></div>` : ""}
    <div class="card"><h2>How it played</h2><p class="muted">The game's own measures over each test's games, beyond the score.</p>
      <div class="grid2">${tests.map((t) => playedTable(run, t)).join("")}</div></div>
    ${tests.map((t) => `<div class="card"><h2>${t.name}: every game</h2>${gamesTable(run, t)}</div>`).join("")}`;
  wirePicks(view);
  if (Object.keys(run.curves).length) {
    const draw = () => {
      const key = view.querySelector("#curve-key").value;
      lineChart(view.querySelector("#curve"), [{name: key, color: SERIES[0], points: run.curves[key]}], {xLabel: "training steps", yLabel: key});
    };
    view.querySelector("#curve-key").onchange = draw;
    draw();
  }
  // What can be watched: each test's best and worst, its every game, the snapshots.
  const picks = view.querySelector("#replay-picks");
  const choices = [...run.replays.map((r) => [`${r.test}: ${r.which}`, r.file])];
  if (run.snapshots.length) choices.push(["snapshots: all, in order", "snapshots"]);
  picks.innerHTML = choices.map(([text, file], i) => `<button data-file="${file}">${text}</button>`).join("") +
    Object.entries(run.games).filter(([, files]) => files.length).map(([test, files]) =>
      `<select data-games="${test}"><option value="">${test}: game…</option>${files.map((f, i) => `<option value="${f}">game ${i + 1}</option>`).join("")}</select>`).join("");
  picks.onclick = (e) => {
    const b = e.target.closest("[data-file]");
    if (!b) return;
    b.dataset.file === "snapshots" ? watchMany(run, run.snapshots) : watch(run, [b.dataset.file]);
  };
  picks.querySelectorAll("[data-games]").forEach((s) => { s.onchange = () => s.value && watch(run, [s.value]); });
  // A link can open straight onto a replay: #/run/<id>?watch=<file>.
  if (query.get("watch")) watch(run, [query.get("watch")]);
  view.querySelectorAll("[data-watch]").forEach((b) => { b.onclick = () => watch(run, [b.dataset.watch]); });
}

// One test's measures for one run: mean, spread, worst, median and best over its games.
function playedTable(run, test) {
  const g = gameOf(run.game);
  const rows = [["score", "score", false], [test.metric, test.metric, test.lower],
                ...(g?.measures.map((m) => [m.key, m.label, m.lower]) || [])];
  const seen = new Set();
  const cells = rows.filter(([key]) => !seen.has(key) && seen.add(key) && run.summary[`${test.prefix}${key}_mean`] !== undefined)
    .map(([key, label, lower]) => {
      const get = (stat) => run.summary[`${test.prefix}${key}_${stat}`];
      return `<tr><td>${esc(label)}${lower ? ' <span class="muted">(lower is better)</span>' : ""}</td>
        ${["mean", "std", "min", "median", "max"].map((stat) => `<td class="num">${fmt(get(stat), 2)}</td>`).join("")}</tr>`;
    }).join("");
  return `<div><h3>${test.name}</h3><table><thead><tr><th></th><th class="num">mean</th><th class="num">±</th>
    <th class="num">min</th><th class="num">median</th><th class="num">max</th></tr></thead><tbody>${cells}</tbody></table></div>`;
}

function gamesTable(run, test) {
  const episodes = run.episodes[test.name] || [];
  const files = run.games[test.name] || [];
  // The score, the test's own measure, and the game's measures.
  const keys = [...new Set(["score", test.metric, "steps", ...(gameOf(run.game)?.measures.map((m) => m.key) || [])])]
    .filter((k) => episodes.some((e) => e[k] !== undefined));
  return `<table><thead><tr><th>game</th>${keys.map((k) => `<th class="num">${esc(measureLabel(run.game, k))}</th>`).join("")}<th></th></tr></thead><tbody>
    ${episodes.map((e, i) => `<tr><td>${i + 1}</td>${keys.map((k) => `<td class="num">${fmt(e[k], 2)}</td>`).join("")}
      <td>${files[i] ? `<button data-watch="${files[i]}">watch</button>` : ""}</td></tr>`).join("")}</tbody></table>`;
}

async function watch(run, files, holder = view.querySelector("#player")) {
  holder.innerHTML = `<p class="muted">Drawing the replay…</p>`;
  holder.scrollIntoView({behavior: "smooth", block: "nearest"});
  try {
    const replays = await Promise.all(files.map(async (file) => ({
      title: `${esc(label(run))} <span class="muted">${file.replace("replays/", "").replace(".json", "")}</span>`,
      data: await api(`frames?run=${encodeURIComponent(run.id)}&file=${encodeURIComponent(file)}`)})));
    new Player(holder, replays);
  } catch (error) {
    holder.innerHTML = `<p class="error">${esc(error.message)}</p>`;
  }
}

// Snapshots are separate games: one after another, with buttons to step between.
async function watchMany(run, files) {
  const holder = view.querySelector("#player");
  holder.innerHTML = `<div class="row" id="snapshot-picks">${files.map((f, i) => `<button data-snap="${f}">${i + 1}</button>`).join("")}</div><div id="snapshot-player"></div>`;
  const show = (file) => {
    holder.querySelectorAll("[data-snap]").forEach((b) => b.classList.toggle("primary", b.dataset.snap === file));
    watch(run, [file], holder.querySelector("#snapshot-player"));
  };
  holder.querySelector("#snapshot-picks").onclick = (e) => { const b = e.target.closest("[data-snap]"); if (b) show(b.dataset.snap); };
  show(files[0]);
}

// -- comparing runs -------------------------------------------------------------------

async function comparePage(query) {
  const all = [...await runs(), ...await groups()];
  await games();
  // Runs by id or by name, as everywhere in omagym.
  const refs = (query.get("runs") || [...picked].join(",")).split(",").filter(Boolean);
  const chosen = refs.map((ref) => all.find((r) => r.id === ref) || all.find((r) => r.name === ref && !isGroup(r))
    || all.find((r) => isGroup(r) && r.name === ref)).filter(Boolean);
  const ids = chosen.map((r) => r.id);
  const tests = testsOf(chosen.length ? chosen : all);
  const test = query.get("test") || tests[0];
  view.innerHTML = `<h1>Compare</h1>
    <div class="card"><div class="filters"><label>runs</label>
      ${chosen.map((r, i) => `<span class="chip" data-drop="${r.id}"><span class="dot" style="background:${SERIES[i % 8]}"></span>${esc(label(r))} <span class="x">✕</span></span>`).join("")}
      <select data-add><option value="">+ add a run or a group…</option>
        <optgroup label="groups of seeds">${all.filter((r) => isGroup(r) && !ids.includes(r.id)).map((r) => `<option value="${r.id}">${esc(r.name)} (${r.seeds} seeds, ${r.agent})</option>`).join("")}</optgroup>
        <optgroup label="runs">${all.filter((r) => !isGroup(r) && !ids.includes(r.id)).map((r) => `<option value="${r.id}">${esc(label(r))} (${r.agent})</option>`).join("")}</optgroup></select>
      <label style="margin-left:12px">test</label><select data-test>${tests.map((t) => `<option ${t === test ? "selected" : ""}>${t}</option>`).join("")}</select>
      <label>rank by</label><select data-by><option value="">the test's measure</option>
        ${rankByOptions(chosen[0]?.game || currentGame(all), query.get("by"))}</select>
      <label><input type="checkbox" data-lower ${query.get("lower") === "1" ? "checked" : ""}> lower is better</label></div></div>
    <div id="compare-body">${chosen.length < 2 ? '<p class="empty">Pick two runs or more: tick them in Rankings or Runs, or add them above.</p>' : '<p class="muted">Comparing…</p>'}</div>`;
  const go = (changes) => {
    const q = new URLSearchParams(query);
    for (const [k, v] of Object.entries(changes)) v === null || v === "" ? q.delete(k) : q.set(k, v);
    location.hash = `#/compare?${q}`;
  };
  view.querySelectorAll("[data-drop]").forEach((c) => { c.onclick = () => { togglePick(c.dataset.drop, false); go({runs: ids.filter((x) => x !== c.dataset.drop).join(",")}); }; });
  view.querySelector("[data-add]").onchange = (e) => { if (e.target.value) { togglePick(e.target.value, true); go({runs: [...ids, e.target.value].join(",")}); } };
  view.querySelector("[data-test]").onchange = (e) => go({test: e.target.value, by: null, lower: null});
  view.querySelector("[data-by]").onchange = (e) => go({by: e.target.value || null});
  view.querySelector("[data-lower]").onchange = (e) => go({lower: e.target.checked ? "1" : null});
  if (chosen.length < 2) return;

  const params = new URLSearchParams({runs: ids.join(","), test});
  if (query.get("by")) params.set("by", query.get("by"));
  if (query.get("lower")) params.set("lower", query.get("lower"));
  const body = view.querySelector("#compare-body");
  let result;
  try { result = await api(`compare?${params}`); } catch (error) { body.innerHTML = `<p class="error">${esc(error.message)}</p>`; return; }
  const colorOf = Object.fromEntries(ids.map((id, i) => [id, SERIES[i % 8]]));
  const verdict = (s) => s.verdict === "best" ? '<span class="badge best">best</span>'
    : s.verdict === "worse" ? '<span class="badge worse">worse</span>'
    : s.verdict === "same" ? '<span class="badge same">same</span>' : '<span class="badge tell">can\'t tell</span>';
  const signed = (v) => (v > 0 ? "+" : "") + fmt(v, 2);
  body.innerHTML = `
    <div class="card"><h2>${result.test}: ranked by ${result.metric}, ${result.lower ? "lower" : "higher"} is better</h2>
      <p class="muted">The ${result.seeds.length} games every run played, cut at ${result.cap} steps. Each run is set against the best game by game;
        "worse" means the whole 95% range of the difference is on the wrong side of zero, "can't tell" that these games can't separate them.</p>
      <table><thead><tr><th class="rank">#</th><th>run</th><th>trained</th><th class="num">${result.metric}</th>
        <th class="num">vs best: won–tied–lost</th><th class="num">difference (95% range)</th><th>verdict</th><th>notes</th></tr></thead><tbody>
      ${result.standings.map((s, i) => `<tr><td class="rank">${i + 1}</td>
        <td><span class="dot" style="background:${colorOf[s.id]}"></span><a href="${s.id.startsWith("group:") ? `#/compare?runs=${s.id}` : `#/run/${s.id}`}">${esc(s.name || s.id)}</a></td>
        <td>${s.trained_steps ? fmt(s.trained_steps) + " steps" : "–"}</td>
        <td class="num">${fmt(s.mean, 2)} <span class="muted">± ${fmt(s.std, 1)}</span></td>
        <td class="num">${s.verdict === "best" ? "" : `${s.wins}–${s.ties}–${s.losses}`}</td>
        <td class="num">${s.verdict === "best" ? "" : `${signed(s.difference)} <span class="muted">(${signed(s.low)} to ${signed(s.high)})</span>`}</td>
        <td>${verdict(s)}</td><td class="notes">${esc(s.notes)}</td></tr>`).join("")}</tbody></table></div>
    ${styleCard(result, colorOf)}
    <div class="card"><h2>Game by game</h2>
      <p class="muted">Each column is one game, the same deal for every run; the darker a cell, the better that run did on that game among these runs. Hover for the value; click to watch the runs play that game side by side.</p>
      <div style="overflow-x:auto">${heatmap(result, colorOf)}</div></div>
    <div class="card"><h2>Watch them play the same game</h2><div id="side-by-side"><p class="muted">Click a column above.</p></div></div>
    ${differs(result, colorOf, all)}
    <div class="card"><h2>Learning curves</h2><p class="muted">The quick evaluations during training (score on short games), for the runs that trained (groups of seeds aren't drawn: open one to see its seeds). The first eight are drawn.</p><div id="curves"></div></div>`;
  wireHeatmap(body, result, all);
  // Curves are a run's: a group's seeds each have their own, so groups are left out here.
  const details = await Promise.all(ids.filter((id) => !id.startsWith("group:") && all.find((r) => r.id === id)?.trained_steps)
    .slice(0, 8).map((id) => api(`run/${id}`)));
  lineChart(body.querySelector("#curves"), details.filter((d) => d.curves["eval/score_mean"]).map((d) => ({
    name: label(d), color: colorOf[d.id], points: d.curves["eval/score_mean"]})), {xLabel: "training steps", yLabel: "quick evaluation score", empty: "None of these runs trained."});
}

// What a comparison can rank by: the score, each test's measure, the game's own.
function rankByOptions(game, current) {
  const g = gameOf(game);
  const options = [["score", "score"], ...(g?.tests.slice(1).map((t) => [t.metric, t.metric]) || []),
                   ...(g?.measures.map((m) => [m.key, m.label]) || [])];
  const seen = new Set();
  return options.filter(([k]) => !seen.has(k) && seen.add(k))
    .map(([k, label]) => `<option value="${esc(k)}" ${current === k ? "selected" : ""}>${esc(label)}</option>`).join("");
}

// How they played, beyond the ranked measure: one row per measure, the best
// of each row in bold, so a change of style shows even when the score doesn't.
function styleCard(result, colorOf) {
  if (!result.style.length) return "";
  const order = result.standings.map((s) => s.id);
  return `<div class="card"><h2>How they played</h2>
    <p class="muted">Means over the ${result.test} games; the best of each row is in bold.</p>
    <table><thead><tr><th></th>${result.standings.map((s) => `<th class="num"><span class="dot" style="background:${colorOf[s.id]}"></span>${esc(s.name || s.id)}</th>`).join("")}</tr></thead><tbody>
    ${result.style.map((row) => `<tr><td>${row.label}${row.lower ? ' <span class="muted">(lower is better)</span>' : ""}</td>
      ${order.map((_, i) => {
        const v = row.values[i];
        const best = v !== null && v === row.best && order.length > 1;
        return `<td class="num">${v === null ? "–" : best ? `<b>${fmt(v, 2)}</b>` : fmt(v, 2)}</td>`;
      }).join("")}</tr>`).join("")}</tbody></table></div>`;
}

// Settings that differ. Those every run has come first; those only some
// agents have (greedy's weights, lookahead's beam) are folded away.
function differs(result, colorOf, all) {
  const entries = Object.entries(result.differing);
  if (!entries.length) return "";
  const has = (v) => v !== null && v !== undefined;
  const shared = entries.filter(([, values]) => values.every(has));
  const partial = entries.filter(([, values]) => !values.every(has));
  const head = `<thead><tr><th>setting</th>${result.runs.map((id) =>
    `<th><span class="dot" style="background:${colorOf[id]}"></span>${esc(label(all.find((r) => r.id === id)))}</th>`).join("")}</tr></thead>`;
  const rows = (list) => list.map(([k, values]) => `<tr><td class="${isReward(k.split(".")[1]) ? "" : "muted"}">${k}</td>${values.map((v) =>
    `<td class="mono">${has(v) ? esc(JSON.stringify(v)) : "–"}</td>`).join("")}</tr>`).join("");
  return `<div class="card"><h2>What differs</h2>
    ${shared.length ? `<table>${head}<tbody>${rows(shared)}</tbody></table>` : '<p class="muted">Nothing that all of them have.</p>'}
    ${partial.length ? `<details style="margin-top:10px"><summary class="muted">${partial.length} settings only some of these agents have</summary>
      <table>${head}<tbody>${rows(partial)}</tbody></table></details>` : ""}</div>`;
}

function heatmap(result, colorOf) {
  const steps = ["--seq-0", "--seq-1", "--seq-2", "--seq-3", "--seq-4", "--seq-5"];
  const rows = result.runs.map((id) => result.per_game[id]);
  const cells = result.seeds.map((_, g) => {
    const column = rows.map((r) => r[g]).filter((v) => v !== null && v !== undefined);
    const lo = Math.min(...column), hi = Math.max(...column);
    return rows.map((r) => {
      const v = r[g];
      if (v === null || v === undefined) return null;
      let t = hi === lo ? 1 : (v - lo) / (hi - lo);
      if (result.lower) t = 1 - t;
      return {v, step: steps[Math.round(t * (steps.length - 1))], ink: t > 0.55 ? "#fff" : "var(--ink)"};
    });
  });
  const name = (id) => esc(([...runsCache, ...(groupsCache || [])].find((r) => r.id === id) || {}).name || id);
  return `<table class="heat"><thead><tr><th></th>${result.seeds.map((_, g) => `<th class="num">${g + 1}</th>`).join("")}</tr></thead><tbody>
    ${result.runs.map((id, r) => `<tr><td class="name"><span class="dot" style="background:${colorOf[id]}"></span>${name(id)}</td>
      ${cells.map((col, g) => col[r] ? `<td data-game="${g}" data-run="${id}" style="background:var(${col[r].step});color:${col[r].ink};cursor:pointer">${fmt(col[r].v, 1)}</td>` : "<td></td>").join("")}</tr>`).join("")}
    </tbody></table>`;
}

function wireHeatmap(body, result, all) {
  body.querySelectorAll("[data-game]").forEach((td) => {
    const g = Number(td.dataset.game);
    td.onmousemove = (e) => showTip(e, `<b>game ${g + 1}</b>${result.runs.map((id) => `<div>${esc(label(all.find((r) => r.id === id)))}: <b>${fmt(result.per_game[id][g], 2)}</b></div>`).join("")}`);
    td.onmouseleave = hideTip;
    td.onclick = () => { hideTip(); sideBySide(result, g, all); };
  });
}

async function sideBySide(result, game, all) {
  const holder = document.querySelector("#side-by-side");
  holder.innerHTML = `<p class="muted">Drawing game ${game + 1}…</p>`;
  holder.scrollIntoView({behavior: "smooth", block: "nearest"});
  const played = gameOf(all.find((r) => r.id === result.runs[0])?.game);
  const prefix = result.test === played?.main_test ? "replays/games" : `replays/${result.test}/games`;
  const file = `${prefix}/${String(game).padStart(2, "0")}.json`;
  const replays = [], missing = [];
  for (const id of result.runs.slice(0, 4)) {
    const run = all.find((r) => r.id === id);
    // A group has no replays of its own: its first seed plays for it.
    const source = isGroup(run) ? run.members[0] : id;
    try {
      replays.push({title: esc(label(run)) + (isGroup(run) ? ' <span class="muted">(its first seed)</span>' : ""),
                    data: await api(`frames?run=${encodeURIComponent(source)}&file=${encodeURIComponent(file)}`)});
    } catch { missing.push(label(run)); }
  }
  holder.innerHTML = (missing.length ? `<p class="muted">No replay of every game for ${missing.map(esc).join(", ")}: runs from before they were kept only have their best and worst.</p>` : "") +
    (result.runs.length > 4 ? `<p class="muted">The first four runs are shown.</p>` : "") + `<div id="sbs-player"></div>`;
  if (replays.length) new Player(holder.querySelector("#sbs-player"), replays);
}

// -- routing -------------------------------------------------------------------------

async function route() {
  hideTip();
  const [path, search] = location.hash.slice(1).split("?");
  const query = new URLSearchParams(search || "");
  const page = path.split("/")[1] || "rankings";
  document.querySelectorAll("[data-nav]").forEach((a) => a.classList.toggle("on", a.dataset.nav === page));
  try {
    if (page === "runs") await runList();
    else if (page === "run") await runPage(decodeURIComponent(path.split("/")[2]), query);
    else if (page === "compare") await comparePage(query);
    else await rankings();
  } catch (error) {
    view.innerHTML = `<p class="error">${esc(error.message)}</p>`;
  }
}
window.addEventListener("hashchange", route);
route();
