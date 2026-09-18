/* =========================================================================
   SSL96 ADB - Bang dieu khien
   Moi chuoi hien thi deu di qua t() trong i18n.js
   ========================================================================= */

let info = null;
let ws = null;
let cFilt = 'all';
let autoRefreshTimer = null;
let lastReport = null;          // giu ban bao cao cuoi de ve lai khi doi ngon ngu

const $ = id => document.getElementById(id);
const S_COLORS = { A: '#06b6d4', B: '#8b5cf6', C: '#10b981', D: '#f59e0b' };
const IC_NAMES = ['IC301','IC302','IC303','IC304','IC305','IC306'];

const pct = w => Math.round((Number(w) / 1023) * 100) + '%';

document.addEventListener('DOMContentLoaded', async () => {
    try {
        info = await (await fetch('/api/info')).json();
    } catch (e) {
        toast(t('tCfgErr'), 'err');
    }
    initTheme();
    initIcCards();
    initOnlyIcButtons();
    initPerIcRows();
    initModel();
    initMap();
    applyLang();                // dat chu lan dau, cung goi refreshDynamicText()
    initWs();
    checkConn();
});

/*  i18n.js goi ham nay moi khi doi ngon ngu, de ve lai nhung chu do JS sinh
    ra chu khong nam san trong HTML. */
function refreshDynamicText() {
    onAllSlider($('rng-all').value);
    updateMapCount();
    $('v-map').textContent = pct($('rng-map').value);
    $('btn-cycle').textContent = cycleTimer ? t('btnCycleStop') : t('btnCycle');

    const conn = $('usb-badge').classList.contains('on');
    $('usb-text').textContent = conn ? t('connected') : t('disconnected');
    $('btn-conn').textContent = conn ? t('btnDisconnect') : t('btnConnect');

    updateModelTag();
    renderTemp(lastReport);
    relabelPerIcRows();
    relabelIcCards();
    relabelMap();

    if (lastReport) renderReport(lastReport);
    else if (!conn) setVerdict('unknown', '?', t('vNoConn'), t('vNoConnSub'));
    else            setVerdict('unknown', '?', t('vNoData'), t('vNoDataSub'));
}

/* ---------------------------------------------------------------- Giao dien */
function initTheme() {
    document.documentElement.setAttribute('data-theme', localStorage.getItem('theme') || 'dark');
}
function toggleTheme() {
    const next = document.documentElement.getAttribute('data-theme') === 'light' ? 'dark' : 'light';
    document.documentElement.setAttribute('data-theme', next);
    localStorage.setItem('theme', next);
}

function tab(id) {
    document.querySelectorAll('.nav-tab').forEach(e => e.classList.toggle('active', e.dataset.t === id));
    document.querySelectorAll('.panel').forEach(e => e.classList.toggle('on', e.id === `p-${id}`));
}

function toast(msg, type = 'ok') {
    const el = document.createElement('div');
    el.className = `toast ${type}`;
    el.textContent = msg;
    $('toasts').appendChild(el);
    setTimeout(() => el.remove(), 4000);
}
function load(on) { $('loader').style.display = on ? 'flex' : 'none'; }

/* ---------------------------------------------------------------- Ket noi */
function initWs() {
    const p = location.protocol === 'https:' ? 'wss:' : 'ws:';
    ws = new WebSocket(`${p}//${location.host}`);
    ws.onclose = () => setTimeout(initWs, 3000);
    ws.onmessage = e => {
        try {
            const d = JSON.parse(e.data);
            if (d.type === 'usb')    updateUsb(d.connected);
            if (d.type === 'report') parseReport(d.text);
        } catch {}
    };
}

async function checkConn() {
    try { updateUsb((await (await fetch('/api/status')).json()).connected); } catch {}
}

function updateUsb(conn) {
    $('usb-badge').className  = `usb-badge ${conn ? 'on' : 'off'}`;
    $('usb-text').textContent = conn ? t('connected') : t('disconnected');
    $('btn-conn').textContent = conn ? t('btnDisconnect') : t('btnConnect');
    if (!conn && !lastReport) setVerdict('unknown', '?', t('vNoConn'), t('vNoConnSub'));
}

async function toggleConn() {
    const isConn = $('usb-badge').classList.contains('on');
    try {
        const d = await (await fetch(isConn ? '/api/disconnect' : '/api/connect', { method: 'POST' })).json();
        updateUsb(d.connected);
        toast(d.connected ? t('tOk') : t('tOff'), d.connected ? 'ok' : 'err');
    } catch { toast(t('tConnErr'), 'err'); }
}

