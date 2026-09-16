let info = null;
let ws = null;
let cFilt = 'all';
let cGrp = 'led';

const $ = id => document.getElementById(id);
const S_COLORS = { A: '#06b6d4', B: '#8b5cf6', C: '#10b981', D: '#f59e0b' };

document.addEventListener('DOMContentLoaded', async () => {
    try {
        const res = await fetch('/api/info');
        info = await res.json();
    } catch (e) {
        toast('Không thể tải thông tin map', 'err');
    }
    
    initGrid();
    initMap();
    initWs();
    checkConn();
});

// --- UI ---
function tab(id) {
    document.querySelectorAll('.nav-tab').forEach(e => e.classList.toggle('active', e.dataset.t === id));
    document.querySelectorAll('.panel').forEach(e => e.classList.toggle('on', e.id === `p-${id}`));
}

function toast(msg, type='ok') {
    const t = document.createElement('div');
    t.className = `toast ${type}`;
    t.textContent = msg;
    $('toasts').appendChild(t);
    setTimeout(() => t.remove(), 4000);
}

function load(state) { $('loader').style.display = state ? 'flex' : 'none'; }

// --- WebSocket & Status ---
function initWs() {
    const p = location.protocol === 'https:' ? 'wss:' : 'ws:';
    ws = new WebSocket(`${p}//${location.host}`);
    ws.onclose = () => setTimeout(initWs, 3000);
    ws.onmessage = e => {
        try {
            const d = JSON.parse(e.data);
            if (d.type === 'usb') updateUsb(d.connected);
            if (d.type === 'report') parseReport(d.text);
        } catch {}
    };
}

async function checkConn() {
    try {
        const res = await fetch('/api/status');
        const data = await res.json();
        updateUsb(data.connected);
    } catch {}
}

function updateUsb(conn) {
    const b = $('usb-badge');
    b.className = `usb-badge ${conn ? 'on' : 'off'}`;
    $('usb-text').textContent = conn ? 'Connected' : 'Disconnected';
    $('btn-conn').textContent = conn ? 'Disconnect' : 'Connect';
}

async function toggleConn() {
    const isConn = $('usb-badge').classList.contains('on');
    try {
        const res = await fetch(isConn ? '/api/disconnect' : '/api/connect', { method: 'POST' });
        const d = await res.json();
        updateUsb(d.connected);
        toast(d.connected ? 'Đã kết nối' : 'Đã ngắt kết nối', d.connected ? 'ok' : 'err');
    } catch (e) { toast('Lỗi kết nối', 'err'); }
}

// --- Dashboard ---
function initGrid() {
    const g = $('ic-grid');
    g.innerHTML = '';
    for (let i = 0; i < 6; i++) {
        g.innerHTML += `
            <div class="ic-card off" id="ic-${i}">
                <div class="ic-hdr">
                    <span class="ic-name">${info ? info.icNames[i] : 'IC'+i}</span>
                    <span class="ic-type">${i===0 ? 'Master' : 'Slave'}</span>
                </div>
                <div class="ic-hdr" style="margin-bottom:12px">
                    <span style="font-size:0.75rem;color:var(--text-dim)">Addr: ${i}</span>
                    <span class="ic-st off" id="st-${i}">Offline</span>
                </div>
                <div class="ic-row"><span>ICID</span><span class="ic-val" id="v-icid-${i}">—</span></div>
                <div class="ic-row"><span>Status</span><span class="ic-val" id="v-st-${i}">—</span></div>
                <div class="ic-row"><span>Fault Open</span><span class="ic-val" id="v-fo-${i}">—</span></div>
                <div class="ic-row"><span>Fault Short</span><span class="ic-val" id="v-fs-${i}">—</span></div>
                <div class="ic-row"><span>CRC Err</span><span class="ic-val" id="v-crc-${i}">—</span></div>
                ${i===0 ? `<div class="ic-row"><span>Temp</span><span class="ic-val" id="v-tmp">—</span></div>` : ''}
            </div>
        `;
    }
}

// --- Map ---
function initMap() {
    const g = $('led-grid');
    g.innerHTML = '';
    if (!info || !info.labels) return;
    
    if (cGrp === 'led') {
        for (let l = 1; l <= 26; l++) {
            const pxl = [];
            for (let p=0; p<info.labels.length; p++) if(info.labels[p].l === l) pxl.push(p);
            if (pxl.length === 0) continue;
            pxl.sort((a,b) => info.labels[a].s.charCodeAt(0) - info.labels[b].s.charCodeAt(0));
            g.innerHTML += `<div class="led-grp">LED ${l}</div>`;
            pxl.forEach(p => g.appendChild(mkCell(p)));
        }
    } else {
        for (let i = 0; i < 6; i++) {
            g.innerHTML += `<div class="led-grp">${info.icNames[i]} (Addr ${i})</div>`;
            for (let c = 0; c < 16; c++) g.appendChild(mkCell(i*16 + c));
        }
    }
    appFilt();
}

