/* =========================================================================
   SSL96 ADB - Da ngon ngu (Viet / English / 한국어)

   Cach dung trong HTML:
       data-i18n="key"        -> thay textContent
       data-i18n-html="key"   -> thay innerHTML (dung cho doan co the <b>)
       data-i18n-title="key"  -> thay thuoc tinh title

   Cach dung trong JS:
       t('key')               -> tra ve chuoi
       t('key', a, b)         -> thay {0}, {1} bang a, b
   ========================================================================= */

const I18N = {

/* ------------------------------------------------------------------ VIET */
vi: {
  langTip:         'Đổi ngôn ngữ',
  langName:        'Tiếng Việt',
  subtitle:        'Bảng điều khiển module đèn ADB',
  connected:       'Đã kết nối',
  disconnected:    'Chưa kết nối',
  btnConnect:      'Kết nối',
  btnDisconnect:   'Ngắt kết nối',
  btnTheme:        'Đổi nền',

  modelName:      '{0} điểm ảnh',
  modelPickTitle: 'Chọn model module',
  modelPickHint:  'Kéo chuột trên hình để xoay xem. Mỗi chấm sáng là một điểm ảnh.',
  modelNoteReal:  'Bố trí đèn vẽ theo đúng bảng ánh xạ thật của module: 4 chuỗi A–D, hai chuỗi trên ngắn hơn nên tấm đèn thót lại phía trên.',
  modelNoteStub:  'Chưa có bảng ánh xạ thật. Hình chỉ là lưới tạm cho đủ {0} điểm ảnh, không phải bố trí thực tế.',
  lblModel:    'Model',
  modelReady:  'Sẵn sàng',
  modelSoon:   'Sắp có',
  tModelSet:   'Đang điều khiển module {0} pixel',
  tModelSoon:  'Module {0} pixel chưa được hỗ trợ, phần này đang phát triển',

  tabDash:  'Tổng quan',
  tabMap:   'Bản đồ đèn',
  tabTerm:  'Nhật ký chi tiết',
  tabHelp:  'Hướng dẫn',

  vNoData:      'Chưa có dữ liệu',
  vNoDataSub:   'Bấm “Kiểm tra module” để bắt đầu.',
  vNoConn:      'Chưa kết nối board',
  vNoConnSub:   'Cắm cáp USB của board vào máy rồi bấm Kết nối.',
  vChecking:    'Đang kiểm tra',
  vCheckingSub: 'Đang hỏi lần lượt 6 mạch, chờ vài giây.',
  vAllOk:       'Cả 6 mạch hoạt động tốt',
  vAllOkSub:    'Module sẵn sàng. Chỉnh độ sáng rồi bấm Bật tất cả.',
  vNone:        'Không mạch nào trả lời',
  vNoneSub:     'Kiểm tra cáp 12 chân, nguồn 5V của module, và hai dây CAN.',
  vPartial:     '{0} trên 6 mạch hoạt động',
  vPartialSub:  'Chưa liên lạc được: {0}. Xem tab Hướng dẫn để biết cách xử lý.',

  vTimeout:     'Không nhận được trả lời',
  vTimeoutSub:  'Board không gửi báo cáo về. Rút cáp USB cắm lại rồi thử lần nữa.',
  vCheckFail:   'Kiểm tra thất bại',
  vCheckFailSub:'Không gửi được lệnh xuống board. Xem lại kết nối USB rồi bấm Kiểm tra module.',
  vNoReport:    'Trả lời không đúng định dạng',
  vNoReportSub: 'Board có trả lời nhưng không phải báo cáo chẩn đoán. Xem tab Nhật ký chi tiết.',

  btnCheck:   'Kiểm tra module',
  hIcStatus:  'Tình trạng 6 mạch điều khiển',
  hIcHint:    'Mỗi mạch điều khiển 16 đèn',
  icMaster:   'mạch chính',
  icSlave:    'mạch phụ',
  icUnknown:  'Chưa rõ',
  icGood:     'Tốt',
  icWriteOnly:'Ghi được, đọc lỗi',
  icNoComm:   'Không liên lạc',
  rowRange:   'Điều khiển 16 đèn',
  rowIcid:    'Mã nhận dạng',
  rowStatus:  'Trạng thái',
  rangeFmt:   'chuỗi {0} · vị trí {1}–{2}',
  idMaster:   '0x96 (mạch chính)',
  idSlave:    '0x98 (mạch phụ)',
  stNormal:   '{0} bình thường',
  stNotInit:  '{0} chưa khởi tạo',

  hTemp:        'Nhiệt độ',
  hTempHint:    'NTC cạnh chuỗi đèn + cảm biến trong lòng IC301',
  ntcLabel:     'NTC cạnh chuỗi đèn (R306)',
  ntcWhere:     'Qua ADC1 của IC301 · điểm đo TP376',
  ntcDetail:    'Điện trở NTC {0} Ω · ADC1/ADC2 {1}',
  ntcBadRead:   'Số đọc không hợp lệ — kiểm tra NTC và cầu phân áp',
  ntcHow:       'NTC TDK B57332V5103F360: 10 kΩ ở 25 °C, B25/100 = 3455 K. Quy đổi dựa trên tỉ số ADC1/ADC2 nên không phụ thuộc điện áp VDK1.',
  tempDie:      'Nhiệt độ trong IC301',
  tempRaw:      'Giá trị thô {0}',
  tempOldFw:    'Firmware trên board chưa hỗ trợ đọc NTC',
  tempOldFwHow: 'Nạp lại file build\\SSL96_ADB_TPS9266x.hex rồi bấm Kiểm tra module',
  tempNoData:   'Chưa có dữ liệu',
  tempUnread:   'Không đọc được cảm biến',
  tempOk:       'bình thường',
  tempHot:      'QUÁ NHIỆT',
  tempNoAnswer: 'không trả lời',

  hLight:     'Bật đèn',
  hLightHint: 'Chỉnh độ sáng rồi bấm “Bật tất cả”',
  lblBright:  'Độ sáng',
  btnAllOn:   'Bật tất cả',
  btnAllOff:  'Tắt hết',
  btnRestore: 'Về mặc định',
  budgetHdr:  'Điện áp cần cho chuỗi dài nhất (ANODE#1, 4 mạch nối tiếp)',
  budgetSafe: 'Khoảng {0} đèn sáng cùng lúc trên {1} kênh. An toàn.',
  budgetNear: 'Khoảng {0} đèn sáng cùng lúc. Đã sát giới hạn, nên giảm bớt.',
  budgetOver: 'Vượt ngân sách. Bộ nguồn không đẩy nổi điện áp nên đèn sẽ TỐI ĐI thay vì sáng hơn.',
  lblPreset:  'Mức nhanh',

  hPerIc:      'Độ sáng riêng từng mạch',
  hPerIcHint:  'Mỗi mạch một mức riêng — cả sáu cùng sáng, không mạch nào bị tắt',
  btnApplyPerIc: 'Gửi cả sáu mạch',
  btnPercEven: 'Đặt tất cả 10%',
  percHint:    'Nút ▶ ở mỗi hàng gửi riêng mạch đó',
  percSendTip: 'Gửi mức sáng cho {0}',
  tIcLevel:    'Đã đặt {0} ở {1}',
  tPerIcApplied: 'Đã gửi mức sáng cho cả sáu mạch',

  hOnly:      'Thử riêng một mạch',
  hOnlyHint:  'Bật một mạch, tắt năm mạch còn lại — để biết nhóm đèn nào thuộc mạch nào',
  btnCycle:   'Chạy lần lượt 6 mạch',
  btnCycleStop:'Dừng lại',
  cycleHint:  'Mỗi mạch sáng 2 giây, lặp lại cho tới khi bấm dừng',

  mapHint:    'Mỗi ô là một đèn. Bấm để chọn, chọn xong đặt độ sáng rồi bấm áp dụng. Những đèn không chọn sẽ tắt.',
  lblFilter:  'Lọc chuỗi',
  fAll:       'Tất cả',
  btnSelAll:  'Chọn hết',
  btnClearSel:'Bỏ chọn',
  mapNone:    'Chưa chọn đèn nào',
  mapSel:     'Đã chọn {0} đèn',
  btnApplyMap:'Áp dụng cho đèn đã chọn',
  strRow:     'Chuỗi {0}',

  termHint:   'Nội dung nguyên văn do board gửi về. Dùng khi cần tra cứu sâu.',
  lblAuto:    'Tự động cập nhật mỗi 3 giây',
  btnReread:  'Đọc lại',
  btnClear:   'Xoá',
  termEmpty:  'Chưa có báo cáo. Bấm “Kiểm tra module” ở tab Tổng quan.',

  helpHtml: `
    <h3>Dùng lần đầu</h3>
    <ol>
      <li>Cắm cáp USB của board vào máy. Ô góc phải phải chuyển sang <b>Đã kết nối</b>.</li>
      <li>Cấp nguồn 5V cho module <b>trước</b>, đợi vài giây, rồi mới bật nguồn đèn.</li>
      <li>Bấm <b>Kiểm tra module</b>. Sáu thẻ mạch sẽ hiện tình trạng.</li>
      <li>Kéo thanh độ sáng về <b>10%</b> rồi bấm <b>Bật tất cả</b>.</li>
    </ol>
    <h3>Vì sao không nên để độ sáng quá cao</h3>
    <p>Bộ nguồn đèn cấp tối đa khoảng <b>40V</b>. Mỗi đèn cần khoảng 3V khi sáng.
       Chuỗi ANODE#1 nối tiếp 4 mạch, tức 64 đèn dùng chung ngân sách điện áp đó.</p>
    <p>Các đèn không sáng cùng lúc mà thay phiên nhau rất nhanh, nên mắt vẫn thấy tất cả
       cùng sáng. Nhưng nếu đặt độ sáng quá cao, số đèn sáng đồng thời vượt ngân sách,
       bộ nguồn không đẩy nổi điện áp và <b>đèn sẽ tối đi thay vì sáng hơn</b>.</p>
    <p>Thước đo dưới thanh trượt tính sẵn con số này. Giữ nó trong vùng xanh là an toàn.</p>
    <h3>Khi một mạch báo không liên lạc</h3>
    <ul>
      <li>Kiểm tra cáp 12 chân và nguồn 5V của module.</li>
      <li>Bấm <b>Kiểm tra module</b> lại vài lần, xem có ổn định không.</li>
      <li>Xem tab <b>Nhật ký chi tiết</b> để biết mạch đó hỏng ở chiều đọc hay chiều ghi.</li>
      <li>Nếu chỉ một mạch hỏng cố định, nhiều khả năng là mối hàn ở chân UART hoặc chân địa chỉ.</li>
    </ul>
    <h3>Ý nghĩa vài con số</h3>
    <table class="help-tbl">
      <tr><td><b>Mã nhận dạng</b></td><td>0x96 là mạch chính (master), 0x98 là mạch phụ (slave)</td></tr>
      <tr><td><b>Trạng thái 0x01</b></td><td>Bình thường, đã khởi tạo xong</td></tr>
      <tr><td><b>Trạng thái 0x00</b></td><td>Chưa khởi tạo hoặc vừa mất nguồn</td></tr>
      <tr><td><b>Độ sáng 10%</b></td><td>Mỗi đèn sáng 1/10 thời gian, mắt vẫn thấy liên tục</td></tr>
    </table>`,

  tOk:        'Đã kết nối board',
  tOff:       'Đã ngắt kết nối',
  tConnErr:   'Không kết nối được',
  tCfgErr:    'Không đọc được cấu hình từ máy chủ',
  tCmdErr:    'Lệnh thất bại',
  tGotReport: 'Đã nhận báo cáo từ board',
  tRestored:  'Đã trả về bảng độ sáng mặc định',
  tAllOff:    'Đã tắt hết đèn',
  tSetLevel:  'Đã đặt độ sáng {0}',
  tOnlyIc:    'Chỉ bật {0} ở {1} — nhìn xem nhóm đèn nào sáng',
  tCycleOn:   'Đang bật {0}',
  tCycleStop: 'Đã dừng',
  tPickLed:   'Chọn ít nhất một đèn trên bản đồ',
  tApplying:  'Đang đặt {0} đèn…',
  tApplied:   'Xong. {0} đèn ở {1}'
},

/* --------------------------------------------------------------- ENGLISH */
en: {
  langTip:         'Change language',
  langName:        'English',
  subtitle:        'ADB LED module control panel',
  connected:       'Connected',
  disconnected:    'Not connected',
  btnConnect:      'Connect',
  btnDisconnect:   'Disconnect',
  btnTheme:        'Theme',

  modelName:      '{0} pixel',
  modelPickTitle: 'Choose a module model',
  modelPickHint:  'Drag on a picture to rotate it. Each glowing dot is one pixel.',
  modelNoteReal:  'Drawn from the module’s real pixel map: 4 strings A–D, the top two shorter, so the panel tapers upward.',
  modelNoteStub:  'No real pixel map yet. This is a placeholder grid holding {0} pixels, not the actual layout.',
  lblModel:    'Model',
  modelReady:  'Ready',
  modelSoon:   'Coming soon',
  tModelSet:   'Now controlling the {0}-pixel module',
  tModelSoon:  'The {0}-pixel module is not supported yet — still in development',

  tabDash:  'Overview',
  tabMap:   'LED map',
  tabTerm:  'Detailed log',
  tabHelp:  'Guide',

  vNoData:      'No data yet',
  vNoDataSub:   'Press “Check module” to begin.',
  vNoConn:      'Board not connected',
  vNoConnSub:   'Plug in the board’s USB cable, then press Connect.',
  vChecking:    'Checking',
  vCheckingSub: 'Querying all six drivers, this takes a few seconds.',
  vAllOk:       'All 6 drivers are healthy',
  vAllOkSub:    'Module ready. Set a brightness, then press Turn all on.',
  vNone:        'No driver responded',
  vNoneSub:     'Check the 12-pin cable, the module’s 5V supply, and both CAN wires.',
  vPartial:     '{0} of 6 drivers working',
  vPartialSub:  'No contact with: {0}. See the Guide tab for what to do.',

  vTimeout:     'No answer received',
  vTimeoutSub:  'The board sent no report. Unplug and replug the USB cable, then try again.',
  vCheckFail:   'Check failed',
  vCheckFailSub:'The command could not be sent. Check the USB connection, then press Check module.',
  vNoReport:    'Unexpected reply',
  vNoReportSub: 'The board answered, but not with a diagnostic report. See the Detailed log tab.',

  btnCheck:   'Check module',
  hIcStatus:  'Status of the 6 driver ICs',
  hIcHint:    'Each driver controls 16 LEDs',
  icMaster:   'master',
  icSlave:    'slave',
  icUnknown:  'Unknown',
  icGood:     'Healthy',
  icWriteOnly:'Writes OK, reads fail',
  icNoComm:   'No communication',
  rowRange:   'Controls 16 LEDs',
  rowIcid:    'Device ID',
  rowStatus:  'Status',
  rangeFmt:   'strings {0} · positions {1}–{2}',
  idMaster:   '0x96 (master)',
  idSlave:    '0x98 (slave)',
  stNormal:   '{0} normal',
  stNotInit:  '{0} not initialised',

  hTemp:        'Temperature',
  hTempHint:    'NTC beside the LED string + IC301’s on-die sensor',
  ntcLabel:     'NTC beside the LED string (R306)',
  ntcWhere:     'Via IC301’s ADC1 · test point TP376',
  ntcDetail:    'NTC resistance {0} Ω · ADC1/ADC2 {1}',
  ntcBadRead:   'Invalid reading — check the NTC and its divider',
  ntcHow:       'TDK B57332V5103F360 NTC: 10 kΩ at 25 °C, B25/100 = 3455 K. The conversion uses the ADC1/ADC2 ratio, so it does not depend on the VDK1 rail voltage.',
  tempDie:      'IC301 die temperature',
  tempRaw:      'Raw value {0}',
  tempOldFw:    'The board firmware cannot read the NTC yet',
  tempOldFwHow: 'Flash build\\SSL96_ADB_TPS9266x.hex, then press Check module',
  tempNoData:   'No data yet',
  tempUnread:   'Sensor could not be read',
  tempOk:       'normal',
  tempHot:      'OVERHEATING',
  tempNoAnswer: 'no answer',

  hLight:     'Turn on the LEDs',
  hLightHint: 'Set a brightness, then press “Turn all on”',
  lblBright:  'Brightness',
  btnAllOn:   'Turn all on',
  btnAllOff:  'Turn all off',
  btnRestore: 'Restore default',
  budgetHdr:  'Voltage needed by the longest chain (ANODE#1, 4 drivers in series)',
  budgetSafe: 'About {0} LEDs lit at once across {1} channels. Safe.',
  budgetNear: 'About {0} LEDs lit at once. Close to the limit, consider lowering.',
  budgetOver: 'Over budget. The supply cannot reach this voltage, so the LEDs will get DIMMER, not brighter.',
  lblPreset:  'Presets',

  hPerIc:      'Per-driver brightness',
  hPerIcHint:  'Each driver keeps its own level — all six stay lit, none is turned off',
  btnApplyPerIc: 'Send all six',
  btnPercEven: 'Set all to 10%',
  percHint:    'The ▶ button on a row sends just that driver',
  percSendTip: 'Send brightness to {0}',
  tIcLevel:    '{0} set to {1}',
  tPerIcApplied: 'Brightness sent to all six drivers',

  hOnly:      'Test one driver',
  hOnlyHint:  'Lights one driver and turns the other five off — shows which LED group belongs to which driver',
  btnCycle:   'Cycle through all 6',
  btnCycleStop:'Stop',
  cycleHint:  'Each driver lights for 2 seconds, repeating until you stop it',

  mapHint:    'Each square is one LED. Click to select, set a brightness, then apply. Unselected LEDs turn off.',
  lblFilter:  'Filter string',
  fAll:       'All',
  btnSelAll:  'Select all',
  btnClearSel:'Clear selection',
  mapNone:    'No LEDs selected',
  mapSel:     '{0} LEDs selected',
  btnApplyMap:'Apply to selected LEDs',
  strRow:     'String {0}',

  termHint:   'Raw text sent by the board. Use this when you need the details.',
  lblAuto:    'Refresh automatically every 3 seconds',
  btnReread:  'Read again',
  btnClear:   'Clear',
  termEmpty:  'No report yet. Press “Check module” on the Overview tab.',

  helpHtml: `
    <h3>First time</h3>
    <ol>
      <li>Plug the board’s USB cable into the PC. The badge top right must read <b>Connected</b>.</li>
      <li>Power the module’s 5V rail <b>first</b>, wait a few seconds, then switch on the LED supply.</li>
      <li>Press <b>Check module</b>. The six driver cards will show their state.</li>
      <li>Set the brightness slider to <b>10%</b> and press <b>Turn all on</b>.</li>
    </ol>
    <h3>Why you should not push the brightness too high</h3>
    <p>The LED supply tops out at about <b>40V</b>. Each LED needs roughly 3V when lit.
       The ANODE#1 chain puts 4 drivers in series, so 64 LEDs share that one voltage budget.</p>
    <p>The LEDs do not all light at the same instant — they take turns very quickly, so the eye
       sees them all lit. But if the brightness is too high, the number lit simultaneously exceeds
       the budget, the supply cannot reach that voltage, and <b>the LEDs get dimmer instead of
       brighter</b>.</p>
    <p>The meter under the slider works this out for you. Keep it in the green.</p>
    <h3>When a driver reports no communication</h3>
    <ul>
      <li>Check the 12-pin cable and the module’s 5V supply.</li>
      <li>Press <b>Check module</b> a few more times and see whether it is stable.</li>
      <li>Open the <b>Detailed log</b> tab to see whether it fails on reads or on writes.</li>
      <li>If exactly one driver fails consistently, a solder joint on its UART or address pins is the likely cause.</li>
    </ul>
    <h3>What the numbers mean</h3>
    <table class="help-tbl">
      <tr><td><b>Device ID</b></td><td>0x96 is the master, 0x98 is a slave</td></tr>
      <tr><td><b>Status 0x01</b></td><td>Normal, initialisation complete</td></tr>
      <tr><td><b>Status 0x00</b></td><td>Not initialised, or power was just lost</td></tr>
      <tr><td><b>Brightness 10%</b></td><td>Each LED is lit 1/10 of the time; the eye still sees it as steady</td></tr>
    </table>`,

  tOk:        'Board connected',
  tOff:       'Disconnected',
  tConnErr:   'Could not connect',
  tCfgErr:    'Could not load configuration from the server',
  tCmdErr:    'Command failed',
  tGotReport: 'Report received from the board',
  tRestored:  'Default brightness table restored',
  tAllOff:    'All LEDs off',
  tSetLevel:  'Brightness set to {0}',
  tOnlyIc:    'Only {0} lit at {1} — look at which LED group comes on',
  tCycleOn:   'Lighting {0}',
  tCycleStop: 'Stopped',
  tPickLed:   'Select at least one LED on the map',
  tApplying:  'Setting {0} LEDs…',
  tApplied:   'Done. {0} LEDs at {1}'
},

/* ---------------------------------------------------------------- 한국어 */
ko: {
  langTip:         '언어 변경',
  langName:        '한국어',
  subtitle:        'ADB LED 모듈 제어판',
  connected:       '연결됨',
  disconnected:    '연결 안 됨',
  btnConnect:      '연결',
  btnDisconnect:   '연결 해제',
  btnTheme:        '테마',

  modelName:      '{0}픽셀',
  modelPickTitle: '모듈 모델 선택',
  modelPickHint:  '그림을 끌어서 돌려 볼 수 있습니다. 빛나는 점 하나가 픽셀 하나입니다.',
  modelNoteReal:  '모듈의 실제 픽셀 맵으로 그렸습니다. A–D 4개 스트링이며 위쪽 두 줄이 짧아 위로 갈수록 좁아집니다.',
  modelNoteStub:  '실제 픽셀 맵이 아직 없습니다. {0}개 픽셀을 채운 임시 격자이며 실제 배치가 아닙니다.',
  lblModel:    '모델',
  modelReady:  '사용 가능',
  modelSoon:   '준비 중',
  tModelSet:   '{0}픽셀 모듈을 제어합니다',
  tModelSoon:  '{0}픽셀 모듈은 아직 지원되지 않습니다 — 개발 중입니다',

  tabDash:  '개요',
  tabMap:   'LED 배치도',
  tabTerm:  '상세 로그',
  tabHelp:  '사용 안내',

  vNoData:      '데이터 없음',
  vNoDataSub:   '“모듈 점검”을 눌러 시작하세요.',
  vNoConn:      '보드가 연결되지 않았습니다',
  vNoConnSub:   '보드의 USB 케이블을 연결한 뒤 연결 버튼을 누르세요.',
  vChecking:    '점검 중',
  vCheckingSub: '6개 드라이버를 차례로 확인하고 있습니다. 잠시 기다리세요.',
  vAllOk:       '6개 드라이버 모두 정상',
  vAllOkSub:    '모듈 준비 완료. 밝기를 정한 뒤 전체 켜기를 누르세요.',
  vNone:        '응답하는 드라이버가 없습니다',
  vNoneSub:     '12핀 케이블, 모듈의 5V 전원, CAN 배선 2가닥을 확인하세요.',
  vPartial:     '6개 중 {0}개 동작 중',
  vPartialSub:  '통신 불가: {0}. 처리 방법은 사용 안내 탭을 보세요.',

  vTimeout:     '응답이 없습니다',
  vTimeoutSub:  '보드가 보고서를 보내지 않았습니다. USB 케이블을 뽑았다 다시 꽂고 시도하세요.',
  vCheckFail:   '점검 실패',
  vCheckFailSub:'명령을 보내지 못했습니다. USB 연결을 확인한 뒤 모듈 점검을 누르세요.',
  vNoReport:    '예상과 다른 응답',
  vNoReportSub: '보드가 응답했지만 진단 보고서가 아닙니다. 상세 로그 탭을 확인하세요.',

  btnCheck:   '모듈 점검',
  hIcStatus:  '드라이버 IC 6개 상태',
  hIcHint:    '드라이버 1개가 LED 16개를 담당합니다',
  icMaster:   '마스터',
  icSlave:    '슬레이브',
  icUnknown:  '미확인',
  icGood:     '정상',
  icWriteOnly:'쓰기 정상, 읽기 오류',
  icNoComm:   '통신 불가',
  rowRange:   'LED 16개 담당',
  rowIcid:    '식별 코드',
  rowStatus:  '상태',
  rangeFmt:   '{0} 스트링 · {1}–{2}번 위치',
  idMaster:   '0x96 (마스터)',
  idSlave:    '0x98 (슬레이브)',
  stNormal:   '{0} 정상',
  stNotInit:  '{0} 초기화 안 됨',

  hTemp:        '온도',
  hTempHint:    'LED 스트링 옆 NTC + IC301 내부 센서',
  ntcLabel:     'LED 스트링 옆 NTC (R306)',
  ntcWhere:     'IC301 ADC1 경유 · 테스트 포인트 TP376',
  ntcDetail:    'NTC 저항 {0} Ω · ADC1/ADC2 {1}',
  ntcBadRead:   '값이 올바르지 않습니다 — NTC와 분압 회로를 확인하세요',
  ntcHow:       'TDK B57332V5103F360 NTC: 25 °C에서 10 kΩ, B25/100 = 3455 K. ADC1/ADC2 비율로 환산하므로 VDK1 전압에 의존하지 않습니다.',
  tempDie:      'IC301 다이 온도',
  tempRaw:      '원시값 {0}',
  tempOldFw:    '보드 펌웨어가 아직 NTC를 읽지 못합니다',
  tempOldFwHow: 'build\\SSL96_ADB_TPS9266x.hex를 다시 플래시한 뒤 모듈 점검을 누르세요',
  tempNoData:   '데이터 없음',
  tempUnread:   '센서를 읽지 못했습니다',
  tempOk:       '정상',
  tempHot:      '과열',
  tempNoAnswer: '무응답',

  hLight:     'LED 켜기',
  hLightHint: '밝기를 정한 뒤 “전체 켜기”를 누르세요',
  lblBright:  '밝기',
  btnAllOn:   '전체 켜기',
  btnAllOff:  '전체 끄기',
  btnRestore: '기본값 복원',
  budgetHdr:  '가장 긴 체인에 필요한 전압 (ANODE#1, 드라이버 4개 직렬)',
  budgetSafe: '{1}개 채널 중 약 {0}개 LED가 동시에 점등. 안전합니다.',
  budgetNear: '약 {0}개 LED가 동시에 점등. 한계에 근접했으니 낮추는 것이 좋습니다.',
  budgetOver: '예산 초과입니다. 전원이 이 전압까지 올리지 못하므로 LED가 더 밝아지지 않고 오히려 어두워집니다.',
  lblPreset:  '빠른 설정',

  hPerIc:      '드라이버별 밝기',
  hPerIcHint:  '드라이버마다 밝기를 따로 지정 — 6개 모두 켜진 상태를 유지합니다',
  btnApplyPerIc: '6개 모두 전송',
  btnPercEven: '전체 10%로 설정',
  percHint:    '각 줄의 ▶ 버튼은 해당 드라이버만 전송합니다',
  percSendTip: '{0}에 밝기 전송',
  tIcLevel:    '{0}을(를) {1}로 설정',
  tPerIcApplied: '6개 드라이버 모두에 밝기를 전송했습니다',

  hOnly:      '드라이버 개별 점검',
  hOnlyHint:  '한 드라이버만 켜고 나머지 5개는 끕니다 — 어느 LED 그룹이 어느 드라이버인지 확인용',
  btnCycle:   '6개 순차 점등',
  btnCycleStop:'정지',
  cycleHint:  '드라이버마다 2초씩 점등하며, 정지를 누를 때까지 반복합니다',

  mapHint:    '칸 하나가 LED 하나입니다. 클릭해 선택하고 밝기를 정한 뒤 적용하세요. 선택하지 않은 LED는 꺼집니다.',
  lblFilter:  '스트링 필터',
  fAll:       '전체',
  btnSelAll:  '전체 선택',
  btnClearSel:'선택 해제',
  mapNone:    '선택된 LED 없음',
  mapSel:     'LED {0}개 선택됨',
  btnApplyMap:'선택한 LED에 적용',
  strRow:     '{0} 스트링',

  termHint:   '보드가 보낸 원문입니다. 자세히 확인할 때 사용하세요.',
  lblAuto:    '3초마다 자동 갱신',
  btnReread:  '다시 읽기',
  btnClear:   '지우기',
  termEmpty:  '보고서가 없습니다. 개요 탭에서 “모듈 점검”을 누르세요.',

  helpHtml: `
    <h3>처음 사용할 때</h3>
    <ol>
      <li>보드의 USB 케이블을 PC에 연결합니다. 오른쪽 위 표시가 <b>연결됨</b>으로 바뀌어야 합니다.</li>
      <li>모듈의 5V 전원을 <b>먼저</b> 켜고 몇 초 기다린 뒤 LED 전원을 켭니다.</li>
      <li><b>모듈 점검</b>을 누릅니다. 드라이버 6개의 상태 카드가 표시됩니다.</li>
      <li>밝기 슬라이더를 <b>10%</b>로 맞추고 <b>전체 켜기</b>를 누릅니다.</li>
    </ol>
    <h3>밝기를 너무 높이면 안 되는 이유</h3>
    <p>LED 전원은 최대 약 <b>40V</b>까지만 공급합니다. LED 하나가 점등할 때 약 3V가 필요합니다.
       ANODE#1 체인은 드라이버 4개가 직렬이라 LED 64개가 이 전압 예산을 함께 씁니다.</p>
    <p>LED는 동시에 켜지는 것이 아니라 매우 빠르게 번갈아 켜지므로 눈에는 모두 켜진 것처럼 보입니다.
       다만 밝기를 너무 높이면 동시에 켜지는 개수가 예산을 넘어서고, 전원이 그 전압까지 올리지 못해
       <b>LED가 더 밝아지지 않고 오히려 어두워집니다</b>.</p>
    <p>슬라이더 아래 계기가 이 값을 미리 계산해 줍니다. 녹색 범위 안에서 사용하세요.</p>
    <h3>드라이버가 통신 불가로 표시될 때</h3>
    <ul>
      <li>12핀 케이블과 모듈의 5V 전원을 확인합니다.</li>
      <li><b>모듈 점검</b>을 여러 번 눌러 결과가 일정한지 봅니다.</li>
      <li><b>상세 로그</b> 탭에서 읽기 쪽이 실패하는지 쓰기 쪽이 실패하는지 확인합니다.</li>
      <li>특정 드라이버 하나만 계속 실패하면 UART 핀이나 주소 핀의 납땜 불량일 가능성이 높습니다.</li>
    </ul>
    <h3>숫자의 의미</h3>
    <table class="help-tbl">
      <tr><td><b>식별 코드</b></td><td>0x96은 마스터, 0x98은 슬레이브</td></tr>
      <tr><td><b>상태 0x01</b></td><td>정상, 초기화 완료</td></tr>
      <tr><td><b>상태 0x00</b></td><td>초기화되지 않았거나 방금 전원이 끊긴 상태</td></tr>
      <tr><td><b>밝기 10%</b></td><td>LED마다 시간의 1/10만 점등하지만 눈에는 계속 켜진 것으로 보입니다</td></tr>
    </table>`,

  tOk:        '보드에 연결되었습니다',
  tOff:       '연결을 해제했습니다',
  tConnErr:   '연결하지 못했습니다',
  tCfgErr:    '서버에서 설정을 읽지 못했습니다',
  tCmdErr:    '명령이 실패했습니다',
  tGotReport: '보드에서 보고서를 받았습니다',
  tRestored:  '기본 밝기표로 복원했습니다',
  tAllOff:    'LED를 모두 껐습니다',
  tSetLevel:  '밝기를 {0}로 설정했습니다',
  tOnlyIc:    '{0}만 {1}로 점등 — 어느 LED 그룹이 켜지는지 보세요',
  tCycleOn:   '{0} 점등 중',
  tCycleStop: '정지했습니다',
  tPickLed:   '배치도에서 LED를 하나 이상 선택하세요',
  tApplying:  'LED {0}개 설정 중…',
  tApplied:   '완료. LED {0}개를 {1}로 설정'
}

};