/* ---------------------------------------------------------------- The IC */
function initIcCards() {
    const g = $('ic-grid');
    g.innerHTML = '';
    for (let i = 0; i < 6; i++) {
        g.innerHTML += `
        <div class="ic-card off" id="ic-${i}">
            <div class="ic-hdr">
                <span class="ic-name">${IC_NAMES[i]}</span>
                <span class="ic-type" id="ic-role-${i}"></span>
            </div>
            <div class="ic-state" id="st-${i}"></div>
            <div class="ic-rows">
                <div class="ic-row"><span id="lb-range-${i}"></span><span class="ic-val" id="v-range-${i}">—</span></div>
                <div class="ic-row"><span id="lb-icid-${i}"></span><span class="ic-val" id="v-icid-${i}">—</span></div>
                <div class="ic-row"><span id="lb-st-${i}"></span><span class="ic-val" id="v-st-${i}">—</span></div>
            </div>
        </div>`;
    }
}

/*  Dat lai nhan tinh cua sau the IC theo ngon ngu hien tai. */
function relabelIcCards() {
    for (let i = 0; i < 6; i++) {
        $(`ic-role-${i}`).textContent = i === 0 ? t('icMaster') : t('icSlave');
        $(`lb-range-${i}`).textContent = t('rowRange');
        $(`lb-icid-${i}`).textContent  = t('rowIcid');
        $(`lb-st-${i}`).textContent    = t('rowStatus');

        if (!lastReport) {
            $(`st-${i}`).textContent = t('icUnknown');
            $(`st-${i}`).className   = 'ic-state';
        }

        if (info && info.labels) {
            const seg  = info.labels.slice(i * 16, i * 16 + 16);
            const strs = [...new Set(seg.map(x => x.s))].sort().join(', ');
            const leds = seg.map(x => x.l);
            $(`v-range-${i}`).textContent =
                t('rangeFmt', strs, Math.min(...leds), Math.max(...leds));
        }
    }
}

function initOnlyIcButtons() {
    const r = $('only-ic-row');
    r.innerHTML = '';
    for (let i = 0; i < 6; i++) {
        const b = document.createElement('button');
        b.className = 'btn';
        b.textContent = IC_NAMES[i];
        b.onclick = () => cmdOnlyIc(i);
        r.appendChild(b);
    }
}

/* ------------------------------------------------------------- Nhiet do

   Ba nguon so lieu, deu nam san trong ban bao cao chan doan:

     1. NTC (R306) qua ADC1 cua IC301 - QUAN TRONG NHAT.
        Cau phan ap VDK1 - R303 - R306 - GND, diem giua qua tu loc C400 vao
        chan 9. R306 la NTC va duoc dat ngay canh chuoi LED, nen no do nhiet
        VUNG DEN chu khong phai nhiet cua con chip.

     2. Cau thu hai R313/R314 qua ADC2, cung kieu.

     3. DIETEMP - nhiet do trong long IC301. Chi TPS92664 moi co.
        Day la nhiet cua ban than con chip, khac han so (1).

   Cong them bit canh bao qua nhiet (STATUS bit 2) cua tung con trong sau.
   -------------------------------------------------------------------- */
/*  Doc nhiet do NTC tu doan bao cao.

    Firmware da quy doi san ra do C, nen o day chi viec doc. Phep quy doi
    dua tren TY SO ADC1/ADC2 chu khong phai gia tri tuyet doi, vi R313=R314
    khien ADC2 luon bang dung VDK1/2 - nho vay dien ap VDK1 va dien ap tham
    chieu cua ADC deu bi khu khoi phep tinh.                                */
