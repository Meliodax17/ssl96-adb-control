/**
 * SSL96 ADB LED Diagnostic & Control — Backend Server
 *
 * Giao tiep voi board STM32 qua USB HID (VID 0x0483, PID 0x5750)
 * Chi su dung cac lenh co san trong firmware (0xD1 - 0xD6)
 *
 * Cac lenh co san:
 *   0xD1 REQUEST    - lay bao cao chan doan hien tai
 *   0xD2 RERUN      - chay lai chan doan roi gui
 *   0xD3 BCAST      - bat sang tat ca kenh 50% bang broadcast
 *   0xD4 RESTORE    - tra ve bang do sang mac dinh
 *   0xD5 ONLY_IC    - chi bat sang 1 IC (byte[1]=0..15, byte[2..3]=level)
 *   0xD6 ALL_LEVEL  - dat tat ca kenh ve cung 1 muc (byte[1..2]=WIDTH LE)
 *   0xD8 IC_LEVEL   - dat 16 kenh cua 1 IC (byte[1]=IC, byte[2..3]=WIDTH LE)
 */

const express = require('express');
const http = require('http');
const { WebSocketServer } = require('ws');
const path = require('path');
let HID;
try { HID = require('node-hid'); } catch (e) { HID = null; }

const PORT = 3096;
const VID = 0x0483;
const PID = 0x5750;
const REPORT_SIZE = 64;
const DIAG_TAG = 0xD1;

const CMD = {
    REQUEST:   0xD1,
    RERUN:     0xD2,
    BCAST:     0xD3,
    RESTORE:   0xD4,
    ONLY_IC:   0xD5,
    ALL_LEVEL: 0xD6
};

/* ========================================================================= */
/*  Pixel Map — exact copy of ADB_PIXEL_LABEL from adb_dimming.c            */
const PIXEL_LABELS = [
    // IC301 (addr 0) — pixel 0..15
    {s:'C',l:21},{s:'D',l:21},{s:'C',l:20},{s:'D',l:20},{s:'C',l:19},{s:'D',l:19},{s:'C',l:18},{s:'D',l:18},
    {s:'C',l:17},{s:'D',l:17},{s:'C',l:16},{s:'D',l:16},{s:'C',l:15},{s:'D',l:15},{s:'C',l:14},{s:'D',l:14},
    // IC302 (addr 1) — pixel 16..31
    {s:'A',l:22},{s:'B',l:22},{s:'A',l:23},{s:'B',l:23},{s:'A',l:24},{s:'B',l:24},{s:'A',l:25},{s:'B',l:25},
    {s:'A',l:26},{s:'B',l:26},{s:'C',l:25},{s:'C',l:24},{s:'C',l:23},{s:'D',l:23},{s:'C',l:22},{s:'D',l:22},
    // IC303 (addr 2) — pixel 32..47
    {s:'B',l:14},{s:'A',l:14},{s:'B',l:15},{s:'A',l:15},{s:'B',l:16},{s:'A',l:16},{s:'B',l:17},{s:'A',l:17},
    {s:'B',l:18},{s:'A',l:18},{s:'B',l:19},{s:'A',l:19},{s:'B',l:20},{s:'A',l:20},{s:'B',l:21},{s:'A',l:21},
    // IC304 (addr 3) — pixel 48..63
    {s:'B',l:6},{s:'A',l:6},{s:'B',l:7},{s:'A',l:7},{s:'B',l:8},{s:'A',l:8},{s:'B',l:9},{s:'A',l:9},
    {s:'B',l:10},{s:'A',l:10},{s:'B',l:11},{s:'A',l:11},{s:'B',l:12},{s:'A',l:12},{s:'B',l:13},{s:'A',l:13},
    // IC305 (addr 4) — pixel 64..79
    {s:'C',l:5},{s:'D',l:5},{s:'C',l:4},{s:'D',l:4},{s:'C',l:3},{s:'C',l:2},{s:'B',l:1},{s:'A',l:1},
    {s:'B',l:2},{s:'A',l:2},{s:'B',l:3},{s:'A',l:3},{s:'B',l:4},{s:'A',l:4},{s:'B',l:5},{s:'A',l:5},
    // IC306 (addr 5) — pixel 80..95
    {s:'C',l:13},{s:'D',l:13},{s:'C',l:12},{s:'D',l:12},{s:'C',l:11},{s:'D',l:11},{s:'C',l:10},{s:'D',l:10},
    {s:'C',l:9},{s:'D',l:9},{s:'C',l:8},{s:'D',l:8},{s:'C',l:7},{s:'D',l:7},{s:'C',l:6},{s:'D',l:6}
];

const WIDTH_DEFAULTS = [
    379,123,512,205,573,205,614,164,368,164,256,82,184,82,205,82,
    276,276,153,82,256,256,153,82,246,246,174,286,297,143,307,307,
    675,675,900,900,1023,1023,900,900,655,655,450,450,338,338,276,276,
    266,266,286,286,297,297,297,307,317,317,338,338,368,368,409,471,
    225,225,225,235,123,235,235,123,256,256,143,41,256,256,143,41,
    153,41,164,61,164,61,164,61,174,61,184,72,205,82,266,123
];

