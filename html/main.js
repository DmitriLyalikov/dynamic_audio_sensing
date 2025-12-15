const ws = new WebSocket("ws://" + location.hostname + "/");
ws.binaryType = "arraybuffer";

const inputTrace = { y: [], mode: "lines", name: "Input" };
const outputTrace = { y: [], mode: "lines", name: "Output" };

Plotly.newPlot("chart", [inputTrace, outputTrace], {
  title: "Dmitri Lyalikov - Dynamic Audio Sensing - Audio Waveforms",
  xaxis: { title: "Sample" },
  yaxis: { title: "Amplitude" }
});

ws.onmessage = (evt) => {
  if (!(evt.data instanceof ArrayBuffer)) {
    console.warn("Non-binary frame ignored");
    return;
  }

  const buf = evt.data;
  const dv = new DataView(buf);
  let off = 0;

  const magic =
    String.fromCharCode(dv.getUint8(0)) +
    String.fromCharCode(dv.getUint8(1)) +
    String.fromCharCode(dv.getUint8(2)) +
    String.fromCharCode(dv.getUint8(3));
  off += 4;

  if (magic !== "AUD0") {
    console.warn("Invalid frame magic:", magic);
    return;
  }

  const N = dv.getUint32(off, true); off += 4;
  const rms = dv.getFloat32(off, true); off += 4;
  const centroid = dv.getFloat32(off, true); off += 4;
  const gain = dv.getFloat32(off, true); off += 4;

  console.log(`Binary AUDIO: N=${N}, RMS=${rms.toFixed(3)}, C=${centroid.toFixed(1)}, Gain=${gain}`);

  const input = new Int16Array(N);
  const output = new Int16Array(N);

  for (let i = 0; i < N; i++) {
    input[i] = dv.getInt16(off, true);
    off += 2;
  }
  for (let i = 0; i < N; i++) {
    output[i] = dv.getInt16(off, true);
    off += 2;
  }

  Plotly.update("chart", {
    y: [Array.from(input), Array.from(output)]
  });
};

ws.onopen = () => console.log("WebSocket connected");
ws.onerror = e => console.error("WS error", e);
