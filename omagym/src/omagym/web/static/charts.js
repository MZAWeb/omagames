// Small SVG charts: a line chart (learning curves) and a scatter (the two
// tests against each other). Thin marks, recessive axes, a tooltip on hover,
// a legend for two series or more and direct labels for up to four.

const NS = "http://www.w3.org/2000/svg";

export const SERIES = ["--series-1", "--series-2", "--series-3", "--series-4",
                       "--series-5", "--series-6", "--series-7", "--series-8"].map((v) => `var(${v})`);

export function fmt(value, digits = 1) {
  if (value === null || value === undefined || Number.isNaN(value)) return "–";
  const a = Math.abs(value);
  if (a >= 1e6) return (value / 1e6).toFixed(a >= 1e7 ? 1 : 2) + "M";
  if (a >= 1e4) return (value / 1e3).toFixed(0) + "k";
  if (a >= 1000) return Math.round(value).toLocaleString();
  if (Number.isInteger(value)) return String(value);
  return value.toFixed(digits);
}

function el(name, attrs = {}, parent) {
  const node = document.createElementNS(NS, name);
  for (const [k, v] of Object.entries(attrs)) node.setAttribute(k, v);
  if (parent) parent.appendChild(node);
  return node;
}

const tooltip = () => document.getElementById("tooltip");
export function showTip(event, html) {
  const tip = tooltip();
  tip.innerHTML = html;
  tip.hidden = false;
  const x = Math.min(event.clientX + 14, window.innerWidth - tip.offsetWidth - 8);
  tip.style.left = `${x}px`;
  tip.style.top = `${event.clientY + 14}px`;
}
export function hideTip() { tooltip().hidden = true; }

function scale(domain, range, log) {
  const [d0, d1] = log ? domain.map(Math.log10) : domain;
  const f = (v) => range[0] + ((log ? Math.log10(v) : v) - d0) / ((d1 - d0) || 1) * (range[1] - range[0]);
  f.ticks = () => {
    if (log) {
      const out = [];
      for (let e = Math.floor(d0); e <= Math.ceil(d1); e++) if (e >= d0 - 1e-9 && e <= d1 + 1e-9) out.push(10 ** e);
      return out.length ? out : [10 ** d0, 10 ** d1];
    }
    const step = niceStep((d1 - d0) / 4);
    const out = [];
    for (let v = Math.ceil(d0 / step) * step; v <= d1 + 1e-9; v += step) out.push(v);
    return out;
  };
  return f;
}
function niceStep(raw) {
  const p = 10 ** Math.floor(Math.log10(raw || 1));
  const m = raw / p;
  return (m > 5 ? 10 : m > 2 ? 5 : m > 1 ? 2 : 1) * p;
}

function axes(svg, x, y, box, opts) {
  for (const t of y.ticks()) {
    el("line", {x1: box.l, x2: box.r, y1: y(t), y2: y(t), class: "gridline"}, svg);
    el("text", {x: box.l - 6, y: y(t) + 4, "text-anchor": "end"}, svg).textContent = fmt(t);
  }
  for (const t of x.ticks()) {
    el("text", {x: x(t), y: box.b + 16, "text-anchor": "middle"}, svg).textContent = fmt(t);
  }
  el("line", {x1: box.l, x2: box.r, y1: box.b, y2: box.b, class: "axis"}, svg);
  if (opts.xLabel) el("text", {x: box.r, y: box.b + 32, "text-anchor": "end", class: "label"}, svg).textContent = opts.xLabel;
  if (opts.yLabel) el("text", {x: box.l, y: box.t - 10, class: "label"}, svg).textContent = opts.yLabel;
}