function renderNtc(body) {
    const dial = $('ntc-dial');
    const num  = $('ntc-num');
    const m = body && body.match(/NTC R306[^:]*:\s*(-?\d+\.\d+)\s*do C\s*\(R_ntc\s*=\s*(\d+)/i);

    if (!m) {
        num.textContent = '—';
        dial.className  = 'temp-dial';
        $('ntc-raw').textContent =
            (body && /NTC R306/.test(body)) ? t('ntcBadRead') : '—';
        return;
    }

    const tC = parseFloat(m[1]);
    num.textContent = tC.toFixed(1);

    const adc = body.match(/ADC1=0x([0-9A-Fa-f]{2})\s+ADC2=0x([0-9A-Fa-f]{2})/i);
    $('ntc-raw').textContent = t('ntcDetail', m[2],
        adc ? '0x' + adc[1].toUpperCase() + ' / 0x' + adc[2].toUpperCase() : '—');

    /*  Nguong cho vung den. NTC chiu toi 150 do C, nen 85 / 110 con du cho
        thoi gian phan ung truoc khi cham gioi han linh kien.               */
    dial.className = 'temp-dial ' + (tC >= 110 ? 'bad' : tC >= 85 ? 'warn' : 'ok');
}

function renderTemp(txt) {
    const dial = $('temp-dial');
    const num  = $('temp-num');

    /*  Tach rieng doan NHIET DO roi moi doc, de cac dong "IC30x" o nhung
        phan khac cua ban bao cao khong bi nham vao day.                  */
    const seg = txt ? txt.match(/NHIET DO([\s\S]*?)(?:={10,}|$)/) : null;

    if (!seg) {
        num.textContent   = '—';
        dial.className    = 'temp-dial';
        $('temp-label').textContent = t('tempNoData');
        $('temp-raw').textContent   = '—';
        $('temp-warn-row').innerHTML = '';
        renderNtc(null);
        return;
    }
    const body = seg[1];

    /* --- NTC qua ADC1 --- */
    renderNtc(body);

    /* --- nhiet do trong long IC301 --- */
    const m = body.match(/DIETEMP\)\s*:\s*raw=0x([0-9A-Fa-f]{2})\s*=\s*(-?\d+)\s*do C/i);
    if (m) {
        const tC = parseInt(m[2], 10);
        num.textContent = tC;
        /*  Nguong theo SLUSE18: canh bao qua nhiet cua chip o vung 125 do C.
            Lay 85 / 110 lam moc vang / do de con thoi gian phan ung.      */
        dial.className = 'temp-dial ' + (tC >= 110 ? 'bad' : tC >= 85 ? 'warn' : 'ok');
        $('temp-label').textContent = t('tempDie');
        $('temp-raw').textContent   = t('tempRaw', '0x' + m[1].toUpperCase());
    } else {
        num.textContent = '—';
        dial.className  = 'temp-dial';
        $('temp-label').textContent = t('tempUnread');
        $('temp-raw').textContent   = '—';
    }

    /* --- bit canh bao qua nhiet tung con --- */
    const row = $('temp-warn-row');
    row.innerHTML = '';
    for (let i = 0; i < 6; i++) {
        const line = body.match(new RegExp('IC30' + (i + 1) + '\\s+(QUA NHIET|binh thuong|khong tra loi)', 'i'));
        let cls = 'none', label = t('tempNoAnswer');
        if (line) {
            if (/QUA NHIET/i.test(line[1]))      { cls = 'bad';  label = t('tempHot'); }
            else if (/binh thuong/i.test(line[1])) { cls = 'ok'; label = t('tempOk'); }
        }
        const chip = document.createElement('span');
        chip.className = 'temp-chip ' + cls;
        chip.innerHTML = '<span class="dot"></span>' + IC_NAMES[i] + ' · ' + label;
        row.appendChild(chip);
    }
}

/* ---------------------------------------------------------- Chon model
   Ba model module: 96, 64 va 88 pixel. Hien chi 96 pixel co firmware va
   bang anh xa den; hai model kia moi la cho san. Chon vao chung thi bao
   chua ho tro va khoa cac nut dieu khien lai, de khong ai vo tinh gui
   lenh sai bang anh xa cho phan cung khac.
   -------------------------------------------------------------------- */
/*  Thu tu hien thi. Khong dung Object.keys vi cac khoa la chuoi dang so,
    JavaScript se tu sap xep tang dan va day 96 - model duy nhat dung duoc -
    xuong cuoi.                                                            */
const MODEL_ORDER = ['96', '64', '88'];

const MODELS = {
    '96': { px: 96, ready: true  },
    '64': { px: 64, ready: false },
    '88': { px: 88, ready: false }
};
let MODEL = localStorage.getItem('model') || '96';
if (!(MODEL in MODELS)) MODEL = '96';

const MODEL_READY = Object.fromEntries(
    Object.entries(MODELS).map(([k, v]) => [k, v.ready]));

function initModel() { applyModel(); }

function selectModel(m) {
    MODEL = m;
    localStorage.setItem('model', m);
    applyModel();
    closeModelPicker();
    if (!MODELS[m].ready) toast(t('tModelSoon', m), 'err');
    else                  toast(t('tModelSet', m));
}

function updateModelTag() {
    const tag = $('model-tag');
    if (!tag) return;
    const ready = MODELS[MODEL].ready;
    tag.textContent = ready ? t('modelReady') : t('modelSoon');
    tag.className   = 'model-tag' + (ready ? '' : ' soon');
    $('model-btn-name').textContent = t('modelName', MODEL);
    $('model-chip').className = 'm3d-chip' + (ready ? '' : ' soon');
}

/*  Model chua ho tro thi mo cac phan dieu khien va chan bam, nhung van cho
    xem giao dien de biet sau nay se co gi.                              */
function applyModel() {
    updateModelTag();
    const ready = MODELS[MODEL].ready;
    document.querySelectorAll('#p-dash, #p-map').forEach(p => {
        p.classList.toggle('model-locked', !ready);
    });
}