/* ========================================================================= */
/*  USB HID                                                                  */
/* ========================================================================= */
let device = null;
let readChunks = [];
let pendingResolve = null;
let pendingTimer = null;

function listHidDevices() {
    if (!HID) return [];
    try { return HID.devices().filter(d => d.vendorId === VID && d.productId === PID); }
    catch { return []; }
}

function openDevice() {
    if (device) return true;
    const devs = listHidDevices();
    if (devs.length === 0) return false;
    try {
        device = new HID.HID(devs[0].path);
        device.on('data', onHidData);
        device.on('error', (err) => {
            console.error('[USB] Error:', err.message);
            device = null;
            wsBroadcast({ type: 'usb', connected: false });
        });
        console.log('[USB] Connected');
        wsBroadcast({ type: 'usb', connected: true });
        return true;
    } catch (e) {
        console.error('[USB] Open failed:', e.message);
        device = null;
        return false;
    }
}

function closeDevice() {
    if (device) { try { device.close(); } catch {} device = null; }
}

function sendReport(cmd, payload = []) {
    if (!device && !openDevice()) throw new Error('Board không kết nối');
    const buf = Buffer.alloc(REPORT_SIZE + 1, 0);
    buf[0] = 0x00;   // Report ID
    buf[1] = cmd;
    for (let i = 0; i < payload.length && i < 62; i++) buf[2 + i] = payload[i];
    try { device.write([...buf]); }
    catch (e) { device = null; throw new Error('Gửi thất bại: ' + e.message); }
}

function onHidData(data) {
    const off = data.length > REPORT_SIZE ? 1 : 0;
    if (data[off] !== DIAG_TAG) return;

    const idx   = data[off + 1];
    const total = data[off + 2];
    const cnt   = data[off + 3];
    const txt   = data.slice(off + 4, off + 4 + cnt).toString('ascii');
    readChunks.push({ idx, total, txt });

    if (idx >= total - 1) {
        const full = readChunks.map(c => c.txt).join('');
        readChunks = [];
        if (pendingResolve) {
            clearTimeout(pendingTimer);
            pendingResolve(full);
            pendingResolve = null;
        }
        wsBroadcast({ type: 'report', text: full });
    }
}

function sendAndWait(cmd, payload = [], timeout = 10000) {
    return new Promise((resolve, reject) => {
        readChunks = [];
        pendingResolve = resolve;
        pendingTimer = setTimeout(() => { pendingResolve = null; reject(new Error('Hết thời gian chờ')); }, timeout);
        sendReport(cmd, payload);
    });
}

/* ========================================================================= */
/*  WebSocket                                                                */
/* ========================================================================= */
const wsClients = new Set();
function wsBroadcast(obj) {
    const msg = JSON.stringify(obj);
    for (const c of wsClients) { if (c.readyState === 1) c.send(msg); }
}

/* ========================================================================= */
/*  Express                                                                  */
/* ========================================================================= */
const app = express();
app.use(express.json());
app.use(express.static(path.join(__dirname, 'public')));

// --- Info ---
app.get('/api/info', (_req, res) => {
    res.json({
        pixelCount: 96, devCount: 6, chPerDev: 16,
        widthMax: 1023, widthLimit: 205,

        /*  Rang buoc dien ap, dung de giao dien tu tinh va canh bao.
         *  Board nguon 32LT3365 boost ra 40V roi buck lai, nen dien ap chuoi
         *  LED luon duoi 40V. Voi LED ~3V thi khoang 13 LED sang dong thoi.
         *  ANODE#1 noi tiep 4 IC = 64 kenh; ANODE#2 va #3 moi duong 1 IC. */
        supplyVolt: 40, ledVf: 3,
        anodeGroups: [
            { name: 'ANODE#1', ics: [0, 1, 4, 5] },
            { name: 'ANODE#2', ics: [2] },
            { name: 'ANODE#3', ics: [3] }
        ],
        labels: PIXEL_LABELS,
        defaults: WIDTH_DEFAULTS,
        icNames: ['IC301','IC302','IC303','IC304','IC305','IC306'],
        icTypes: ['TPS92664 Master','TPS92667 Slave','TPS92667 Slave',
                  'TPS92667 Slave','TPS92667 Slave','TPS92667 Slave']
    });
});

// --- USB status ---
app.get('/api/status', (_req, res) => {
    res.json({ connected: !!device, found: listHidDevices().length > 0, hid: !!HID });
});

app.post('/api/connect', (_req, res) => { res.json({ connected: openDevice() }); });
app.post('/api/disconnect', (_req, res) => { closeDevice(); res.json({ connected: false }); });

// --- Diagnostic commands ---
async function diagHandler(cmd, payload, timeout) {
    return async (_req, res) => {
        try {
            const p = typeof payload === 'function' ? payload(_req) : (payload || []);
            const t = typeof timeout === 'function' ? timeout(_req) : (timeout || 10000);
            const text = await sendAndWait(cmd, p, t);
            res.json({ ok: true, text });
        } catch (e) {
            res.status(500).json({ ok: false, error: e.message });
        }
    };
}

