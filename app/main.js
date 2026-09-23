const { app, BrowserWindow, dialog } = require('electron');
const path = require('path');
const { autoUpdater } = require('electron-updater');

/*  Chi cho phep MOT ban sao chay.
 *
 *  Mo ban thu hai se lam ban do dung cong 3096 cua ban dau va sinh ra loi
 *  EADDRINUSE. Thay vi de no sap, ta dua nguoi dung ve cua so dang mo.
 *  Phai kiem tra TRUOC khi nap server.js, vi chinh viec nap do mo cong.   */
const gotLock = app.requestSingleInstanceLock();
if (!gotLock) {
    app.quit();
} else {

/* Bat may chu giao tiep USB. Phai nam cung thu muc voi file nay. */
require('./server.js');

const URL_APP = 'http://localhost:3096';

/*  Cua so gioi thieu hien it nhat ngan nay, de doan video kip chay chu
 *  khong nhay mot cai roi bien mat tren may nhanh.                        */
const SPLASH_MIN_MS = 2400;

/*  Va nhieu nhat ngan nay. Neu trang chinh treo thi van phai cho nguoi
 *  dung thay cua so chinh, khong nhot ho o man hinh gioi thieu.           */
const SPLASH_MAX_MS = 15000;

let mainWindow = null;
let splashWindow = null;
let splashShownAt = 0;
let handedOver = false;

function createSplash() {
    splashWindow = new BrowserWindow({
        width: 720, height: 405,           /* 16:9 */
        frame: false,
        resizable: false,
        center: true,
        show: false,
        skipTaskbar: true,
        backgroundColor: '#000000',
        title: 'ADB Controller',
        webPreferences: { nodeIntegration: false, contextIsolation: true }
    });

    splashWindow.loadFile(path.join(__dirname, 'public', 'splash.html'));
    splashWindow.once('ready-to-show', () => {
        splashShownAt = Date.now();
        if (splashWindow) splashWindow.show();
    });
    splashWindow.on('closed', () => { splashWindow = null; });
}

function splashSay(text) {
    if (!splashWindow || splashWindow.isDestroyed()) return;
    const safe = JSON.stringify(String(text));
    splashWindow.webContents
        .executeJavaScript(`window.setStatus && window.setStatus(${safe})`)
        .catch(() => {});
}

/*  Chuyen tu cua so gioi thieu sang cua so chinh.
 *  Goi duoc nhieu lan, chi lan dau co tac dung.                           */
function handOver() {
    if (handedOver) return;
    handedOver = true;

    const waited    = Date.now() - splashShownAt;
    const remaining = Math.max(0, SPLASH_MIN_MS - waited);

    setTimeout(() => {
        if (splashWindow && !splashWindow.isDestroyed()) {
            splashWindow.webContents
                .executeJavaScript('window.leave && window.leave()')
                .catch(() => {});
            /* cho hieu ung mo dan chay xong roi moi dong */
            setTimeout(() => {
                if (splashWindow && !splashWindow.isDestroyed()) splashWindow.close();
            }, 450);
        }
        if (mainWindow && !mainWindow.isDestroyed()) {
            mainWindow.show();
            mainWindow.focus();
        }
    }, remaining);
}

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1280,
        height: 800,
        minWidth: 1024,
        minHeight: 768,
        show: false,                       /* chi hien sau khi tai xong */
        backgroundColor: '#050505',
        title: 'ADB Controller',
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true
        }
    });

    mainWindow.setMenuBarVisibility(false);

    splashSay('Đang kết nối máy chủ…');
    mainWindow.loadURL(URL_APP);

    mainWindow.webContents.on('did-finish-load', () => {
        splashSay('Đang dựng giao diện…');
        /*  Trang con lam mot vai lenh goi bat dong bo sau khi tai xong
            (doc cau hinh, do bang anh xa den). Cho mot nhip ngan cho no
            lang xuong, de cua so chinh hien ra da san sang chu khong con
            dang nhay so.                                                  */
        setTimeout(handOver, 350);
    });

    /*  Tai that bai - thuong la may chu chua kip mo cong. Thu lai vai lan
        roi moi chiu thua.                                                 */
    let retries = 0;
    mainWindow.webContents.on('did-fail-load', (_e, code, desc) => {
        if (handedOver) return;
        if (retries < 12) {
            retries++;
            splashSay(`Đang chờ máy chủ… (${retries})`);
            setTimeout(() => {
                if (mainWindow && !mainWindow.isDestroyed()) mainWindow.loadURL(URL_APP);
            }, 500);
            return;
        }
        splashSay('Không mở được giao diện');
        dialog.showErrorBox('Không mở được giao diện',
            `Không tải được ${URL_APP}\n\n${desc} (mã ${code})\n\n` +
            'Kiểm tra xem có bản sao nào khác của ứng dụng đang chạy không.');
        handOver();
    });

    /* Phong hờ: du co chuyen gi, cua so chinh van phai hien ra. */
    setTimeout(handOver, SPLASH_MAX_MS);

    mainWindow.on('closed', () => { mainWindow = null; });
}

app.whenReady().then(() => {
    createSplash();
    createWindow();

    app.on('activate', () => {
        if (BrowserWindow.getAllWindows().length === 0) createWindow();
    });

    autoUpdater.checkForUpdatesAndNotify();
});

/*  Nguoi dung bam vao loi tat lan nua: dua ho ve cua so dang mo. */
app.on('second-instance', () => {
    if (mainWindow && !mainWindow.isDestroyed()) {
        if (mainWindow.isMinimized()) mainWindow.restore();
        mainWindow.show();
        mainWindow.focus();
    }
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') app.quit();
});

/* ========================================================================= */
/* Co che tu cap nhat cho ban phat hanh                                      */
/* ========================================================================= */
autoUpdater.on('update-available', () => {
    console.log('Update available.');
});

autoUpdater.on('update-downloaded', (info) => {
    console.log('Update downloaded.');
    dialog.showMessageBox({
        type: 'info',
        title: 'Bản cập nhật đã sẵn sàng',
        message: `Phiên bản mới (${info.version}) đã được tải xuống hoàn tất.`,
        buttons: ['Cài đặt ngay & Khởi động lại', 'Để sau']
    }).then((result) => {
        if (result.response === 0) autoUpdater.quitAndInstall();
    });
});

autoUpdater.on('error', (err) => {
    console.error('Lỗi khi kiểm tra cập nhật: ' + err);
});

}   /* het khoi gotLock */