/* ------------------------------------------------ Hinh 3D cua tam den

   Tra ve danh sach {row, col} cua tung diem anh.

   Voi model 96 pixel thi lay tu bang anh xa THAT do may chu gui sang: bon
   chuoi A/B/C/D, moi chuoi mot hang, so thu tu den la cot. Hinh vi vay ra
   dung dang thuc te - hai hang tren ngan hon nen tam den thot lai phia
   tren, giong mot khoi den pha.

   Hai model kia CHUA co bang anh xa. Khong bia ra so lieu that; chi ve mot
   luoi bon hang deu nhau cho du so diem anh, va danh dau la so do tam de
   nguoi dung biet day chua phai bo tri that.
   -------------------------------------------------------------------- */
const ROW_ORDER = ['D', 'C', 'B', 'A'];        // tu tren xuong duoi

function modelLayout(key) {
    if (key === '96' && info && info.labels) {
        const cols = Math.max(...info.labels.map(x => x.l));
        return {
            real: true,
            rows: ROW_ORDER.length,
            cols,
            pix: info.labels.map(x => ({ r: ROW_ORDER.indexOf(x.s), c: x.l - 1 }))
        };
    }
    /* Luoi tam: 4 hang deu nhau */
    const rows = 4;
    const cols = Math.ceil(MODELS[key].px / rows);
    const pix  = [];
    for (let i = 0; i < MODELS[key].px; i++) {
        pix.push({ r: Math.floor(i / cols), c: i % cols });
    }
    return { real: false, rows, cols, pix };
}

/*  Ve mot tam den vao phan tu cho truoc. */
function renderModel3D(host, key) {
    const L = modelLayout(key);
    const W = 250, H = 92;                       // kich thuoc mat bo mach, px
    const cw = W / L.cols, ch = H / L.rows;
    const d  = Math.max(3, Math.min(cw, ch) * 0.52);

    const board = document.createElement('div');
    board.className = 'm3d-board';
    board.style.width  = W + 'px';
    board.style.height = H + 'px';

    const plate = document.createElement('div');
    plate.className = 'm3d-plate';
    board.appendChild(plate);

    /*  Mau di dan tu vang am o hang duoi len trang lanh o hang tren, cho de
        phan biet bon chuoi khi nhin nghieng. */
    const rowColor = ['#e8f2ff', '#fff0d0', '#ffd48a', '#ffbe63'];

    L.pix.forEach(p => {
        const el = document.createElement('div');
        el.className = 'm3d-pix' + (L.real ? '' : ' dim');
        el.style.width  = d + 'px';
        el.style.height = d + 'px';
        el.style.left   = (p.c * cw + cw / 2 - d / 2) + 'px';
        el.style.top    = (p.r * ch + ch / 2 - d / 2) + 'px';
        if (L.real) el.style.setProperty('--px', rowColor[p.r] || '#ffd48a');
        board.appendChild(el);
    });

    host.innerHTML = '';
    host.appendChild(board);
    attachDrag(host, board);
}

/*  Keo chuot de xoay. Dung con tro thay vi chuot rieng de cham man hinh
    cung dung duoc.                                                       */
function attachDrag(host, board) {
    let rx = 60, rz = -12, px = 0, py = 0, on = false;

    const set = () => {
        board.style.transform =
            `translate(-50%, -50%) rotateX(${rx}deg) rotateZ(${rz}deg)`;
    };

    host.addEventListener('pointerdown', e => {
        on = true; px = e.clientX; py = e.clientY;
        host.classList.add('drag');
        board.style.animation = 'none';
        host.setPointerCapture(e.pointerId);
    });
    host.addEventListener('pointermove', e => {
        if (!on) return;
        rz += (e.clientX - px) * 0.4;
        rx  = Math.max(12, Math.min(84, rx + (e.clientY - py) * 0.3));
        px = e.clientX; py = e.clientY;
        set();
    });
    const stop = e => {
        if (!on) return;
        on = false;
        host.classList.remove('drag');
        try { host.releasePointerCapture(e.pointerId); } catch {}
    };
    host.addEventListener('pointerup', stop);
    host.addEventListener('pointercancel', stop);
}

/* --------------------------------------------------- Hop thoai chon model */
function openModelPicker() {
    const box = $('model-cards');
    box.innerHTML = '';

    MODEL_ORDER.forEach(key => {
        const m = MODELS[key];
        const card = document.createElement('button');
        card.className = 'model-card' + (key === MODEL ? ' on' : '') + (m.ready ? '' : ' soon');
        card.onclick = () => selectModel(key);
        card.innerHTML = `
            <div class="m3d" id="m3d-${key}"></div>
            <div class="model-card-hdr">
                <span class="model-card-name">${t('modelName', key)}</span>
                <span class="model-tag${m.ready ? '' : ' soon'}">${m.ready ? t('modelReady') : t('modelSoon')}</span>
            </div>
            <div class="model-card-note">${m.ready ? t('modelNoteReal') : t('modelNoteStub', m.px)}</div>`;
        box.appendChild(card);
        renderModel3D(card.querySelector('.m3d'), key);
    });

    $('model-modal').classList.add('on');
}

