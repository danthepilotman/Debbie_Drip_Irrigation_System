function pad(n) {
  return String(n).padStart(2, "0");
}

// Machine format: 2026-05-09T02:08:27-0400
function getCreatedAt() {
  const now = new Date();

  const yyyy = now.getFullYear();
  const mm = pad(now.getMonth() + 1);
  const dd = pad(now.getDate());
  const hh = pad(now.getHours());
  const min = pad(now.getMinutes());
  const ss = pad(now.getSeconds());

  const offset = getOffset();

  return `${yyyy}-${mm}-${dd}T${hh}:${min}:${ss}${offset}`;
}

// Human-readable format: Sat May 09, 2026 02:08:27 AM
function getStatusTime() {
  return new Date().toLocaleString("en-US", {
    weekday: "short",
    month: "short",
    day: "2-digit",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
    hour12: true
  });
}

// Timezone offset like -0400 / +0100
function getOffset() {
  const o = -new Date().getTimezoneOffset();
  const sign = o >= 0 ? "+" : "-";
  const pad2 = n => String(Math.floor(Math.abs(n))).padStart(2, "0");
  return `${sign}${pad2(o / 60)}${pad2(o % 60)}`;
}



fetch("https://api.thingspeak.com/update", {
  method: "POST",
  headers: {
    "Content-Type": "application/x-www-form-urlencoded"
  },
  body: new URLSearchParams({
    api_key: "WZ6S3B4NWT6PKXD2",
    created_at: getCreatedAt(),
    field1: 26.7,
    field2: 68.2,
    field3: 290,
    field4: 6.1,
    field5: 332,
    field6: 264,
    field7: 159,
    field8: 0,
    status: `Update sent ${getStatusTime()} SW: v1.0.13`
  })
})
.then(r => r.text())
.then(console.log);