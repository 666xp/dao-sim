import { useEffect, useMemo, useRef, useState } from "react";
import uPlot from "uplot";
import "uplot/dist/uPlot.min.css";
import { bitsString, connectWs, type Telemetry, type WsClient } from "./telemetry";

const MAX_POINTS = 800;
const CARDINALS = ["东", "西", "南", "北", "上", "下"];

function useSeries() {
  const ticks = useRef<number[]>([]);
  const D = useRef<number[]>([]);
  const sigma = useRef<number[]>([]);
  const rates = useRef<number[][]>([[], [], [], [], [], []]);
  const bits = useRef<string[]>([]);

  const push = (t: Telemetry) => {
    ticks.current.push(t.tick);
    D.current.push(t.D);
    sigma.current.push(t.sigma);
    for (let i = 0; i < 6; i++) rates.current[i].push(t.rates[i] ?? 0);
    bits.current.push(bitsString(t.outBits));
    if (ticks.current.length > MAX_POINTS) {
      const cut = ticks.current.length - MAX_POINTS;
      ticks.current.splice(0, cut);
      D.current.splice(0, cut);
      sigma.current.splice(0, cut);
      for (let i = 0; i < 6; i++) rates.current[i].splice(0, cut);
      bits.current.splice(0, cut);
    }
  };

  return { ticks, D, sigma, rates, bits, push };
}

function Plot({
  title,
  xs,
  ys,
  labels,
}: {
  title: string;
  xs: number[];
  ys: number[][];
  labels: string[];
}) {
  const ref = useRef<HTMLDivElement>(null);
  const plot = useRef<uPlot | null>(null);

  useEffect(() => {
    if (!ref.current) return;
    const series: uPlot.Series[] = [{}, ...labels.map((l) => ({ label: l, stroke: undefined }))];
    const colors = ["#3db8a0", "#d4a017", "#6ea8fe", "#c084fc", "#fb923c", "#f472b6", "#94a3b8"];
    for (let i = 1; i < series.length; i++) {
      series[i].stroke = colors[(i - 1) % colors.length];
      series[i].width = 1.5;
    }
    plot.current = new uPlot(
      {
        width: ref.current.clientWidth,
        height: 180,
        title,
        series,
        scales: { x: { time: false } },
        axes: [{ stroke: "#8aa0b5" }, { stroke: "#8aa0b5" }],
      },
      [xs, ...ys],
      ref.current,
    );
    const onResize = () => {
      if (ref.current && plot.current) {
        plot.current.setSize({ width: ref.current.clientWidth, height: 180 });
      }
    };
    window.addEventListener("resize", onResize);
    return () => {
      window.removeEventListener("resize", onResize);
      plot.current?.destroy();
      plot.current = null;
    };
  }, [title, labels]);

  useEffect(() => {
    plot.current?.setData([xs, ...ys]);
  }, [xs, ys]);

  return <div ref={ref} />;
}

function hamming(a: string[], b: string[]): number {
  const n = Math.min(a.length, b.length);
  if (!n) return 0;
  let diff = 0;
  for (let i = 0; i < n; i++) {
    for (let j = 0; j < 12; j++) {
      if ((a[i][j] ?? "0") !== (b[i][j] ?? "0")) diff++;
    }
  }
  return diff / (n * 12);
}

