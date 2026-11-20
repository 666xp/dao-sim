import { useEffect, useMemo, useState } from "react";

type LightItem = {
  status: "PASS" | "FAIL";
  name: string;
  value: number | string;
  threshold: number | string | null;
  phase: string;
  line: string;
};

type LightsPayload = {
  ok: boolean;
  items: LightItem[];
  inject_fail?: boolean;
};

const PHASE_LABEL: Record<string, string> = {
  "0": "阶段 0 · 导通",
  "2": "阶段 2 · 道核/场/σ",
  "3": "阶段 3 · 输出/消融",
  "4": "阶段 4 · 审计",
  "?": "其他",
};

export default function Lights() {
  const [data, setData] = useState<LightsPayload | null>(null);
  const [error, setError] = useState<string>("");
  const [source, setSource] = useState<"lights.json" | "lights.fail.json">("lights.json");

  const load = async (file: "lights.json" | "lights.fail.json") => {
    setError("");
    setSource(file);
    try {
      const res = await fetch("/" + file + "?t=" + Date.now());
      if (!res.ok) throw new Error("HTTP " + res.status);
      const json = (await res.json()) as LightsPayload;
      setData(json);
    } catch (e) {
      setData(null);
      setError(String(e));
    }
  };

  useEffect(() => {
    void load("lights.json");
  }, []);

  const groups = useMemo(() => {
    const g: Record<string, LightItem[]> = {};
    for (const it of data?.items ?? []) {
      (g[it.phase] ??= []).push(it);
    }
    return g;
  }, [data]);

  const failCount = data?.items.filter((i) => i.status === "FAIL").length ?? 0;

  return (
    <div className="app lights">
      <h1 className="brand">
        dao-sim
        <span className="badge">阶段 4 · 红绿灯</span>
      </h1>
      <p className="sub">
        解析 bench 的 <code>PASS/FAIL: name value (threshold t)</code> 行。当前源：{source}
        {data?.inject_fail ? " · inject_fail" : ""}
      </p>

      <div className="toolbar">
        <button className="primary" onClick={() => void load("lights.json")}>
          加载 lights.json
        </button>
        <button onClick={() => void load("lights.fail.json")}>加载失败样例（应变红）</button>
        <a className="muted" href="/">
          ← 监测台
        </a>
      </div>

      {error && <p className="fail">加载失败：{error}。先跑 scripts/collect_lights.py</p>}

      {data && (
        <>
          <p className={failCount === 0 ? "pass" : "fail"}>
            汇总：{failCount === 0 ? "全绿" : `${failCount} 项红`} · items={data.items.length}
          </p>
          {Object.keys(PHASE_LABEL).map((ph) => {
            const items = groups[ph];
            if (!items?.length) return null;
            return (
              <div className="panel" key={ph} style={{ marginTop: "1rem" }}>
                <h2>{PHASE_LABEL[ph]}</h2>
                <ul className="checklist">
                  {items.map((it) => (
                    <li key={it.name} className={it.status === "PASS" ? "pass" : "fail"}>
                      {it.line}
                    </li>
                  ))}
                </ul>
                {ph === "3" && (
                  <p className="muted">
                    非预设三项一组：ones_density_band + ones_stats_similar + sequences_differ（稀疏度下勿单看汉明）。
                  </p>
                )}
              </div>
            );
          })}
        </>
      )}
    </div>
  );
}