function mkCell(p) {
    const l = info.labels[p];
    const c = S_COLORS[l.s] || '#fff';
    const div = document.createElement('div');
    div.className = 'led-cell';
    div.dataset.s = l.s;
    div.innerHTML = `
        <div class="lbl" style="color:${c}">${l.s}${l.l}</div>
        <div class="inf">P${p}</div>
        <div class="inf" style="margin-top:2px">IC${Math.floor(p/16)+1}:${p%16+1}</div>
    `;
    return div;
}

function filt(s) {
    cFilt = s;
    document.querySelectorAll('.fbtn').forEach(b => b.classList.toggle('on', b.dataset.f === s));
    appFilt();
}
function appFilt() {
    document.querySelectorAll('.led-cell').forEach(c => c.classList.toggle('hide', cFilt !== 'all' && c.dataset.s !== cFilt));
}
function grp(g) {
    cGrp = g;
    document.querySelectorAll('.gbtn').forEach(b => b.classList.toggle('on', b.dataset.g === g));
    initMap();
}

// --- API calls ---
async function api(url, data) {
    load(1);
    try {
        const res = await fetch(url, { method: 'POST', headers: {'Content-Type':'application/json'}, body: data ? JSON.stringify(data) : null });
        const d = await res.json();
        if (!d.ok) throw new Error(d.error);
        return d;
    } catch(e) { toast(e.message, 'err'); }
    finally { load(0); }
}

const cmdRequest = async () => await api('/api/diag/request');
const cmdRerun = async () => await api('/api/diag/rerun');
const cmdBroadcast = async () => await api('/api/diag/broadcast');
const cmdRestore = async () => await api('/api/diag/restore');
const cmdOnlyIc = async () => await api('/api/diag/only-ic', { ic: $('sel-ic').value, level: $('rng-ic').value });

function onAllSlider(val) {
    $('v-all').textContent = val;
    $('warn-all').textContent = info && val > info.widthLimit ? `⚠ WIDTH > ${info.widthLimit}` : '';
}
const cmdAllLevel = async () => await api('/api/diag/all-level', { width: $('rng-all').value });

// --- Terminal & Parsing ---
function parseReport(txt) {
    // Term
    const t = $('tout');
    const time = new Date().toLocaleTimeString();
    
    let h = txt
        .replace(/^(=+)$/gm, '\n$1')
        .replace(/(OK\s+)/g, '<span class="t-ok">$1</span>')
        .replace(/(IM LANG|KHONG ACK|SAI CRC|MAT ECHO|TX TIMEOUT|that bai|KHONG|ERR)/gi, '<span class="t-err">$1</span>')
        .replace(/(CANH BAO|VUOT ngan sach)/gi, '<span class="t-warn">$1</span>')
        .replace(/(BAO CAO|TONG KET|KIEM TRA|QUET)/gi, '<span class="t-hdr">$1</span>')
        .replace(/(0x[0-9A-Fa-f]+)/g, '<span class="t-val">$1</span>');
        
    t.innerHTML = `<span class="tm">── ${time} ──</span>\n${h}\n\n` + t.innerHTML;
    tab('term');
    toast('Đã nhận báo cáo');

    // Parse UI
    let onN = 0;
    for (let i = 0; i < 6; i++) {
        const rx = new RegExp(`IC30${i+1}[\\s\\S]*?(?=IC30${i+2}|-{4,}|QUET|$)`, 'i');
        const m = txt.match(rx);
        const card = $(`ic-${i}`);
        
        let isOn = false, icid = '—', st = '—', fo = '—', fs = '—', crc = '—';
        
        if (m) {
            const s = m[0];
            const im = s.match(/ICID\s*:\s*OK\s*=(0x[0-9A-Fa-f]{2})/);
            if (im) { isOn = true; icid = im[1]; }
            
            const sm = s.match(/STATUS=(0x[0-9A-Fa-f]{2})/);
            if (sm) st = sm[1];
            
            if (s.match(/GHI\s+WIDTH01:\s*OK/)) isOn = true;
        }
        
        if (isOn) onN++;
        
        card.className = `ic-card ${isOn?'on':'off'}`;
        $(`st-${i}`).className = `ic-st ${isOn?'on':'off'}`;
        $(`st-${i}`).textContent = isOn ? 'Online' : 'Offline';
        $(`v-icid-${i}`).textContent = icid;
        $(`v-st-${i}`).textContent = st;
        $(`v-icid-${i}`).className = `ic-val ${isOn?'ok':'err'}`;
        $(`v-st-${i}`).className = `ic-val ${st==='0x00'?'ok':st==='—'?'':'err'}`;
    }
    $('online-n').textContent = `${onN}/6 Online`;
}