function closeModelPicker() { $('model-modal').classList.remove('on'); }

document.addEventListener('keydown', e => {
    if (e.key === 'Escape') closeModelPicker();
});

/* ---------------------------------------------- Do sang rieng tung IC
   Moi IC mot thanh truot. Khac voi nut "thu rieng mot mach" o tren (lenh
   loai tru, bat mot con va tat nam con), o day moi con giu muc cua rieng
   no nen ca sau con cung sang theo sau muc khac nhau.
   -------------------------------------------------------------------- */
const percLevel = [102, 102, 102, 102, 102, 102];

function initPerIcRows() {
    const box = $('perc-list');
    box.innerHTML = '';
    for (let i = 0; i < 6; i++) {
        const row = document.createElement('div');
        row.className = 'perc-row';
        row.innerHTML = `
            <span class="perc-name">${IC_NAMES[i]}</span>
            <span class="perc-sub" id="perc-sub-${i}"></span>
            <input type="range" class="rng" id="perc-rng-${i}" min="0" max="1023"
                   value="${percLevel[i]}" oninput="onPercSlider(${i}, this.value)">
            <span class="rv" id="perc-val-${i}">${pct(percLevel[i])}</span>
            <button class="btn btn-sm" onclick="cmdIcLevel(${i})" data-perc-send>&#9654;</button>`;
        box.appendChild(row);
    }
    relabelPerIcRows();
    updatePercBudget();
}

/*  Hang phu duoi ten IC cho biet con do thuoc chuoi anode nao - chinh cho
    nay quyet dinh ngan sach dien ap, nen phai nhin thay ngay. */
function relabelPerIcRows() {
    for (let i = 0; i < 6; i++) {
        const el = $(`perc-sub-${i}`);
        if (!el) return;
        el.textContent = anodeNameOf(i);
        const b = document.querySelector(`#perc-list .perc-row:nth-child(${i + 1}) [data-perc-send]`);
        if (b) b.title = t('percSendTip', IC_NAMES[i]);
    }
    updatePercBudget();
}

function anodeNameOf(ic) {
    const g = (info?.anodeGroups || []).find(x => x.ics.includes(ic));
    return g ? g.name : '';
}

function onPercSlider(ic, val) {
    percLevel[ic] = Number(val);
    $(`perc-val-${ic}`).textContent = pct(val);
    updatePercBudget();
}

function percSetAll(w) {
    for (let i = 0; i < 6; i++) {
        percLevel[i] = w;
        $(`perc-rng-${i}`).value = w;
        $(`perc-val-${i}`).textContent = pct(w);
    }
    updatePercBudget();
}

/*  Ngan sach dien ap tinh tren muc THAT cua tung IC, khong phai mot muc
    chung. Chi cong cac IC nam tren chuoi anode dai nhat, vi do la chuoi noi
    tiep duy nhat co the vuot 40V.                                        */
function updatePercBudget() {
    const volt  = info?.supplyVolt ?? 40;
    const vf    = info?.ledVf ?? 3;
    const group = info?.anodeGroups?.[0] ?? { ics: [0, 1, 4, 5] };

    let sim = 0;
    group.ics.forEach(i => { sim += 16 * (percLevel[i] / 1024); });
    const need  = sim * vf;
    const ratio = Math.min(need / volt, 1.6);

    $('perc-budget-num').textContent = `${need.toFixed(0)}V / ${volt}V`;
    $('perc-budget-bar').style.width = (ratio / 1.6 * 100).toFixed(1) + '%';

    const el = $('perc-budget');
    if (need <= volt * 0.75) {
        el.className = 'budget ok';
        $('perc-budget-note').textContent = t('budgetSafe', sim.toFixed(1), group.ics.length * 16);
    } else if (need <= volt) {
        el.className = 'budget warn';
        $('perc-budget-note').textContent = t('budgetNear', sim.toFixed(1), group.ics.length * 16);
    } else {
        el.className = 'budget bad';
        $('perc-budget-note').textContent = t('budgetOver');
    }
}

/*  Gui muc sang cua MOT IC. Lenh 0xD8 khong dung toi cac IC khac. */
async function cmdIcLevel(ic) {
    await api('/api/diag/ic-level', { ic, level: percLevel[ic] });
    toast(t('tIcLevel', IC_NAMES[ic], pct(percLevel[ic])));
}

