export type Telemetry = {
  tick: number;
  D: number;
  sigma: number;
  rates: number[];
  outBits: number;
};

const FRAME_BYTES = 8 + 4 + 4 + 24 + 2; // 42

export function parseTelemetryFrame(buf: ArrayBuffer): Telemetry | null {
  if (buf.byteLength < FRAME_BYTES) return null;
  const view = new DataView(buf);
  let o = 0;
  const tickLo = view.getUint32(o, true);
  const tickHi = view.getUint32(o + 4, true);
  o += 8;
  const tick = tickHi * 2 ** 32 + tickLo;
  const D = view.getFloat32(o, true);
  o += 4;
  const sigma = view.getFloat32(o, true);
  o += 4;
  const rates: number[] = [];
  for (let i = 0; i < 6; i++) {
    rates.push(view.getFloat32(o, true));
    o += 4;
  }
  const outBits = view.getUint16(o, true);
  return { tick, D, sigma, rates, outBits };
}

export function bitsString(bits: number): string {
  return (bits & 0x0fff).toString(2).padStart(12, "0");
}

export type WsClient = {
  send: (obj: unknown) => void;
  close: () => void;
};

export function connectWs(
  url: string,
  onFrame: (t: Telemetry) => void,
  onStatus: (s: string) => void,
): WsClient {
  const ws = new WebSocket(url);
  ws.binaryType = "arraybuffer";
  ws.onopen = () => onStatus("已连接");
  ws.onclose = () => onStatus("已断开");
  ws.onerror = () => onStatus("连接错误");
  ws.onmessage = (ev) => {
    if (ev.data instanceof ArrayBuffer) {
      const t = parseTelemetryFrame(ev.data);
      if (t) onFrame(t);
    }
  };
  return {
    send(obj: unknown) {
      if (ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify(obj));
    },
    close() {
      ws.close();
    },
  };
}