export default function App() {
  const series = useSeries();
  const [status, setStatus] = useState("未连接");
  const [seed, setSeed] = useState(42);
  const [stepN, setStepN] = useState(10000);
  const [lastBits, setLastBits] = useState("000000000000");
  const [tick, setTick] = useState(0);
  const [daoAdapt, setDaoAdapt] = useState(true);
  const [noise, setNoise] = useState(true);
  const [checklist, setChecklist] = useState<string[]>([]);
  const client = useRef<WsClient | null>(null);
  const baselineBits = useRef<string[]>([]);
  const [, bump] = useState(0);

  useEffect(() => {
    const c = connectWs(
      "ws://127.0.0.1:3848",
      (t) => {
        series.push(t);
        setLastBits(bitsString(t.outBits));
        setTick(t.tick);
        bump((x) => x + 1);
      },
      setStatus,
    );
    client.current = c;
    return () => c.close();
  }, []);

  const send = (obj: unknown) => client.current?.send(obj);

  const xs = series.ticks.current;
  const dYs = [series.D.current];
  const sYs = [series.sigma.current];
  const rYs = series.rates.current;

  const rasterRef = useRef<HTMLCanvasElement>(null);
  useEffect(() => {
    const canvas = rasterRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    const w = canvas.width;
    const h = canvas.height;
    ctx.fillStyle = "#0a1016";
    ctx.fillRect(0, 0, w, h);
    const data = series.bits.current.slice(-w);
    for (let x = 0; x < data.length; x++) {
      for (let b = 0; b < 12; b++) {
        if (data[x][b] === "1") {
          ctx.fillStyle = "#3db8a0";
          ctx.fillRect(x, b * (h / 12), 1, h / 12);
        }
      }
    }
  });

  const runAblationChecks = () => {
    const lines: string[] = [];
    const cur = series.bits.current.slice(-200);
    if (baselineBits.current.length === 0) {
      baselineBits.current = [...cur];
      lines.push("PASS: baseline_bits_captured " + baselineBits.current.length);
    } else {
      const ham = hamming(baselineBits.current, cur);
      lines.push(
        (ham > 0.02 ? "PASS" : "FAIL") + ": bitstream_diverged_from_baseline " + ham.toFixed(4),
      );
    }
    const zc = series.D.current.reduce((acc, v, i, arr) => {
      if (i === 0) return 0;
      return acc + ((arr[i - 1] < 0 && v >= 0) || (arr[i - 1] >= 0 && v < 0) ? 1 : 0);
    }, 0);
    lines.push((zc >= 1 ? "PASS" : "FAIL") + ": dao_zero_crossings " + zc);
    const sig = series.sigma.current.at(-1) ?? 0;
    lines.push(
      (sig >= 0.5 && sig <= 1.5 ? "PASS" : "FAIL") + ": sigma_near_critical " + sig.toFixed(3),
    );
    setChecklist(lines);
  };

  const bitPreview = useMemo(() => series.bits.current.slice(-40).join(" "), [tick]);

  return (
    <div className="app">
      <h1 className="brand">
        dao-sim
        <span className="badge">非实时 · 按 tick 推进</span>
      </h1>
      <p className="sub">
        仿脑—星系脉冲动力监测台 · 引擎 ws://127.0.0.1:3848 · 状态：{status} · tick {tick}
      </p>

      <div className="toolbar">
        <button className="primary" onClick={() => send({ cmd: "start" })}>
          启动
        </button>
        <button onClick={() => send({ cmd: "pause" })}>暂停</button>
        <label className="muted">
          快进
          <input
            type="number"
            value={stepN}
            onChange={(e) => setStepN(Number(e.target.value))}
            style={{ width: 100, marginLeft: 6 }}
          />
        </label>
        <button onClick={() => send({ cmd: "step", n: stepN })}>快进 N tick</button>
        <label className="muted">
          种子
          <input
            type="number"
            value={seed}
            onChange={(e) => setSeed(Number(e.target.value))}
            style={{ width: 80, marginLeft: 6 }}
          />
        </label>
        <button onClick={() => send({ cmd: "seed", value: seed })}>重播种</button>
      </div>

      <div className="toolbar">
        <label>
          <input
            type="checkbox"
            checked={daoAdapt}
            onChange={(e) => {
              setDaoAdapt(e.target.checked);
              send({ cmd: "ablation", name: "dao_adaptation", on: e.target.checked });
            }}
          />{" "}
          道核适应电流
        </label>
        <label>
          <input
            type="checkbox"
            checked={noise}
            onChange={(e) => {
              setNoise(e.target.checked);
              send({ cmd: "ablation", name: "noise", on: e.target.checked });
            }}
          />{" "}
          内在噪声
        </label>
        <button onClick={() => send({ cmd: "perturb", group: "east", pulses: 50 })}>
          扰动东群 50 脉冲
        </button>
        <button onClick={runAblationChecks}>刷新验收行</button>
      </div>

      <div className="grid two">
        <div className="panel">
          <h2>道核 D(t)</h2>
          <Plot title="" xs={xs} ys={dYs} labels={["D"]} />
        </div>
        <div className="panel">
          <h2>分支比 σ</h2>
          <Plot title="" xs={xs} ys={sYs} labels={["σ"]} />
        </div>
      </div>

      <div className="panel" style={{ marginTop: "1rem" }}>
        <h2>六方位放电率（东/西/南/北/上/下）</h2>
        <Plot title="" xs={xs} ys={rYs} labels={CARDINALS} />
      </div>

      <div className="grid two" style={{ marginTop: "1rem" }}>
        <div className="panel">
          <h2>输出群栅格（12 bit）</h2>
          <canvas className="raster" ref={rasterRef} width={800} height={120} />
          <div className="bits">当前 {lastBits}</div>
          <div className="bits muted">{bitPreview}</div>
        </div>
        <div className="panel">
          <h2>验收（PASS/FAIL）</h2>
          <ul className="checklist">
            {checklist.length === 0 && <li className="muted">点击「刷新验收行」采集基线/对比</li>}
            {checklist.map((line) => (
              <li key={line} className={line.startsWith("PASS") ? "pass" : "fail"}>
                {line}
              </li>
            ))}
          </ul>
          <p className="muted">
            消融 A：关道核适应 · B：关噪声 · C：东群脉冲。输出仅为 0/1，无语义解释。
          </p>
        </div>
      </div>
    </div>
  );
}