/*  Gui lan luot ca sau con. Gui tuan tu chu khong song song: duong UART
    chi co mot lan song, hai lenh chong nhau se lam hong khung truyen. */
async function cmdApplyPerIc() {
    for (let i = 0; i < 6; i++) {
        await api('/api/diag/ic-level', { ic: i, level: percLevel[i] });
    }
    toast(t('tPerIcApplied'));
}

/* ------------------------------------------------- Dai trang thai tong the */
function setVerdict(kind, icon, title, sub) {
    $('verdict').className         = `verdict ${kind}`;
    $('verdict-icon').textContent  = icon;
    $('verdict-title').textContent = title;
    $('verdict-sub').textContent   = sub;
}

/* ------------------------------------------------- Thuoc do ngan sach dien ap
   Cac IC tren cung mot duong anode noi TIEP nhau, nen dien ap bo nguon phai
   cap bang tong so den sang TAI CUNG MOT THOI DIEM cua ca chuoi.            */
function budgetFor(width) {
    const volt  = info?.supplyVolt ?? 40;
    const vf    = info?.ledVf ?? 3;
    const group = info?.anodeGroups?.[0] ?? { ics: [0, 1, 4, 5] };
    const chans = group.ics.length * 16;
    const sim   = chans * (Number(width) / 1024);
    return { need: sim * vf, sim, volt, chans };
}

function onAllSlider(val) {
    $('v-all-pct').textContent = pct(val);

    const b = budgetFor(val);
    const ratio = Math.min(b.need / b.volt, 1.6);

    $('budget-num').textContent = `${b.need.toFixed(0)}V / ${b.volt}V`;
    $('budget-bar').style.width = (ratio / 1.6 * 100).toFixed(1) + '%';

    const el = $('budget');
    if (b.need <= b.volt * 0.75) {
        el.className = 'budget ok';
        $('budget-note').textContent = t('budgetSafe', b.sim.toFixed(1), b.chans);
    } else if (b.need <= b.volt) {
        el.className = 'budget warn';
        $('budget-note').textContent = t('budgetNear', b.sim.toFixed(1), b.chans);
    } else {
        el.className = 'budget bad';
        $('budget-note').textContent = t('budgetOver');
    }
}

function setLevel(w) { $('rng-all').value = w; onAllSlider(w); }

/* ---------------------------------------------------------------- Goi lenh */
async function api(url, data) {
    load(1);
    try {
        const res = await fetch(url, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: data ? JSON.stringify(data) : null
        });
        const d = await res.json();
        if (!d.ok) throw new Error(d.error || t('tCmdErr'));
        return d;
    } catch (e) {
        toast(e.message, 'err');
        return null;
    } finally { load(0); }
}

async function cmdRequest() {
    const d = await api('/api/diag/request');
    if (d) parseReport(d.text || '');
    return d;
}
const cmdRestore = async () => { await api('/api/diag/restore'); toast(t('tRestored')); };

/*  Dai trang thai PHAI luon ket thuc.
 *
 *  Ban truoc chi dat "dang kiem tra" roi trong cho tin nhan WebSocket goi
 *  parseReport. Neu lenh loi, het gio, hay tin nhan do khong toi (WebSocket
 *  dang noi lai), dai trang thai ket o "dang kiem tra" mai mai.
 *
 *  Gio dung thang doan van ban do chinh loi goi nay tra ve, va co dong ho
 *  chan de du co gi bat thuong thi van bao duoc cho nguoi dung.            */
let checkingTimer = null;

async function cmdRerun() {
    setVerdict('unknown', '…', t('vChecking'), t('vCheckingSub'));

    clearTimeout(checkingTimer);
    checkingTimer = setTimeout(() => {
        setVerdict('bad', '✕', t('vTimeout'), t('vTimeoutSub'));
    }, 25000);

    const d = await api('/api/diag/rerun');
    clearTimeout(checkingTimer);

    if (!d) {
        /*  api() da hien thong bao loi chi tiet roi, o day chi can dua dai
            trang thai ra khoi trang thai cho.                              */
        setVerdict('bad', '✕', t('vCheckFail'), t('vCheckFailSub'));
        return;
    }

    if (!parseReport(d.text || '')) {
        setVerdict('warn', '!', t('vNoReport'), t('vNoReportSub'));
    }
}

function cmdAllLevel(level) {
    if (level === undefined) level = parseInt($('rng-all').value);
    api('/api/diag/all-level', { width: level });
    toast(level === 0 ? t('tAllOff') : t('tSetLevel', pct(level)));
}

async function cmdOnlyIc(ic) {
    const lvl = parseInt($('rng-all').value) || 102;
    await api('/api/diag/only-ic', { ic, level: lvl });
    toast(t('tOnlyIc', IC_NAMES[ic], pct(lvl)));
}