/* ------------------------------------------------------------------------ */
let LANG = localStorage.getItem('lang') || 'vi';
if (!I18N[LANG]) LANG = 'vi';

/*  Lay chuoi theo khoa hien tai. Neu ngon ngu dang chon thieu khoa nao thi
    lay tam ban tieng Viet, de giao dien khong bao gio hien ra khoa tho. */
function t(key, ...args) {
    let s = (I18N[LANG] && I18N[LANG][key]) || I18N.vi[key] || key;
    args.forEach((v, i) => { s = s.split('{' + i + '}').join(v); });
    return s;
}

/*  Quet toan bo trang va thay chu theo ngon ngu dang chon. */
function applyLang(lang) {
    if (lang && I18N[lang]) { LANG = lang; localStorage.setItem('lang', lang); }

    document.documentElement.lang = LANG;

    document.querySelectorAll('[data-i18n]').forEach(el => {
        el.textContent = t(el.dataset.i18n);
    });
    document.querySelectorAll('[data-i18n-html]').forEach(el => {
        el.innerHTML = t(el.dataset.i18nHtml);
    });
    document.querySelectorAll('[data-i18n-title]').forEach(el => {
        el.title = t(el.dataset.i18nTitle);
    });

    const cur = document.getElementById('lang-cur-name');
    if (cur) cur.textContent = I18N[LANG].langName;

    document.querySelectorAll('.lang-item').forEach(b => {
        b.classList.toggle('on', b.dataset.lang === LANG);
        b.setAttribute('aria-selected', b.dataset.lang === LANG);
    });

    // Phan noi dung dong do app.js ve lai
    if (typeof refreshDynamicText === 'function') refreshDynamicText();
}

/* ------------------------------------------------------------------------
   Menu chon ngon ngu
   ------------------------------------------------------------------------ */
function toggleLangMenu(ev) {
    if (ev) ev.stopPropagation();
    const sw = document.getElementById('lang-sw');
    const open = sw.classList.toggle('open');
    document.getElementById('lang-cur').setAttribute('aria-expanded', open);
}

function closeLangMenu() {
    const sw = document.getElementById('lang-sw');
    if (!sw) return;
    sw.classList.remove('open');
    document.getElementById('lang-cur').setAttribute('aria-expanded', 'false');
}

function pickLang(lang) {
    closeLangMenu();
    applyLang(lang);
}

/*  Bam ra ngoai hoac nhan Esc thi dong menu lai. */
document.addEventListener('click', e => {
    if (!e.target.closest('#lang-sw')) closeLangMenu();
});
document.addEventListener('keydown', e => {
    if (e.key === 'Escape') closeLangMenu();
});
