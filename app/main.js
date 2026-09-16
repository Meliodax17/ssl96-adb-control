const { app, BrowserWindow, dialog } = require('electron');
const path = require('path');
const { autoUpdater } = require('electron-updater');

// Bật server giao tiếp USB
// Yêu cầu file server.js nằm cùng thư mục
require('./server.js');

let mainWindow;

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1280,
        height: 800,
        minWidth: 1024,
        minHeight: 768,
        title: "SSL96 ADB Control",
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true
        }
    });

    // Bỏ menu mặc định
    mainWindow.setMenuBarVisibility(false);

    // Tải giao diện từ server Express
    mainWindow.loadURL('http://localhost:3096');

    mainWindow.on('closed', function () {
        mainWindow = null;
    });
}

// Khi Electron đã khởi tạo xong
app.whenReady().then(() => {
    createWindow();

    app.on('activate', function () {
        if (BrowserWindow.getAllWindows().length === 0) createWindow();
    });

    // Bắt đầu kiểm tra bản cập nhật
    autoUpdater.checkForUpdatesAndNotify();
});

// Thoát khi tất cả cửa sổ bị đóng (trừ macOS)
app.on('window-all-closed', function () {
    if (process.platform !== 'darwin') app.quit();
});

/* ========================================================================= */
/* Cơ chế Auto-Updater cho Bản Phát Hành                                     */
/* ========================================================================= */
autoUpdater.on('update-available', () => {
    console.log('Update available.');
});

autoUpdater.on('update-downloaded', (info) => {
    console.log('Update downloaded.');
    // Khi tải xong bản cập nhật mới, hỏi người dùng có muốn cài đặt luôn không
    dialog.showMessageBox({
        type: 'info',
        title: 'Bản cập nhật đã sẵn sàng',
        message: `Phiên bản mới (${info.version}) đã được tải xuống hoàn tất.`,
        buttons: ['Cài đặt ngay & Khởi động lại', 'Để sau']
    }).then((result) => {
        if (result.response === 0) {
            autoUpdater.quitAndInstall();
        }
    });
});

autoUpdater.on('error', (err) => {
    console.error('Lỗi khi kiểm tra cập nhật: ' + err);
});