let cycleTimer = null;
function cmdCycleStrings() {
    const btn = $('btn-cycle');
    if (cycleTimer) {
        clearInterval(cycleTimer);
        cycleTimer = null;
        api('/api/diag/all-level', { width: 0 });
        btn.textContent = t('btnCycle');
        btn.classList.remove('btn-accent');
        toast(t('tCycleStop'));
        return;
    }

    let ic = 0;
    const lvl = parseInt($('rng-all').value) || 102;
    btn.textContent = t('btnCycleStop');
    btn.classList.add('btn-accent');

    /*  Ten truong phai la "ic" cho khop voi may chu. Ban truoc gui "target"
        nen may chu luon nhan undefined va roi ve 0, khien vong lap chi bat
        di bat lai mot minh IC301. */
    const step = () => {
        api('/api/diag/only-ic', { ic: ic % 6, level: lvl });
        toast(t('tCycleOn', IC_NAMES[ic % 6]));
        ic++;
    };
    step();
    cycleTimer = setInterval(step, 2000);
}

function toggleAutoRefresh(on) {
    if (on) { cmdRequest(); autoRefreshTimer = setInterval(cmdRequest, 3000); }
    else    { clearInterval(autoRefreshTimer); autoRefreshTimer = null; }
}

/* ---------------------------------------------------------------- Ban do den */
function initMap() {
    const g = $('led-grid');
    g.innerHTML = '';
    if (!info || !info.labels) return;

    const strings = ['A', 'B', 'C', 'D'];
    const byPos = {};
    strings.forEach(s => byPos[s] = {});
    info.labels.forEach((lb, p) => { byPos[lb.s][lb.l] = p; });

    const positions = [];
    for (let l = 1; l <= 26; l++) {
        if (strings.some(s => byPos[s][l] !== undefined)) positions.push(l);
    }

    const table = document.createElement('table');
    table.className = 'led-table';

    const head = document.createElement('tr');
    head.innerHTML = '<th></th>';
    positions.forEach(l => {
        const th = document.createElement('th');
        th.textContent = l;
        head.appendChild(th);
    });
    table.appendChild(head);

    ['D', 'C', 'B', 'A'].forEach(s => {
        const tr = document.createElement('tr');
        const hd = document.createElement('td');
        hd.className = 'row-hdr';
        hd.dataset.str = s;
        hd.style.color = S_COLORS[s];
        tr.appendChild(hd);

        positions.forEach(l => {
            const td = document.createElement('td');
            const p = byPos[s][l];
            if (p !== undefined) td.appendChild(mkCell(p));
            tr.appendChild(td);
        });
        table.appendChild(tr);
    });

    g.appendChild(table);
    appFilt();
}

/*  Ten hang cua ban do co chua chu "Chuoi/String/스트링" nen phai dat lai
    khi doi ngon ngu. */
function relabelMap() {
    document.querySelectorAll('.row-hdr[data-str]').forEach(td => {
        td.textContent = t('strRow', td.dataset.str);
    });
}

function mkCell(p) {
    const lb = info.labels[p];
    const ic = Math.floor(p / 16), ch = p % 16;
    const div = document.createElement('div');
    div.className = 'led-cell';
    div.dataset.s = lb.s;
    div.dataset.ic = ic;
    div.dataset.ch = ch;
    div.onclick = () => { div.classList.toggle('selected'); updateMapCount(); };
    div.innerHTML = `<div class="lbl" style="color:${S_COLORS[lb.s]}">${lb.s}${lb.l}</div>
                     <div class="inf">${IC_NAMES[ic].slice(2)}·${ch + 1}</div>`;
    return div;
}

function updateMapCount() {
    const n = document.querySelectorAll('.led-cell.selected').length;
    $('map-count').textContent = n === 0 ? t('mapNone') : t('mapSel', n);
}

function filt(s) {
    cFilt = s;
    document.querySelectorAll('.fbtn').forEach(b => b.classList.toggle('on', b.dataset.f === s));
    appFilt();
}
function appFilt() {
    document.querySelectorAll('.led-cell')
        .forEach(c => c.classList.toggle('hide', cFilt !== 'all' && c.dataset.s !== cFilt));
}

function mapSelectAll() {
    document.querySelectorAll('.led-cell:not(.hide)').forEach(c => c.classList.add('selected'));
    updateMapCount();
}
function cmdClearMapSel() {
    document.querySelectorAll('.led-cell.selected').forEach(c => c.classList.remove('selected'));
    updateMapCount();
}