app.post('/api/diag/request',   async (req, res) => (await diagHandler(CMD.REQUEST))(req, res));
app.post('/api/diag/rerun',     async (req, res) => (await diagHandler(CMD.RERUN, [], 15000))(req, res));
app.post('/api/diag/broadcast', async (req, res) => (await diagHandler(CMD.BCAST))(req, res));
app.post('/api/diag/restore',   async (req, res) => (await diagHandler(CMD.RESTORE))(req, res));

app.post('/api/diag/only-ic', async (req, res) => {
    try {
        const ic = parseInt(req.body.ic) || 0;
        const level = parseInt(req.body.level) || 512;

        /*  Firmware doc: target = report[1], level = report[2] | report[3]<<8
         *  Ma buf[2+i] = payload[i] tro thanh report[1+i], nen payload phai la
         *  [ic, level_lo, level_hi]. Ban truoc chen thua mot so 0 o giua, lam
         *  level lech mot byte va luon ra 0, khien firmware roi ve mac dinh
         *  50% bat ke thanh truot dat o dau. */
        const p = [ic & 0xFF, level & 0xFF, (level >> 8) & 0xFF];
        const text = await sendAndWait(CMD.ONLY_IC, p);
        res.json({ ok: true, text });
    } catch (e) { res.status(500).json({ ok: false, error: e.message }); }
});

app.post('/api/diag/only-string', async (req, res) => {
    try {
        let ic = parseInt(req.body.ic);
        let ch = parseInt(req.body.channel);
        let lvl = parseInt(req.body.level);
        if (isNaN(ic) || ic < 0 || ic > 5) ic = 0;
        if (isNaN(ch) || ch < 0 || ch > 15) ch = 0;
        if (isNaN(lvl) || lvl < 0 || lvl > 1023) lvl = 511;

        const text = await sendAndWait(0xD7, [ic, ch, lvl & 0xFF, (lvl >> 8) & 0xFF], 5000);
        res.json({ok: true, text});
    } catch (e) {
        res.status(500).json({ok: false, error: e.message});
    }
});

app.post('/api/diag/ic-level', async (req, res) => {
    try {
        let ic  = parseInt(req.body.ic);
        let lvl = parseInt(req.body.level);
        if (isNaN(ic)  || ic  < 0 || ic  > 5)    ic  = 0;
        if (isNaN(lvl) || lvl < 0 || lvl > 1023) lvl = 0;

        /*  0xD8 IC_LEVEL: dat ca 16 kenh cua mot IC, khong dung toi cac IC
         *  khac. Khac voi 0xD5 ONLY_IC la lenh loai tru (tat het con lai). */
        const text = await sendAndWait(0xD8, [ic, lvl & 0xFF, (lvl >> 8) & 0xFF], 5000);
        res.json({ ok: true, text });
    } catch (e) { res.status(500).json({ ok: false, error: e.message }); }
});

app.post('/api/diag/all-level', async (req, res) => {
    try {
        const w = Math.min(Math.max(0, parseInt(req.body.width) || 0), 1023);
        const text = await sendAndWait(CMD.ALL_LEVEL, [w & 0xFF, (w >> 8) & 0xFF]);
        res.json({ ok: true, text });
    } catch (e) { res.status(500).json({ ok: false, error: e.message }); }
});

/* ========================================================================= */
/*  Start                                                                    */
/* ========================================================================= */
const server = http.createServer(app);
const wss = new WebSocketServer({ server });
wss.on('connection', (ws) => {
    wsClients.add(ws);
    ws.on('close', () => wsClients.delete(ws));
    ws.send(JSON.stringify({ type: 'usb', connected: !!device, found: listHidDevices().length > 0, hid: !!HID }));
});

/*  Cong da bi chiem thi KHONG duoc de van de nay lam sap ung dung.
 *
 *  Truoc day loi EADDRINUSE thoat ra ngoai thanh ngoai le khong bat, va
 *  Electron do thang no ra hop thoai "A JavaScript error occurred in the
 *  main process" - nguoi dung khong hieu chuyen gi va khong mo duoc app.
 *
 *  Gan nhu luon la mot ban sao khac cua chinh ung dung nay dang chay. Khi
 *  do cu de nguyen ban sao do phuc vu; cua so se tai giao dien tu no nhu
 *  binh thuong.                                                            */
server.on('error', (err) => {
    if (err.code === 'EADDRINUSE') {
        console.log(`\n  [!] Cong ${PORT} dang duoc mot tien trinh khac su dung.`);
        console.log('      Dung lai may chu cua tien trinh do, khong mo them.\n');
        module.exports.portBusy = true;
        return;
    }
    console.error('  [!] Loi may chu:', err.message);
});

server.listen(PORT, () => {
    console.log(`\n  SSL96 ADB Control  →  http://localhost:${PORT}\n`);
    if (!HID) { console.log('  [!] node-hid not loaded. Run: npm install node-hid\n'); return; }
    if (openDevice()) console.log('  [USB] Auto-connected to board\n');
    else              console.log('  [USB] Board not found — connect via UI\n');
});

module.exports.portBusy = false;