// series: [{name, color, points: [[x, y], ...]}]
export function lineChart(container, series, opts = {}) {
  container.innerHTML = "";
  const all = series.flatMap((s) => s.points);
  if (!all.length) { container.innerHTML = `<p class="empty">${opts.empty || "Nothing to plot."}</p>`; return; }
  // Drawn at the width it is shown at, so text stays its real size.
  const W = Math.max(360, container.clientWidth || 640), H = opts.height || 260;
  const box = {l: 56, r: W - (series.length > 1 && series.length <= 4 ? 110 : 16), t: 26, b: H - 40};
  const xs = all.map((p) => p[0]), ys = all.map((p) => p[1]);
  const x = scale([Math.min(...xs), Math.max(...xs)], [box.l, box.r]);
  const ylo = Math.min(0, ...ys), yhi = Math.max(...ys);
  const y = scale([ylo, yhi === ylo ? ylo + 1 : yhi], [box.b, box.t]);
  const svg = el("svg", {viewBox: `0 0 ${W} ${H}`, width: "100%", role: "img", "aria-label": opts.title || "chart"});
  axes(svg, x, y, box, opts);
  series.forEach((s) => {
    const d = s.points.map((p, i) => `${i ? "L" : "M"}${x(p[0]).toFixed(1)},${y(p[1]).toFixed(1)}`).join("");
    el("path", {d, fill: "none", stroke: s.color, "stroke-width": 2, "stroke-linejoin": "round"}, svg);
    for (const p of s.points) el("circle", {cx: x(p[0]), cy: y(p[1]), r: 3, fill: s.color}, svg);
    if (series.length > 1 && series.length <= 4 && s.points.length) {
      const last = s.points[s.points.length - 1];
      el("text", {x: x(last[0]) + 6, y: y(last[1]) + 4, class: "label"}, svg).textContent = s.name;
    }
  });
  // Crosshair: the nearest x across all series, every series' value there.
  const cross = el("line", {y1: box.t, y2: box.b, class: "axis", visibility: "hidden"}, svg);
  const hit = el("rect", {x: box.l, y: box.t, width: box.r - box.l, height: box.b - box.t, fill: "transparent"}, svg);
  hit.addEventListener("mousemove", (event) => {
    const rect = svg.getBoundingClientRect();
    const px = (event.clientX - rect.left) * (W / rect.width);
    let best = null;
    for (const v of new Set(xs)) if (best === null || Math.abs(x(v) - px) < Math.abs(x(best) - px)) best = v;
    cross.setAttribute("x1", x(best)); cross.setAttribute("x2", x(best)); cross.setAttribute("visibility", "visible");
    const rows = series.map((s) => {
      const p = s.points.find((q) => q[0] === best);
      return p ? `<div><span class="dot" style="background:${s.color}"></span>${s.name}: <b>${fmt(p[1])}</b></div>` : "";
    }).join("");
    showTip(event, `<div class="muted">${opts.xLabel || "x"} ${fmt(best)}</div>${rows}`);
  });
  hit.addEventListener("mouseleave", () => { cross.setAttribute("visibility", "hidden"); hideTip(); });
  container.appendChild(svg);
  if (series.length > 1) {
    const legend = document.createElement("div");
    legend.className = "legend";
    legend.innerHTML = series.map((s) => `<span><span class="dot" style="background:${s.color}"></span>${s.name}</span>`).join("");
    container.appendChild(legend);
  }
}

// points: [{x, y, label, tip, href}]; one series, so no legend: the title names it.
export function scatter(container, points, opts = {}) {
  container.innerHTML = "";
  if (!points.length) { container.innerHTML = `<p class="empty">${opts.empty || "Nothing to plot."}</p>`; return; }
  const W = Math.max(360, container.clientWidth || 760), H = opts.height || 380;
  const box = {l: 60, r: W - 20, t: 30, b: H - 44};
  const xs = points.map((p) => p.x), ys = points.map((p) => p.y);
  const pad = (lo, hi, log) => log ? [lo / 1.5, hi * 1.5] : [lo - (hi - lo) * 0.08, hi + (hi - lo) * 0.08];
  const x = scale(pad(Math.min(...xs), Math.max(...xs), opts.logX), [box.l, box.r], opts.logX);
  // Lower is better on y, so the axis runs downward: better is up.
  const yd = pad(Math.min(...ys), Math.max(...ys), opts.logY);
  const y = scale(opts.invertY ? yd : [yd[1], yd[0]].reverse(), opts.invertY ? [box.t, box.b] : [box.b, box.t], opts.logY);
  const svg = el("svg", {viewBox: `0 0 ${W} ${H}`, width: "100%", role: "img", "aria-label": opts.title || "scatter"});
  axes(svg, x, y, box, opts);
  // Selective labels: one that would cover a label already drawn is left
  // out (its dot still names itself on hover); near the right edge they
  // read leftward so they stay in the chart.
  const taken = [];
  const free = (r) => taken.every((t) => r.x2 < t.x1 || r.x1 > t.x2 || r.y2 < t.y1 || r.y1 > t.y2);
  // Labels go on a layer of their own, above every dot, so no dot covers one.
  const dots = el("g", {}, svg), labels = el("g", {}, svg);
  for (const p of points) {
    const g = el("g", {}, dots);
    if (p.href) g.style.cursor = "pointer";
    el("circle", {cx: x(p.x), cy: y(p.y), r: 6, fill: "var(--series-1)", stroke: "var(--surface)", "stroke-width": 2}, g);
    el("circle", {cx: x(p.x), cy: y(p.y), r: 14, fill: "transparent"}, g);
    const width = p.label.length * 7.2, left = x(p.x) + 9 + width > box.r;
    const rect = {x1: left ? x(p.x) - 9 - width : x(p.x) + 9, y1: y(p.y) - 19, y2: y(p.y) - 4};
    rect.x2 = rect.x1 + width;
    if (free(rect)) {
      taken.push(rect);
      el("text", {x: left ? x(p.x) - 9 : x(p.x) + 9, y: y(p.y) - 7, class: "label", "text-anchor": left ? "end" : "start",
                  "pointer-events": "none"}, labels).textContent = p.label;
    }
    g.addEventListener("mousemove", (event) => showTip(event, p.tip));
    g.addEventListener("mouseleave", hideTip);
    if (p.href) g.addEventListener("click", () => { hideTip(); location.hash = p.href; });
  }
  container.appendChild(svg);
}