async function cmdApplyMap() {
    const sel = document.querySelectorAll('.led-cell.selected');
    if (!sel.length) return toast(t('tPickLed'), 'err');

    const level = parseInt($('rng-map').value);
    toast(t('tApplying', sel.length));

    await api('/api/diag/all-level', { width: 0 });
    for (const c of sel) {
        await api('/api/diag/only-string', { ic: c.dataset.ic, channel: c.dataset.ch, level });
    }
    toast(t('tApplied', sel.length, pct(level)));
}

/* ---------------------------------------------------------------- Doc bao cao */
function parseReport(txt) {
    if (!txt) return false;

    // --- Nhat ky nguyen van ---
    const out  = $('tout');
    const time = new Date().toLocaleTimeString();
    const html = txt
        .replace(/(OK\s+)/g, '<span class="t-ok">$1</span>')
        .replace(/(IM LANG|KHONG ACK|SAI CRC|MAT ECHO|TX TIMEOUT|that bai|VUOT)/gi, '<span class="t-err">$1</span>')
        .replace(/(0x[0-9A-Fa-f]+)/g, '<span class="t-val">$1</span>');
    out.innerHTML = `<span class="tm">── ${time} ──</span>\n${html}\n\n` + out.innerHTML;

    const logs = out.innerHTML.split('<span class="tm">');
    if (logs.length > 11) out.innerHTML = '<span class="tm">' + logs.slice(1, 11).join('<span class="tm">');

    /*  Chi ban bao cao day du moi co hai mat na de ve sau the IC. Cac loi
        dap ngan (vi du "Set all channels to 102") thi bo qua.             */
    if (!txt.includes('BAO CAO CHAN DOAN CHUOI')) return false;

    clearTimeout(checkingTimer);
    lastReport = txt;
    renderReport(txt);
    if (!autoRefreshTimer) toast(t('tGotReport'));
    return true;
}

/*  Tach rieng phan ve giao dien de goi lai duoc khi doi ngon ngu. */
function renderReport(txt) {
    /*  Doc hai mat na o dau ban bao cao. Day la nguon tin cay nhat vi no do
        chinh xac tung bit, khong phu thuoc viec tach van ban theo doan IC. */
    const mOn   = txt.match(/tra loi lenh doc:\s*0x([0-9A-Fa-f]{2})/i);
    const mFail = txt.match(/ghi WIDTH that bai:\s*0x([0-9A-Fa-f]{2})/i);
    const online = mOn   ? parseInt(mOn[1], 16)   : 0;
    const fail   = mFail ? parseInt(mFail[1], 16) : 0xFF;

    let good = 0;
    const missing = [];

    for (let i = 0; i < 6; i++) {
        const bit      = 1 << i;
        const canRead  = (online & bit) !== 0;
        const canWrite = (fail   & bit) === 0;

        const seg = txt.match(new RegExp(`IC30${i + 1}\\b[\\s\\S]*?(?=IC30${i + 2}\\b|QUET CA|$)`, 'i'));
        let icid = '—', st = '—';
        if (seg) {
            const a = seg[0].match(/ICID\s*:\s*OK\s*=\s*(0x[0-9A-Fa-f]{2})/i);
            if (a) icid = a[1];
            const b = seg[0].match(/STATUS=(0x[0-9A-Fa-f]{2})/i);
            if (b) st = b[1];
        }

        let cls, label;
        if (canRead && canWrite) { cls = 'on';   label = t('icGood');      good++; }
        else if (canWrite)       { cls = 'warn'; label = t('icWriteOnly'); good++; }
        else                     { cls = 'off';  label = t('icNoComm'); missing.push(IC_NAMES[i]); }

        $(`ic-${i}`).className       = `ic-card ${cls}`;
        $(`st-${i}`).className       = `ic-state ${cls}`;
        $(`st-${i}`).textContent     = label;
        $(`v-icid-${i}`).textContent = icid === '0x96' ? t('idMaster')
                                     : icid === '0x98' ? t('idSlave') : icid;

        /*  STATUS bit 0 (PWR) = 1 nghia la DA khoi tao, tuc binh thuong.
            Ban truoc to mau nguoc: coi 0x00 la tot va 0x01 la loi. */
        const stv = st === '—' ? null : parseInt(st, 16);
        $(`v-st-${i}`).textContent = stv === null ? '—'
                                   : (stv & 0x01) ? t('stNormal', st) : t('stNotInit', st);
        $(`v-st-${i}`).className   = 'ic-val ' +
            (stv === null ? '' : (((stv & 0xFE) === 0 && (stv & 0x01)) ? 'ok' : 'err'));
    }

    renderTemp(txt);

    if (good === 6)      setVerdict('ok',   '✓', t('vAllOk'), t('vAllOkSub'));
    else if (good === 0) setVerdict('bad',  '✕', t('vNone'),  t('vNoneSub'));
    else                 setVerdict('warn', '!', t('vPartial', good),
                                     t('vPartialSub', missing.join(', ')));
}
