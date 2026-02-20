#pragma once

#include <Arduino.h>

// Generated from data/index.html. Keep this file in sync after UI changes.
static const char WEB_UI_HTML[] PROGMEM = R"WEBUI(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>Moto32 Dashboard</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;600;700&family=Outfit:wght@300;400;500;600;700&display=swap');
:root{
  --bg:#06090f;--bg2:#0c1219;--panel:#111a25;--panel2:#162030;
  --border:#1e2d42;--border2:#2a3f5c;
  --text:#e4ecf7;--muted:#7b92b0;--dim:#4a6080;
  --accent:#00e68a;--accent2:#00cc7a;--accentDim:rgba(0,230,138,.1);
  --accentGlow:rgba(0,230,138,.2);
  --warn:#ffc246;--warnDim:rgba(255,194,70,.1);
  --danger:#ff5470;--dangerDim:rgba(255,84,112,.12);
  --info:#4da6ff;--infoDim:rgba(77,166,255,.1);
  --on:#00e68a;--off:#3a4a5e;
  --radius:10px;--radius2:14px;
}
*{box-sizing:border-box;margin:0;padding:0}
html{font-size:15px}
body{
  font-family:'Outfit',sans-serif;background:var(--bg);color:var(--text);
  min-height:100vh;-webkit-font-smoothing:antialiased;
}
body.advanced-mode{
  --accent:#ff5470;--accent2:#ff2b52;--accentDim:rgba(255,84,112,.14);
  --accentGlow:rgba(255,84,112,.28);
  --on:#ff6e89;
}
/* ─── Scrollbar ─── */
::-webkit-scrollbar{width:6px;height:6px}
::-webkit-scrollbar-track{background:var(--bg2)}
::-webkit-scrollbar-thumb{background:var(--border2);border-radius:3px}

/* ─── Header ─── */
.header{
  background:linear-gradient(180deg,var(--panel2) 0%,var(--panel) 100%);
  border-bottom:1px solid var(--border);
  padding:14px 20px;display:flex;align-items:center;gap:16px;
  position:sticky;top:0;z-index:100;
  backdrop-filter:blur(12px);
}
.logo{
  font-family:'JetBrains Mono',monospace;font-weight:700;font-size:1.3rem;
  color:var(--accent);letter-spacing:-.5px;
}
.logo span{color:var(--muted);font-weight:400;font-size:.8rem;margin-left:6px}
.conn-stack{
  display:flex;flex-direction:column;gap:4px;min-width:170px;
  font-family:'JetBrains Mono',monospace;
}
.conn-title{
  font-size:.76rem;font-weight:700;letter-spacing:.8px;color:var(--text);
}
.conn-badge{
  display:flex;align-items:center;gap:6px;
  font-size:.74rem;color:var(--muted);
}
.conn-kind{
  font-size:.66rem;font-weight:700;letter-spacing:.6px;
  color:var(--dim);min-width:24px;
}
.conn-dot{width:8px;height:8px;border-radius:50%;background:var(--danger);flex-shrink:0}
.conn-dot.ok{background:#00e68a;box-shadow:0 0 8px #00e68a}
.header-right{margin-left:auto;display:flex;align-items:center;gap:12px}
.mode-chip{
  font-family:'JetBrains Mono',monospace;font-size:.72rem;font-weight:700;
  border:1px solid var(--border);border-radius:999px;padding:5px 10px;
  background:var(--bg);color:var(--muted);letter-spacing:.5px;
}

/* ─── Language Switcher ─── */
.lang-switch{
  display:flex;gap:2px;background:var(--bg);border:1px solid var(--border);
  border-radius:6px;padding:2px;
}
.lang-btn{
  font-family:'JetBrains Mono',monospace;font-size:.7rem;font-weight:600;
  padding:4px 10px;border:none;border-radius:4px;cursor:pointer;
  background:transparent;color:var(--muted);transition:.2s;
}
.lang-btn:hover{color:var(--text)}
.lang-btn.active{background:var(--accent);color:#06090f}

/* ─── Tabs ─── */
.tabs{
  display:flex;gap:2px;padding:12px 20px 0;background:var(--bg);
}
.tab{
  padding:10px 22px;font-size:.85rem;font-weight:600;
  border:1px solid transparent;border-bottom:none;
  border-radius:var(--radius) var(--radius) 0 0;
  color:var(--muted);cursor:pointer;transition:.2s;
  background:transparent;font-family:inherit;
}
.tab:hover{color:var(--text);background:var(--panel)}
.tab.active{
  color:var(--accent);background:var(--panel);
  border-color:var(--border);
}
.tab-content{display:none;padding:20px;animation:fadeUp .3s ease}
.tab-content.active{display:block}
@keyframes fadeUp{from{opacity:0;transform:translateY(8px)}to{opacity:1;transform:none}}
.mode-banner{
  display:none;margin:10px 20px 0;border:1px solid rgba(255,84,112,.45);
  background:rgba(255,84,112,.14);color:var(--danger);
  border-radius:10px;padding:10px 14px;font-size:.8rem;font-weight:700;
  letter-spacing:.4px;text-transform:uppercase;
}
body.advanced-mode .mode-banner{display:block}

/* ─── Cards ─── */
.card{
  background:linear-gradient(170deg,var(--panel) 0%,var(--bg2) 100%);
  border:1px solid var(--border);border-radius:var(--radius2);
  padding:18px;margin-bottom:16px;
}
.card-title{
  font-size:.75rem;text-transform:uppercase;letter-spacing:1.5px;
  color:var(--muted);margin-bottom:14px;font-weight:600;
  display:flex;align-items:center;gap:8px;
}
.card-title::before{
  content:'';width:3px;height:14px;border-radius:2px;
  background:var(--accent);flex-shrink:0;
}

/* ─── Grid layouts ─── */
.grid2{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.grid3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px}
.grid4{display:grid;grid-template-columns:1fr 1fr 1fr 1fr;gap:10px}
@media(max-width:700px){
  .grid2,.grid3,.grid4{grid-template-columns:1fr 1fr}
}
@media(max-width:420px){
  .grid2,.grid3,.grid4{grid-template-columns:1fr}
}

/* ─── I/O Tiles ─── */
.io-tile{
  background:var(--bg);border:1px solid var(--border);
  border-radius:var(--radius);padding:12px;
  display:flex;flex-direction:column;gap:6px;
  transition:.25s;position:relative;overflow:hidden;
}
.io-tile.clickable{cursor:pointer}
.io-tile.clickable:hover{transform:translateY(-1px)}
.io-tile.locked{cursor:not-allowed;opacity:.7}
.io-tile.active{
  border-color:var(--accent);
  box-shadow:0 0 20px var(--accentGlow);
}
.io-tile.active::after{
  content:'';position:absolute;top:0;left:0;right:0;height:2px;
  background:linear-gradient(90deg,transparent,var(--accent),transparent);
}
.io-tile .name{
  font-size:.78rem;color:var(--muted);font-weight:500;
  display:flex;align-items:center;gap:6px;
}
.io-tile .pin{
  font-family:'JetBrains Mono',monospace;font-size:.65rem;
  color:var(--dim);margin-left:auto;
}
.io-tile .val{
  font-family:'JetBrains Mono',monospace;font-size:1.15rem;
  font-weight:700;color:var(--off);
}
.io-tile.active .val{color:var(--on)}
.io-tile .sub{font-size:.7rem;color:var(--dim);font-family:'JetBrains Mono',monospace}
.status-dot{
  width:7px;height:7px;border-radius:50%;
  background:var(--off);flex-shrink:0;transition:.25s;
}
.io-tile.active .status-dot{background:var(--on);box-shadow:0 0 6px var(--on)}

/* ─── Error flags ─── */
.err-chip{
  display:inline-flex;align-items:center;gap:5px;
  padding:4px 10px;border-radius:20px;font-size:.72rem;
  font-weight:600;font-family:'JetBrains Mono',monospace;
}
.err-chip.ok{background:var(--accentDim);color:var(--accent)}
.err-chip.warn{background:var(--warnDim);color:var(--warn)}
.err-chip.err{background:var(--dangerDim);color:var(--danger)}

/* ─── Settings ─── */
.setting-row{
  display:flex;align-items:center;justify-content:space-between;
  padding:12px 0;border-bottom:1px solid var(--border);gap:12px;
}
.setting-row:last-child{border:none}
.setting-label{
  display:flex;flex-direction:column;gap:2px;flex:1;min-width:0;
}
.setting-label .name{font-weight:500;font-size:.88rem}
.setting-label .desc{font-size:.72rem;color:var(--dim)}
.mode-switch{
  display:inline-flex;gap:4px;background:var(--bg);border:1px solid var(--border);
  border-radius:8px;padding:3px;
}
.mode-btn{
  border:none;border-radius:6px;padding:8px 14px;cursor:pointer;
  font-family:'JetBrains Mono',monospace;font-size:.74rem;font-weight:700;
  background:transparent;color:var(--muted);transition:.2s;
}
.mode-btn.active{background:var(--accent);color:#05080d}
select,input[type=number]{
  font-family:'JetBrains Mono',monospace;font-size:.82rem;
  background:var(--bg);border:1px solid var(--border);
  border-radius:6px;padding:8px 12px;color:var(--text);
  outline:none;min-width:140px;transition:.2s;
}
select:focus,input[type=number]:focus{border-color:var(--accent)}
select option{background:var(--panel);color:var(--text)}

/* Toggle switch */
.toggle{position:relative;width:44px;height:24px;flex-shrink:0}
.toggle input{opacity:0;width:0;height:0}
.toggle .slider{
  position:absolute;inset:0;background:var(--off);
  border-radius:12px;cursor:pointer;transition:.25s;
}
.toggle .slider::before{
  content:'';position:absolute;width:18px;height:18px;
  left:3px;bottom:3px;background:#fff;
  border-radius:50%;transition:.25s;
}
.toggle input:checked+.slider{background:var(--accent)}
.toggle input:checked+.slider::before{transform:translateX(20px)}

/* ─── BLE Keyless ─── */
.ble-card{
  background:linear-gradient(135deg,#0d1a2a 0%,#0a1520 100%);
  border:1px solid var(--border2);border-radius:var(--radius2);
  padding:20px;
}
.ble-device{
  display:flex;align-items:center;gap:12px;
  padding:10px 14px;background:var(--bg);
  border:1px solid var(--border);border-radius:var(--radius);
  margin-bottom:8px;
}
.ble-device .icon{font-size:1.3rem}
.ble-device .info{flex:1}
.ble-device .info .mac{
  font-family:'JetBrains Mono',monospace;font-size:.75rem;color:var(--dim);
}
.ble-device .rssi{
  font-family:'JetBrains Mono',monospace;font-size:.82rem;
  color:var(--info);font-weight:600;
}
.ble-device.paired{border-color:var(--accent)}
.ble-device.paired .icon{color:var(--accent)}

.btn{
  font-family:inherit;font-size:.82rem;font-weight:600;
  padding:10px 20px;border-radius:8px;border:1px solid var(--border);
  cursor:pointer;transition:.2s;background:var(--panel2);color:var(--text);
}
.btn:hover{background:var(--border);border-color:var(--border2)}
.btn.primary{
  background:var(--accent);color:#06090f;border-color:var(--accent);
}
.btn.primary:hover{background:var(--accent2)}
.btn.danger{background:var(--dangerDim);color:var(--danger);border-color:rgba(255,84,112,.3)}
.btn.danger:hover{background:rgba(255,84,112,.2)}
.btn:disabled{opacity:.4;cursor:not-allowed}

.btn-row{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}

/* ─── Keyless State ─── */
.keyless-status{
  display:flex;align-items:center;gap:12px;padding:14px;
  background:var(--bg);border:1px solid var(--border);
  border-radius:var(--radius);margin-top:12px;
}
.keyless-ring{
  width:48px;height:48px;border-radius:50%;
  border:3px solid var(--off);display:flex;
  align-items:center;justify-content:center;
  font-size:1.4rem;transition:.4s;flex-shrink:0;
}
.keyless-ring.active{
  border-color:var(--accent);
  box-shadow:0 0 20px rgba(0,230,138,.2);
  animation:pulse 2s ease infinite;
}
@keyframes pulse{
  0%,100%{box-shadow:0 0 20px rgba(0,230,138,.15)}
  50%{box-shadow:0 0 30px rgba(0,230,138,.3)}
}
.keyless-info{flex:1}
.keyless-info .state{font-weight:600;font-size:.95rem}
.keyless-info .detail{font-size:.72rem;color:var(--dim);margin-top:2px}

/* ─── Toast ─── */
.toast{
  position:fixed;bottom:20px;left:50%;transform:translateX(-50%) translateY(80px);
  background:var(--panel2);border:1px solid var(--border2);
  border-radius:var(--radius);padding:12px 24px;
  font-size:.85rem;font-weight:500;z-index:999;
  opacity:0;transition:.35s;pointer-events:none;
  box-shadow:0 10px 40px rgba(0,0,0,.4);
}
.toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
.toast.success{border-color:var(--accent);color:var(--accent)}
.toast.error{border-color:var(--danger);color:var(--danger)}

/* ─── Footer ─── */
.footer{
  text-align:center;padding:30px 20px;font-size:.72rem;color:var(--dim);
  font-family:'JetBrains Mono',monospace;
}
</style>
</head>
<body>

<!-- ═══ Header ═══ -->
<div class="header">
  <div class="logo">MOTO32<span>v2.1 Dashboard</span></div>
  <div class="header-right">
    <div class="conn-stack">
      <div class="conn-title">MOTO32</div>
      <div class="conn-badge">
        <span class="conn-kind">WEB</span>
        <div class="conn-dot" id="connDot"></div>
        <span id="connText">Connecting...</span>
      </div>
      <div class="conn-badge">
        <span class="conn-kind">BLE</span>
        <div class="conn-dot" id="bleDot"></div>
        <span id="bleText">Disconnected</span>
      </div>
    </div>
    <div class="mode-chip" id="modeChip">BASIC</div>
  </div>
</div>

<!-- ═══ Tabs ═══ -->
<div class="tabs">
  <button class="tab active" data-tab="diag" data-i18n="tab_diag"></button>
  <button class="tab" data-tab="settings" data-i18n="tab_settings"></button>
  <button class="tab" data-tab="ble" data-i18n="tab_ble"></button>
</div>
<div class="mode-banner" id="modeBanner">ADVANCED MODE ACTIVE</div>

<!-- ══════════════════════════════════════════════════ -->
<!-- TAB: DIAGNOSTICS                                   -->
<!-- ══════════════════════════════════════════════════ -->
<div class="tab-content active" id="tab-diag">

  <!-- Battery & System -->
  <div class="card">
    <div class="card-title" data-i18n="card_battery"></div>
    <div class="grid3" style="margin-bottom:12px">
      <div>
        <div style="font-size:.72rem;color:var(--muted);margin-bottom:4px" data-i18n="lbl_voltage"></div>
        <div id="batVolt" style="font-family:'JetBrains Mono',monospace;font-size:1.6rem;font-weight:700;color:var(--accent)">--.- V</div>
      </div>
      <div>
        <div style="font-size:.72rem;color:var(--muted);margin-bottom:4px" data-i18n="lbl_status"></div>
        <div id="errChips" style="display:flex;gap:6px;flex-wrap:wrap">
          <span class="err-chip ok">OK</span>
        </div>
      </div>
      <div>
        <div style="font-size:.72rem;color:var(--muted);margin-bottom:4px" data-i18n="lbl_engine"></div>
        <div id="engineState" style="font-family:'JetBrains Mono',monospace;font-size:1.1rem;font-weight:700;color:var(--off)">OFF</div>
      </div>
    </div>
  </div>

  <!-- Inputs -->
  <div class="card">
    <div class="card-title" data-i18n="card_inputs"></div>
    <div class="grid4" id="inputGrid"></div>
  </div>

  <!-- Outputs -->
  <div class="card">
    <div class="card-title" data-i18n="card_outputs"></div>
    <div style="font-size:.72rem;color:var(--dim);margin-bottom:10px" data-i18n="adv_outputs_hint"></div>
    <div class="grid4" id="outputGrid"></div>
  </div>

  <!-- Live Log -->
  <div class="card">
    <div class="card-title" data-i18n="card_log"></div>
    <div id="logBox" style="
      background:var(--bg);border:1px solid var(--border);border-radius:6px;
      padding:10px;height:140px;overflow-y:auto;
      font-family:'JetBrains Mono',monospace;font-size:.72rem;color:var(--dim);
      line-height:1.7;
    "></div>
  </div>
</div>

<!-- ══════════════════════════════════════════════════ -->
<!-- TAB: SETTINGS                                      -->
<!-- ══════════════════════════════════════════════════ -->
<div class="tab-content" id="tab-settings">

  <!-- Language -->
  <div class="card">
    <div class="card-title" data-i18n="card_language"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_language_name"></div>
        <div class="desc" data-i18n="set_language_desc"></div>
      </div>
      <div class="lang-switch">
        <button class="lang-btn active" data-lang="en" onclick="setLang('en')">EN</button>
        <button class="lang-btn" data-lang="de" onclick="setLang('de')">DE</button>
      </div>
    </div>
  </div>

  <!-- Operation Mode -->
  <div class="card">
    <div class="card-title" data-i18n="card_mode"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_mode_name"></div>
        <div class="desc" data-i18n="set_mode_desc"></div>
      </div>
      <div class="mode-switch">
        <button class="mode-btn active" id="btnModeBasic" data-i18n="mode_basic" onclick="setControlMode('basic')"></button>
        <button class="mode-btn" id="btnModeAdvanced" data-i18n="mode_advanced" onclick="setControlMode('advanced')"></button>
      </div>
    </div>
  </div>

  <!-- Handlebar -->
  <div class="card">
    <div class="card-title" data-i18n="card_handlebar"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_handlebar_name"></div>
        <div class="desc" data-i18n="set_handlebar_desc"></div>
      </div>
      <select id="s_handlebar"></select>
    </div>
  </div>

  <!-- Lights -->
  <div class="card">
    <div class="card-title" data-i18n="card_lights"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_rear_name"></div>
        <div class="desc" data-i18n="set_rear_desc"></div>
      </div>
      <select id="s_rear"></select>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_lowbeam_name"></div>
        <div class="desc" data-i18n="set_lowbeam_desc"></div>
      </div>
      <select id="s_lowBeam"></select>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_poslight_name"></div>
        <div class="desc" data-i18n="set_poslight_desc"></div>
      </div>
      <input type="number" id="s_posLight" min="0" max="9" value="0">
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_parking_name"></div>
        <div class="desc" data-i18n="set_parking_desc"></div>
      </div>
      <select id="s_parking"></select>
    </div>
  </div>

  <!-- Turn Signals -->
  <div class="card">
    <div class="card-title" data-i18n="card_turn"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_turnoff_name"></div>
        <div class="desc" data-i18n="set_turnoff_desc"></div>
      </div>
      <select id="s_turnMode"></select>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_turndist_name"></div>
        <div class="desc" data-i18n="set_turndist_desc"></div>
      </div>
      <input type="number" id="s_turnDist" min="10" max="1000" value="50">
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_turndist_cal_name"></div>
        <div class="desc" data-i18n="set_turndist_cal_desc"></div>
        <div id="turnCalStatus" style="font-size:.72rem;color:var(--dim);margin-top:4px"></div>
      </div>
      <button class="btn" id="btnTurnCal" onclick="toggleTurnDistanceCalibration()"></button>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_wave_name"></div>
        <div class="desc" data-i18n="set_wave_desc"></div>
      </div>
      <label class="toggle"><input type="checkbox" id="s_wave"><span class="slider"></span></label>
    </div>
  </div>

  <!-- Brake Light -->
  <div class="card">
    <div class="card-title" data-i18n="card_brake"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_brakemode_name"></div>
        <div class="desc" data-i18n="set_brakemode_desc"></div>
      </div>
      <select id="s_brakeMode"></select>
    </div>
  </div>

  <!-- Safety -->
  <div class="card">
    <div class="card-title" data-i18n="card_safety"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_standkill_name"></div>
        <div class="desc" data-i18n="set_standkill_desc"></div>
      </div>
      <select id="s_standKill"></select>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_alarm_name"></div>
        <div class="desc" data-i18n="set_alarm_desc"></div>
      </div>
      <select id="s_alarm"></select>
    </div>
  </div>

  <!-- AUX -->
  <div class="card">
    <div class="card-title" data-i18n="card_aux"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_aux1_name"></div>
        <div class="desc" data-i18n="set_aux1_desc"></div>
      </div>
      <select id="s_aux1"></select>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_aux2_name"></div>
        <div class="desc" data-i18n="set_aux2_desc"></div>
      </div>
      <select id="s_aux2"></select>
    </div>
  </div>

  <div class="btn-row">
    <button class="btn primary" onclick="saveSettings()" data-i18n="btn_save"></button>
    <button class="btn" onclick="loadSettings()" data-i18n="btn_reload"></button>
    <button class="btn danger" onclick="factoryReset()" data-i18n="btn_factory"></button>
  </div>
</div>

<!-- ══════════════════════════════════════════════════ -->
<!-- TAB: BLE KEYLESS                                   -->
<!-- ══════════════════════════════════════════════════ -->
<div class="tab-content" id="tab-ble">

  <div class="ble-card">
    <div class="card-title" data-i18n="card_keyless"></div>
    <p id="keylessDesc" style="font-size:.82rem;color:var(--muted);margin-bottom:16px;line-height:1.6" data-i18n="keyless_desc"></p>

    <div class="keyless-status" id="keylessStatus">
      <div class="keyless-ring" id="keylessRing">&#x1f512;</div>
      <div class="keyless-info">
        <div class="state" id="keylessState" data-i18n="keyless_inactive"></div>
        <div class="detail" id="keylessDetail" data-i18n="keyless_no_device"></div>
      </div>
    </div>
  </div>

  <!-- Paired Devices -->
  <div class="card" style="margin-top:16px">
    <div class="card-title" data-i18n="card_paired"></div>
    <div id="pairedDevices">
      <div style="color:var(--dim);font-size:.82rem;padding:12px 0" data-i18n="no_paired"></div>
    </div>
    <div class="btn-row">
      <button class="btn primary" id="btnScan" onclick="startScan()" data-i18n="btn_scan"></button>
    </div>
  </div>

  <!-- Scan Results -->
  <div class="card" style="margin-top:16px;display:none" id="scanCard">
    <div class="card-title" data-i18n="card_found"></div>
    <div id="scanResults"></div>
    <div class="btn-row">
      <button class="btn" onclick="stopScan()" data-i18n="btn_stopscan"></button>
    </div>
  </div>

  <!-- Keyless Settings -->
  <div class="card" style="margin-top:16px">
    <div class="card-title" data-i18n="card_keyless_settings"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_keyless_en_name"></div>
        <div class="desc" data-i18n="set_keyless_en_desc"></div>
      </div>
      <label class="toggle"><input type="checkbox" id="s_keylessEn" onchange="sendKeylessConfig()"><span class="slider"></span></label>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_rssi_name"></div>
        <div class="desc" data-i18n="set_rssi_desc"></div>
      </div>
      <input type="number" id="s_rssiThresh" value="-65" min="-90" max="-30" onchange="sendKeylessConfig()">
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_grace_name"></div>
        <div class="desc" data-i18n="set_grace_desc"></div>
      </div>
      <input type="number" id="s_graceTime" value="10" min="5" max="60" onchange="sendKeylessConfig()">
    </div>
  </div>
</div>

<!-- ═══ Toast ═══ -->
<div class="toast" id="toast"></div>

<div class="footer">MOTO32 OPEN SOURCE &middot; ESP32-WROOM-32D &middot; MIT LICENSE</div>

<!-- ══════════════════════════════════════════════════ -->
<!-- JAVASCRIPT                                         -->
<!-- ══════════════════════════════════════════════════ -->
<script>
// ═══════════════════════════════════════════════════════
// I18N – LANGUAGE SYSTEM
// ═══════════════════════════════════════════════════════

const LANG = {
  en: {
    // Tabs
    tab_diag: '\u26a1 Diagnostics',
    tab_settings: '\u2699 Settings',
    tab_ble: '\ud83d\udd11 BLE Keyless',
    mode_basic: 'BASIC',
    mode_advanced: 'ADVANCED',
    mode_banner: 'ADVANCED MODE ACTIVE',

    // Connection
    conn_connecting: 'Connecting\u2026',
    conn_connected: 'Connected',
    conn_disconnected: 'Disconnected',
    conn_lost: 'Connection lost',
    ws_connected: 'WebSocket connected',
    ble_connected: 'Connected',
    ble_disconnected: 'Disconnected',

    // Diagnostics
    card_battery: 'Battery & System',
    lbl_voltage: 'Voltage',
    lbl_status: 'Status',
    lbl_engine: 'Engine',
    card_inputs: 'Inputs',
    card_outputs: 'Outputs',
    adv_outputs_hint: 'In ADVANCED mode, tap an output card to latch that output on/off.',
    adv_tap_toggle: 'Tap to toggle',
    card_log: 'Event Log',

    // Engine states
    eng_off: 'OFF',
    eng_running: 'RUNNING',
    eng_starter: 'STARTER',
    eng_ignition: 'IGNITION',

    // Errors
    err_ok: '\u2713 All OK',
    err_undervolt: 'Undervoltage',
    err_overvolt: 'Overvoltage',
    err_starter_timeout: 'Starter Timeout',
    err_watchdog: 'Watchdog Reset',
    err_sidestand: 'Sidestand',

    // I/O labels
    io_active: 'Active',
    io_inactive: 'Inactive',

    // Input names
    in_lock: 'Ignition Lock',
    in_turnL: 'Turn L',
    in_turnR: 'Turn R',
    in_light: 'Light',
    in_start: 'Starter',
    in_horn: 'Horn',
    in_brake: 'Brake',
    in_kill: 'Kill Switch',
    in_stand: 'Sidestand',
    in_aux1: 'AUX 1 In',
    in_aux2: 'AUX 2 In',
    in_speed: 'Speed Pulse',

    // Output names
    out_turnLOut: 'Turn L',
    out_turnROut: 'Turn R',
    out_lightOut: 'Low Beam',
    out_hibeam: 'High Beam',
    out_brakeOut: 'Brake Light',
    out_hornOut: 'Horn',
    out_start1: 'Starter 1',
    out_start2: 'Starter 2',
    out_ignOut: 'Ignition',
    out_aux1Out: 'AUX 1',
    out_aux2Out: 'AUX 2',

    // Settings - Handlebar
    card_language: 'Language',
    set_language_name: 'Dashboard Language',
    set_language_desc: 'Switch UI language for all labels and messages.',

    // Settings - Handlebar
    card_mode: 'Operation Mode',
    set_mode_name: 'Control Level',
    set_mode_desc: 'BASIC = safe display. ADVANCED = direct output toggling.',

    // Settings - Handlebar
    card_handlebar: 'Handlebar Configuration',
    set_handlebar_name: 'Switch Unit',
    set_handlebar_desc: 'Type of handlebar switch unit',
    opt_hbar_a: 'A \u2013 5 Pushbuttons',
    opt_hbar_b: 'B \u2013 Harley / BMW',
    opt_hbar_c: 'C \u2013 Japanese / European',
    opt_hbar_d: 'D \u2013 Ducati',
    opt_hbar_e: 'E \u2013 4 Pushbuttons',

    // Settings - Lights
    card_lights: 'Lighting',
    set_rear_name: 'Tail Light Mode',
    set_rear_desc: 'Tail light behavior',
    opt_rear_0: 'Standard',
    opt_rear_1: 'Dimmed',
    opt_rear_2: 'Always On',
    set_lowbeam_name: 'Low Beam Mode',
    set_lowbeam_desc: 'When the low beam turns on',
    opt_low_0: 'On at engine start',
    opt_low_1: 'Manual',
    opt_low_2: 'Daytime running light (always on)',
    set_poslight_name: 'Position Light',
    set_poslight_desc: 'Brightness 0 (off) to 9 (50%)',
    set_parking_name: 'Parking Light',
    set_parking_desc: 'Lights when ignition is off',
    opt_park_0: 'Off',
    opt_park_1: 'Position light',
    opt_park_2: 'Left indicator',
    opt_park_3: 'Right indicator',

    // Settings - Turn Signals
    card_turn: 'Turn Signals',
    set_turnoff_name: 'Auto Cancel',
    set_turnoff_desc: 'Automatically cancel turn signal after\u2026',
    opt_turn_0: 'Disabled',
    opt_turn_1: 'Distance (~50m)',
    opt_turn_2: '10 Seconds',
    opt_turn_3: '20 Seconds',
    opt_turn_4: '30 Seconds',
    set_turndist_name: 'Distance Pulses',
    set_turndist_desc: 'Speed sensor pulses for distance-based cancel',
    set_turndist_cal_name: 'Distance Calibration',
    set_turndist_cal_desc: 'Press start, push bike 10m, then confirm.',
    btn_turndist_cal_start: 'Start 10m',
    btn_turndist_cal_confirm: 'Confirm 10m',
    turn_cal_idle: 'Ready for 10m calibration.',
    turn_cal_running: 'Pulses captured: {pulses} (target preview: {target})',
    turn_cal_done: 'Calibration applied.',
    turn_cal_failed: 'No pulses detected. Try again.',
    set_wave_name: 'mo.wave Animation',
    set_wave_desc: 'Sequential turn signal effect',

    // Settings - Brake
    card_brake: 'Brake Light',
    set_brakemode_name: 'Brake Light Mode',
    set_brakemode_desc: 'Behavior when braking',
    opt_brake_0: 'Continuous',
    opt_brake_1: 'Pulsing (PWM fade)',
    opt_brake_2: 'Flash 5Hz',
    opt_brake_3: '8x Flash \u2192 Continuous',
    opt_brake_4: '2x Flash \u2192 Continuous',
    opt_brake_5: '3s Continuous \u2192 Flash',
    opt_brake_6: 'Emergency braking (hazard)',

    // Settings - Safety
    card_safety: 'Safety',
    set_standkill_name: 'Sidestand / Kill Switch',
    set_standkill_desc: 'Combined setting: tens = stand, ones = kill',
    opt_sk_0: 'Stand active (N/O) + Kill (N/O)',
    opt_sk_1: 'Stand active (N/O) + Kill (N/C)',
    opt_sk_2: 'Stand active (N/O) + Kill disabled',
    opt_sk_3: 'Stand active (N/C) + Kill (N/O)',
    opt_sk_4: 'Stand active (N/C) + Kill (N/C)',
    opt_sk_5: 'Stand active (N/C) + Kill disabled',
    opt_sk_6: 'Stand disabled + Kill (N/O)',
    opt_sk_7: 'Stand disabled + Kill (N/C)',
    opt_sk_8: 'Stand disabled + Kill disabled',
    set_alarm_name: 'Alarm',
    set_alarm_desc: 'Vibration sensor alarm in standby',
    opt_alarm_0: 'Disabled',
    opt_alarm_1: 'Enabled',

    // Settings - AUX
    card_aux: 'Auxiliary Outputs',
    set_aux1_name: 'AUX 1 Mode',
    set_aux1_desc: 'Behavior of auxiliary output 1',
    set_aux2_name: 'AUX 2 Mode',
    set_aux2_desc: 'Behavior of auxiliary output 2',
    opt_aux_0: 'On with ignition',
    opt_aux_1: 'On with engine',
    opt_aux_2: 'Manual',
    opt_aux_3: 'Disabled',

    // Buttons
    btn_save: '\ud83d\udcbe Save Settings',
    btn_reload: '\ud83d\udd04 Reload',
    btn_factory: '\u26a0 Factory Reset',

    // BLE Keyless
    card_keyless: 'BLE Keyless Ignition',
    keyless_desc: 'Pair a smartphone via BLE with the Moto32. When the paired phone is in range, the ignition is automatically activated. It stays on until the engine has run and been shut off. Within 10\u00a0seconds you can restart. After that, the phone must be detected again.',
    keyless_inactive: 'Inactive',
    keyless_no_device: 'No device paired',
    keyless_disabled: 'Disabled',
    keyless_disabled_detail: 'Keyless function is turned off',
    keyless_unlocked: 'Unlocked',
    keyless_unlocked_detail: 'Device detected ({rssi} dBm) \u2013 ignition granted',
    keyless_grace: 'Grace Period',
    keyless_grace_detail: '{sec}s remaining for restart',
    keyless_locked: 'Locked',
    keyless_locked_detail: 'Waiting for phone detection\u2026',

    card_paired: 'Paired Devices',
    no_paired: 'No devices paired',
    btn_scan: '\ud83d\udce1 Scan for new device\u2026',
    card_found: 'Discovered Devices',
    btn_stopscan: 'Stop Scan',
    btn_pair: 'Pair',
    unknown: 'Unknown',
    confirm_remove: 'Remove device?',
    scanning: 'Scanning\u2026',
    no_devices_found: 'No devices found',
    pairing_device: 'Pairing device\u2026',

    card_keyless_settings: 'Keyless Settings',
    set_keyless_en_name: 'Keyless Enabled',
    set_keyless_en_desc: 'BLE proximity detection for ignition',
    set_rssi_name: 'RSSI Threshold',
    set_rssi_desc: 'Signal strength for detection (typ. -70 to -50 dBm)',
    set_grace_name: 'Grace Period',
    set_grace_desc: 'Seconds after engine off during which restart is possible',

    // Toast messages
    toast_saved: 'Settings saved',
    toast_factory: 'Factory defaults restored',
    confirm_factory: 'Reset all settings to factory defaults?',
    confirm_advanced: 'Warning: ADVANCED mode can force outputs manually and may bypass normal safety behavior. Continue?',
    toast_adv_required: 'Switch to ADVANCED mode first.',
    toast_adv_enabled: 'ADVANCED mode enabled',
    toast_adv_disabled: 'Back to BASIC mode',
    ble_scan_request: 'BLE scan requested',
    ble_scan_stop: 'BLE scan stopped',
    ble_scan_waiting: 'Waiting for scan results…',
    ble_scan_debug: 'Open browser console for BLE debug details.',
  },

  de: {
    tab_diag: '\u26a1 Diagnose',
    tab_settings: '\u2699 Einstellungen',
    tab_ble: '\ud83d\udd11 BLE Keyless',
    mode_basic: 'BASIC',
    mode_advanced: 'ADVANCED',
    mode_banner: 'ADVANCED MODUS AKTIV',

    conn_connecting: 'Verbinde\u2026',
    conn_connected: 'Verbunden',
    conn_disconnected: 'Getrennt',
    conn_lost: 'Verbindung verloren',
    ws_connected: 'WebSocket verbunden',
    ble_connected: 'Verbunden',
    ble_disconnected: 'Getrennt',

    card_battery: 'Batterie & System',
    lbl_voltage: 'Spannung',
    lbl_status: 'Status',
    lbl_engine: 'Motor',
    card_inputs: 'Eing\u00e4nge',
    card_outputs: 'Ausg\u00e4nge',
    adv_outputs_hint: 'Im ADVANCED-Modus schaltet ein Tipp auf eine Ausgangskarte den Ausgang dauerhaft ein/aus.',
    adv_tap_toggle: 'Tippen zum Schalten',
    card_log: 'Event Log',

    eng_off: 'AUS',
    eng_running: 'L\u00c4UFT',
    eng_starter: 'STARTER',
    eng_ignition: 'Z\u00dcNDUNG',

    err_ok: '\u2713 Alles OK',
    err_undervolt: 'Unterspannung',
    err_overvolt: '\u00dcberspannung',
    err_starter_timeout: 'Starter Timeout',
    err_watchdog: 'Watchdog Reset',
    err_sidestand: 'Seitenst\u00e4nder',

    io_active: 'Aktiv',
    io_inactive: 'Inaktiv',

    in_lock: 'Z\u00fcndschloss',
    in_turnL: 'Blinker L',
    in_turnR: 'Blinker R',
    in_light: 'Licht',
    in_start: 'Starter',
    in_horn: 'Hupe',
    in_brake: 'Bremse',
    in_kill: 'Kill-Switch',
    in_stand: 'Seitenst\u00e4nder',
    in_aux1: 'AUX 1 In',
    in_aux2: 'AUX 2 In',
    in_speed: 'Tacho-Puls',

    out_turnLOut: 'Blinker L',
    out_turnROut: 'Blinker R',
    out_lightOut: 'Abblendlicht',
    out_hibeam: 'Fernlicht',
    out_brakeOut: 'Bremslicht',
    out_hornOut: 'Hupe',
    out_start1: 'Starter 1',
    out_start2: 'Starter 2',
    out_ignOut: 'Z\u00fcndung',
    out_aux1Out: 'AUX 1',
    out_aux2Out: 'AUX 2',

    card_language: 'Sprache',
    set_language_name: 'Dashboard-Sprache',
    set_language_desc: 'Stellt die Sprache aller Labels und Meldungen um.',

    card_mode: 'Betriebsmodus',
    set_mode_name: 'Steuerstufe',
    set_mode_desc: 'BASIC = sichere Anzeige. ADVANCED = direkte Ausgangssteuerung.',

    card_handlebar: 'Lenker-Konfiguration',
    set_handlebar_name: 'Lenkerarmatur',
    set_handlebar_desc: 'Art der Schaltereinheit am Lenker',
    opt_hbar_a: 'A \u2013 5 Taster',
    opt_hbar_b: 'B \u2013 Harley / BMW',
    opt_hbar_c: 'C \u2013 Japan / Europa',
    opt_hbar_d: 'D \u2013 Ducati',
    opt_hbar_e: 'E \u2013 4 Taster',

    card_lights: 'Beleuchtung',
    set_rear_name: 'R\u00fccklicht-Modus',
    set_rear_desc: 'Verhalten des R\u00fccklichts',
    opt_rear_0: 'Standard',
    opt_rear_1: 'Gedimmt',
    opt_rear_2: 'Immer an',
    set_lowbeam_name: 'Abblendlicht-Modus',
    set_lowbeam_desc: 'Wann schaltet das Abblendlicht ein',
    opt_low_0: 'An bei Motorstart',
    opt_low_1: 'Manuell',
    opt_low_2: 'Tagfahrlicht (immer an)',
    set_poslight_name: 'Positionslicht',
    set_poslight_desc: 'Helligkeit 0 (aus) bis 9 (50%)',
    set_parking_name: 'Parkbeleuchtung',
    set_parking_desc: 'Licht bei ausgeschalteter Z\u00fcndung',
    opt_park_0: 'Aus',
    opt_park_1: 'Standlicht',
    opt_park_2: 'Blinker links',
    opt_park_3: 'Blinker rechts',

    card_turn: 'Blinker',
    set_turnoff_name: 'Auto-Abschaltung',
    set_turnoff_desc: 'Blinker automatisch ausschalten nach\u2026',
    opt_turn_0: 'Deaktiviert',
    opt_turn_1: 'Strecke (~50m)',
    opt_turn_2: '10 Sekunden',
    opt_turn_3: '20 Sekunden',
    opt_turn_4: '30 Sekunden',
    set_turndist_name: 'Strecken-Impulse',
    set_turndist_desc: 'Pulse vom Geschwindigkeitssensor f\u00fcr Streckenabschaltung',
    set_turndist_cal_name: 'Distanz-Kalibrierung',
    set_turndist_cal_desc: 'Start dr\u00fccken, Motorrad 10m schieben, dann best\u00e4tigen.',
    btn_turndist_cal_start: 'Start 10m',
    btn_turndist_cal_confirm: '10m best\u00e4tigen',
    turn_cal_idle: 'Bereit f\u00fcr 10m-Kalibrierung.',
    turn_cal_running: 'Erfasste Pulse: {pulses} (Zielvorschau: {target})',
    turn_cal_done: 'Kalibrierung \u00fcbernommen.',
    turn_cal_failed: 'Keine Pulse erkannt. Bitte erneut versuchen.',
    set_wave_name: 'mo.wave Animation',
    set_wave_desc: 'Laufender Blinker-Effekt',

    card_brake: 'Bremslicht',
    set_brakemode_name: 'Bremslicht-Modus',
    set_brakemode_desc: 'Verhalten beim Bremsen',
    opt_brake_0: 'Dauerlicht',
    opt_brake_1: 'Pulsierend (PWM-Fade)',
    opt_brake_2: 'Blitzen 5Hz',
    opt_brake_3: '8\u00d7 Blitzen \u2192 Dauer',
    opt_brake_4: '2\u00d7 Blitzen \u2192 Dauer',
    opt_brake_5: '3s Dauer \u2192 Blitzen',
    opt_brake_6: 'Notbremsung (Warnblinker)',

    card_safety: 'Sicherheit',
    set_standkill_name: 'Seitenst\u00e4nder / Kill-Switch',
    set_standkill_desc: 'Kombinierte Einstellung: Zehner = St\u00e4nder, Einer = Kill',
    opt_sk_0: 'St\u00e4nder aktiv (N/O) + Kill (N/O)',
    opt_sk_1: 'St\u00e4nder aktiv (N/O) + Kill (N/C)',
    opt_sk_2: 'St\u00e4nder aktiv (N/O) + Kill deaktiviert',
    opt_sk_3: 'St\u00e4nder aktiv (N/C) + Kill (N/O)',
    opt_sk_4: 'St\u00e4nder aktiv (N/C) + Kill (N/C)',
    opt_sk_5: 'St\u00e4nder aktiv (N/C) + Kill deaktiviert',
    opt_sk_6: 'St\u00e4nder deaktiviert + Kill (N/O)',
    opt_sk_7: 'St\u00e4nder deaktiviert + Kill (N/C)',
    opt_sk_8: 'St\u00e4nder deaktiviert + Kill deaktiviert',
    set_alarm_name: 'Alarmanlage',
    set_alarm_desc: 'Ersch\u00fctterungssensor-Alarm im Standby',
    opt_alarm_0: 'Deaktiviert',
    opt_alarm_1: 'Aktiviert',

    card_aux: 'Zusatzausg\u00e4nge',
    set_aux1_name: 'AUX 1 Modus',
    set_aux1_desc: 'Verhalten des Zusatzausgangs 1',
    set_aux2_name: 'AUX 2 Modus',
    set_aux2_desc: 'Verhalten des Zusatzausgangs 2',
    opt_aux_0: 'An bei Z\u00fcndung',
    opt_aux_1: 'An bei Motor',
    opt_aux_2: 'Manuell',
    opt_aux_3: 'Deaktiviert',

    btn_save: '\ud83d\udcbe Einstellungen speichern',
    btn_reload: '\ud83d\udd04 Neu laden',
    btn_factory: '\u26a0 Werkseinstellungen',

    card_keyless: 'BLE Keyless-Z\u00fcndung',
    keyless_desc: 'Verbinde ein Smartphone per BLE mit der Moto32. Wenn das angelernte Handy in Reichweite ist, wird die Z\u00fcndung automatisch aktiviert. Sie bleibt an, bis der Motor einmal lief und wieder abgestellt wurde. Innerhalb von 10\u00a0Sekunden kann erneut gestartet werden. Danach muss das Handy wieder erkannt werden.',
    keyless_inactive: 'Inaktiv',
    keyless_no_device: 'Kein Ger\u00e4t angelernt',
    keyless_disabled: 'Deaktiviert',
    keyless_disabled_detail: 'Keyless-Funktion ist ausgeschaltet',
    keyless_unlocked: 'Entsperrt',
    keyless_unlocked_detail: 'Ger\u00e4t erkannt ({rssi} dBm) \u2013 Z\u00fcndung freigegeben',
    keyless_grace: 'Nachlauf',
    keyless_grace_detail: '{sec}s verbleibend f\u00fcr Neustart',
    keyless_locked: 'Gesperrt',
    keyless_locked_detail: 'Warte auf Handy-Erkennung\u2026',

    card_paired: 'Angelernte Ger\u00e4te',
    no_paired: 'Keine Ger\u00e4te angelernt',
    btn_scan: '\ud83d\udce1 Neues Ger\u00e4t suchen\u2026',
    card_found: 'Gefundene Ger\u00e4te',
    btn_stopscan: 'Suche beenden',
    btn_pair: 'Anlernen',
    unknown: 'Unbekannt',
    confirm_remove: 'Ger\u00e4t entfernen?',
    scanning: 'Suche l\u00e4uft\u2026',
    no_devices_found: 'Keine Ger\u00e4te gefunden',
    pairing_device: 'Ger\u00e4t wird angelernt\u2026',

    card_keyless_settings: 'Keyless-Einstellungen',
    set_keyless_en_name: 'Keyless aktiviert',
    set_keyless_en_desc: 'BLE-Ann\u00e4herungserkennung f\u00fcr Z\u00fcndung',
    set_rssi_name: 'RSSI Schwelle',
    set_rssi_desc: 'Signalst\u00e4rke f\u00fcr Erkennung (typ. -70 bis -50 dBm)',
    set_grace_name: 'Nachlaufzeit',
    set_grace_desc: 'Sekunden nach Motorabschaltung, in denen Neustart m\u00f6glich ist',

    toast_saved: 'Einstellungen gespeichert',
    toast_factory: 'Werkseinstellungen wiederhergestellt',
    confirm_factory: 'Alle Einstellungen auf Werkseinstellungen zur\u00fccksetzen?',
    confirm_advanced: 'Warnung: Im ADVANCED-Modus k\u00f6nnen Ausg\u00e4nge manuell erzwungen werden und Sicherheitsfunktionen umgangen werden. Fortfahren?',
    toast_adv_required: 'Bitte zuerst auf ADVANCED umschalten.',
    toast_adv_enabled: 'ADVANCED-Modus aktiviert',
    toast_adv_disabled: 'Zur\u00fcck im BASIC-Modus',
    ble_scan_request: 'BLE-Scan angefordert',
    ble_scan_stop: 'BLE-Scan gestoppt',
    ble_scan_waiting: 'Warte auf Scan-Ergebnisse…',
    ble_scan_debug: 'F\u00fcr BLE-Debug bitte Browser-Konsole \u00f6ffnen.',
  }
};

let currentLang = localStorage.getItem('moto32_lang') || 'en';

function t(key, params) {
  let str = (LANG[currentLang] && LANG[currentLang][key]) || LANG.en[key] || key;
  if (params) {
    Object.keys(params).forEach(k => { str = str.replace('{'+k+'}', params[k]); });
  }
  return str;
}

function setLang(lang) {
  currentLang = lang;
  localStorage.setItem('moto32_lang', lang);
  document.querySelectorAll('.lang-btn').forEach(b => {
    b.classList.toggle('active', b.dataset.lang === lang);
  });
  document.documentElement.lang = lang;
  applyI18n();
  populateSelects();
  initTiles();
  if (lastDiagState) updateDiag(lastDiagState);
}

function applyI18n() {
  document.querySelectorAll('[data-i18n]').forEach(el => {
    el.textContent = t(el.dataset.i18n);
  });
  updateModeUi();
  if ($('connText')) $('connText').textContent = connected ? t('conn_connected') : t('conn_disconnected');
  updateBleConnectionUi(lastDiagState ? !!lastDiagState.bleConnected : false);
  updateTurnDistanceCalibrationUi();
}

// ─── Select population (language-aware) ───
function populateSelects() {
  const fill = (id, opts) => {
    const sel = document.getElementById(id);
    const cur = sel.value;
    sel.innerHTML = opts.map((o,i) => `<option value="${i}">${o}</option>`).join('');
    if (cur !== '') sel.value = cur;
  };
  fill('s_handlebar', [t('opt_hbar_a'),t('opt_hbar_b'),t('opt_hbar_c'),t('opt_hbar_d'),t('opt_hbar_e')]);
  fill('s_rear', [t('opt_rear_0'),t('opt_rear_1'),t('opt_rear_2')]);
  fill('s_lowBeam', [t('opt_low_0'),t('opt_low_1'),t('opt_low_2')]);
  fill('s_parking', [t('opt_park_0'),t('opt_park_1'),t('opt_park_2'),t('opt_park_3')]);
  fill('s_turnMode', [t('opt_turn_0'),t('opt_turn_1'),t('opt_turn_2'),t('opt_turn_3'),t('opt_turn_4')]);
  fill('s_brakeMode', [t('opt_brake_0'),t('opt_brake_1'),t('opt_brake_2'),t('opt_brake_3'),t('opt_brake_4'),t('opt_brake_5'),t('opt_brake_6')]);

  const skOpts = [];
  for (let i = 0; i <= 8; i++) skOpts.push(t('opt_sk_'+i));
  fill('s_standKill', skOpts);
  fill('s_alarm', [t('opt_alarm_0'),t('opt_alarm_1')]);
  fill('s_aux1', [t('opt_aux_0'),t('opt_aux_1'),t('opt_aux_2'),t('opt_aux_3')]);
  fill('s_aux2', [t('opt_aux_0'),t('opt_aux_1'),t('opt_aux_2'),t('opt_aux_3')]);
}

// ═══════════════════════════════════════════════════════
// CONFIG
// ═══════════════════════════════════════════════════════

const INPUT_PINS = {lock:46,turnL:47,turnR:48,light:21,start:22,horn:23,brake:1,kill:2,stand:3,aux1:4,aux2:5,speed:6};
const INPUT_IDS = ['lock','turnL','turnR','light','start','horn','brake','kill','stand','aux1','aux2','speed'];
const OUTPUT_PINS = {turnLOut:9,turnROut:10,lightOut:11,hibeam:12,brakeOut:13,hornOut:41,start1:44,start2:45,ignOut:42,aux1Out:43,aux2Out:40};
const OUTPUT_IDS = ['turnLOut','turnROut','lightOut','hibeam','brakeOut','hornOut','start1','start2','ignOut','aux1Out','aux2Out'];

// ─── State ───
let ws = null;
let connected = false;
let reconnTimer = null;
let logLines = [];
let advancedMode = false;
let lastDiagState = null;
let toastTimer = null;
let scanWatchdogTimer = null;
let lastBleScanSignature = '';
let currentSpeedPulses = 0;
let turnDistanceCalibrationActive = false;
let turnDistanceCalibrationStartPulses = 0;

// ─── DOM refs ───
const $ = id => document.getElementById(id);

function bleDebug(message, payload){
  if (payload === undefined) console.debug('[BLE]', message);
  else console.debug('[BLE]', message, payload);
}

function updateModeUi(){
  document.body.classList.toggle('advanced-mode', advancedMode);
  if ($('modeChip')) $('modeChip').textContent = advancedMode ? t('mode_advanced') : t('mode_basic');
  if ($('modeBanner')) $('modeBanner').textContent = t('mode_banner');
  if ($('btnModeBasic')) $('btnModeBasic').classList.toggle('active', !advancedMode);
  if ($('btnModeAdvanced')) $('btnModeAdvanced').classList.toggle('active', advancedMode);

  document.querySelectorAll('#outputGrid .io-tile').forEach(tile => {
    tile.classList.add('clickable');
    tile.classList.toggle('locked', !advancedMode);
  });
}

function updateBleConnectionUi(isConnected){
  const dot = $('bleDot');
  const text = $('bleText');
  if (!dot || !text) return;
  dot.classList.toggle('ok', !!isConnected);
  text.textContent = isConnected ? t('ble_connected') : t('ble_disconnected');
}

function updateTurnDistanceCalibrationUi(){
  const btn = $('btnTurnCal');
  const status = $('turnCalStatus');
  if (!btn || !status) return;

  if (!turnDistanceCalibrationActive) {
    btn.textContent = t('btn_turndist_cal_start');
    status.textContent = t('turn_cal_idle');
    return;
  }

  const captured = Math.max(0, currentSpeedPulses - turnDistanceCalibrationStartPulses);
  const target = Math.max(10, Math.min(1000, captured * 5));
  btn.textContent = t('btn_turndist_cal_confirm');
  status.textContent = t('turn_cal_running', {pulses: captured, target});
}

function setControlMode(mode, opts = {}){
  const {skipConfirm = false, silent = false, force = false} = opts;
  if (!force && ((mode === 'advanced') === advancedMode)) return;
  if (mode === 'advanced' && !skipConfirm) {
    if (!confirm(t('confirm_advanced'))) {
      updateModeUi();
      return;
    }
  }
  advancedMode = mode === 'advanced';
  updateModeUi();
  if (!silent) {
    toast(t(advancedMode ? 'toast_adv_enabled' : 'toast_adv_disabled'),
      advancedMode ? 'error' : 'success');
  }
}

function handleOutputTileClick(ev){
  const tile = ev.currentTarget;
  const id = tile.dataset.outputId;
  if (!id) return;
  if (!advancedMode) {
    toast(t('toast_adv_required'), 'error', 4200);
    return;
  }
  bleDebug('toggle output requested', {id});
  wsSend({cmd:'toggleOutput', id});
}

// ─── Init Tiles ───
function initTiles(){
  const ig = $('inputGrid');
  ig.innerHTML = INPUT_IDS.map(id => `
    <div class="io-tile" id="in_${id}">
      <div class="name"><span class="status-dot"></span>${t('in_'+id)}<span class="pin">GPIO ${INPUT_PINS[id]}</span></div>
      <div class="val">OFF</div>
      <div class="sub">\u2013</div>
    </div>`).join('');
  const og = $('outputGrid');
  og.innerHTML = OUTPUT_IDS.map(id => `
    <div class="io-tile clickable locked" id="out_${id}" data-output-id="${id}">
      <div class="name"><span class="status-dot"></span>${t('out_'+id)}<span class="pin">GPIO ${OUTPUT_PINS[id]}</span></div>
      <div class="val">OFF</div>
      <div class="sub">\u2013</div>
    </div>`).join('');
  document.querySelectorAll('#outputGrid .io-tile').forEach(tile => {
    tile.addEventListener('click', handleOutputTileClick);
  });
  updateModeUi();
}

// ─── WebSocket ───
function wsConnect(){
  if(ws && ws.readyState < 2) return;
  const host = location.hostname || '192.168.4.1';
  ws = new WebSocket(`ws://${host}/ws`);
  ws.binaryType = 'arraybuffer';
  ws.onopen = () => {
    connected = true;
    setControlMode('basic', {silent:true, force:true});
    $('connDot').classList.add('ok');
    $('connText').textContent = t('conn_connected');
    addLog(t('ws_connected'));
    bleDebug('websocket connected');
    ws.send(JSON.stringify({cmd:'getSettings'}));
    ws.send(JSON.stringify({cmd:'getKeyless'}));
  };
  ws.onclose = () => {
    connected = false;
    $('connDot').classList.remove('ok');
    $('connText').textContent = t('conn_disconnected');
    updateBleConnectionUi(false);
    addLog(t('conn_lost'));
    clearTimeout(reconnTimer);
    reconnTimer = setTimeout(wsConnect, 2000);
  };
  ws.onerror = () => ws.close();
  ws.onmessage = e => {
    try{
      const msg = JSON.parse(e.data);
      handleMsg(msg);
    }catch(ex){
      console.warn('WS parse error', ex);
    }
  };
}

function wsSend(obj){ if(ws&&ws.readyState===1) ws.send(JSON.stringify(obj)); }

// ─── Message Handler ───
function handleMsg(m){
  if(m.type==='state')    updateDiag(m);
  if(m.type==='settings') applySettings(m);
  if(m.type==='keyless')  updateKeyless(m);
  if(m.type==='scan')     updateScan(m);
  if(m.type==='toast')    toast(m.text, m.level||'success');
  if(m.type==='log')      addLog(m.text);
}

// ─── Update Diagnostics ───
function updateDiag(s){
  lastDiagState = s;
  currentSpeedPulses = Number(s.speedPulses || 0);
  updateBleConnectionUi(!!s.bleConnected);
  if (typeof s.turnCalActive === 'boolean') {
    turnDistanceCalibrationActive = s.turnCalActive;
    if (turnDistanceCalibrationActive && typeof s.turnCalPulses === 'number') {
      turnDistanceCalibrationStartPulses =
          Math.max(0, currentSpeedPulses - Number(s.turnCalPulses));
    }
    updateTurnDistanceCalibrationUi();
  }

  // Battery
  const v = s.voltage || 0;
  $('batVolt').textContent = v.toFixed(1) + ' V';

  // Engine
  const eng = $('engineState');
  if(s.engineRunning){ eng.textContent=t('eng_running'); eng.style.color='var(--on)'; }
  else if(s.starterEngaged){ eng.textContent=t('eng_starter'); eng.style.color='var(--warn)'; }
  else if(s.ignitionOn){ eng.textContent=t('eng_ignition'); eng.style.color='var(--info)'; }
  else{ eng.textContent=t('eng_off'); eng.style.color='var(--off)'; }

  // Errors
  const errs = [];
  const ef = s.errorFlags||0;
  if(ef===0) errs.push('<span class="err-chip ok">'+t('err_ok')+'</span>');
  if(ef&1)  errs.push('<span class="err-chip err">'+t('err_undervolt')+'</span>');
  if(ef&2)  errs.push('<span class="err-chip err">'+t('err_overvolt')+'</span>');
  if(ef&4)  errs.push('<span class="err-chip warn">'+t('err_starter_timeout')+'</span>');
  if(ef&8)  errs.push('<span class="err-chip warn">'+t('err_watchdog')+'</span>');
  if(ef&16) errs.push('<span class="err-chip warn">'+t('err_sidestand')+'</span>');
  $('errChips').innerHTML = errs.join('');

  // Inputs
  const ins = s.inputs||{};
  INPUT_IDS.forEach(id => {
    const tile = $('in_'+id);
    if(!tile) return;
    const active = !!ins[id];
    tile.classList.toggle('active', active);
    tile.querySelector('.val').textContent = active?'ON':'OFF';
    const extra = ins[id+'_info'];
    if(extra) tile.querySelector('.sub').textContent = extra;
    else tile.querySelector('.sub').textContent = active?t('io_active'):t('io_inactive');
  });

  // Outputs
  const outs = s.outputs||{};
  OUTPUT_IDS.forEach(id => {
    const tile = $('out_'+id);
    if(!tile) return;
    const active = !!outs[id];
    tile.classList.toggle('active', active);
    tile.querySelector('.val').textContent = active?'ON':'OFF';
    const pwm = outs[id+'_pwm'];
    if(pwm!==undefined && pwm>0) tile.querySelector('.sub').textContent = 'PWM '+Math.round(pwm/255*100)+'%';
    else if (advancedMode) tile.querySelector('.sub').textContent = t('adv_tap_toggle');
    else tile.querySelector('.sub').textContent = active?'\u26a1 '+t('io_active'):t('io_inactive');
  });
}

// ─── Settings ───
function applySettings(s){
  $('s_handlebar').value = s.handlebar??0;
  $('s_rear').value      = s.rear??0;
  $('s_turnMode').value  = s.turn??2;
  $('s_brakeMode').value = s.brake??0;
  $('s_alarm').value     = s.alarm??0;
  $('s_posLight').value  = s.pos??0;
  $('s_wave').checked    = !!s.wave;
  $('s_lowBeam').value   = s.low??0;
  $('s_aux1').value      = s.aux1??0;
  $('s_aux2').value      = s.aux2??0;
  $('s_standKill').value = s.stand??0;
  $('s_parking').value   = s.park??0;
  $('s_turnDist').value  = s.tdist??50;
  updateTurnDistanceCalibrationUi();
}

function saveSettings(){
  wsSend({cmd:'setSettings', data:{
    handlebar: +$('s_handlebar').value,
    rear:      +$('s_rear').value,
    turn:      +$('s_turnMode').value,
    brake:     +$('s_brakeMode').value,
    alarm:     +$('s_alarm').value,
    pos:       +$('s_posLight').value,
    wave:      $('s_wave').checked?1:0,
    low:       +$('s_lowBeam').value,
    aux1:      +$('s_aux1').value,
    aux2:      +$('s_aux2').value,
    stand:     +$('s_standKill').value,
    park:      +$('s_parking').value,
    tdist:     +$('s_turnDist').value,
  }});
  toast(t('toast_saved'),'success');
}

function loadSettings(){ wsSend({cmd:'getSettings'}); }

function factoryReset(){
  if(confirm(t('confirm_factory'))){
    wsSend({cmd:'factoryReset'});
    toast(t('toast_factory'),'success');
  }
}

// ─── BLE Keyless ───
function updateKeyless(m){
  const ring = $('keylessRing');
  const state = $('keylessState');
  const detail = $('keylessDetail');

  $('s_keylessEn').checked = !!m.enabled;
  $('s_rssiThresh').value = m.rssiThreshold||-65;
  $('s_graceTime').value = m.graceSeconds||10;

  if (m.scanning) {
    const count = Array.isArray(m.scanResults) ? m.scanResults.length : 0;
    const sig = `scan:${m.scanning}|count:${count}|ready:${m.scanReady}|running:${m.scannerRunning}`;
    if (sig !== lastBleScanSignature) {
      lastBleScanSignature = sig;
      bleDebug('scan status update', {
        scanning: m.scanning,
        count,
        scanReady: m.scanReady,
        scannerRunning: m.scannerRunning
      });
    }
  }

  // Paired devices
  const pdiv = $('pairedDevices');
  if(m.paired && m.paired.length>0){
    pdiv.innerHTML = m.paired.map(d => `
      <div class="ble-device ${d.connected?'paired':''}">
        <div class="icon">${d.connected?'\ud83d\udcf1':'\ud83d\udcf5'}</div>
        <div class="info">
          <div style="font-weight:500">${d.name||t('unknown')}</div>
          <div class="mac">${d.mac}</div>
        </div>
        <div class="rssi">${d.rssi?d.rssi+' dBm':'\u2013'}</div>
        <button class="btn danger" style="padding:5px 10px;font-size:.7rem" onclick="removePaired('${d.mac}')">\u2715</button>
      </div>`).join('');
  } else {
    pdiv.innerHTML = '<div style="color:var(--dim);font-size:.82rem;padding:12px 0">'+t('no_paired')+'</div>';
  }

  // Status
  if(!m.enabled){
    ring.classList.remove('active');
    ring.innerHTML = '\ud83d\udd12';
    state.textContent = t('keyless_disabled');
    detail.textContent = t('keyless_disabled_detail');
  } else if(m.phoneDetected){
    ring.classList.add('active');
    ring.innerHTML = '\ud83d\udd13';
    state.textContent = t('keyless_unlocked');
    state.style.color = 'var(--accent)';
    detail.textContent = t('keyless_unlocked_detail', {rssi: m.currentRssi||'?'});
  } else if(m.graceActive){
    ring.classList.add('active');
    ring.innerHTML = '\u23f1';
    state.textContent = t('keyless_grace');
    state.style.color = 'var(--warn)';
    detail.textContent = t('keyless_grace_detail', {sec: m.graceRemaining||0});
  } else {
    ring.classList.remove('active');
    ring.innerHTML = '\ud83d\udd12';
    state.textContent = t('keyless_locked');
    state.style.color = 'var(--danger)';
    detail.textContent = t('keyless_locked_detail');
  }

  if (Array.isArray(m.scanResults)) {
    updateScan({devices: m.scanResults});
  }

  if (m.scanning === false && $('btnScan').disabled) {
    $('btnScan').disabled = false;
    if (scanWatchdogTimer) {
      clearTimeout(scanWatchdogTimer);
      scanWatchdogTimer = null;
    }
  }
  if (m.scanning === false && !Array.isArray(m.scanResults) && $('scanCard').style.display !== 'none') {
    $('scanResults').innerHTML = '<div style="color:var(--dim);padding:12px">'+t('no_devices_found')+'</div>';
    bleDebug('scan finished without named devices');
  }
}

function sendKeylessConfig(){
  wsSend({cmd:'setKeyless', data:{
    enabled: $('s_keylessEn').checked,
    rssiThreshold: +$('s_rssiThresh').value,
    graceSeconds: +$('s_graceTime').value,
  }});
}

function toggleTurnDistanceCalibration(){
  if (!turnDistanceCalibrationActive) {
    turnDistanceCalibrationActive = true;
    turnDistanceCalibrationStartPulses = currentSpeedPulses;
    updateTurnDistanceCalibrationUi();
    wsSend({cmd:'startTurnDistanceCal'});
    return;
  }

  const captured = Math.max(0, currentSpeedPulses - turnDistanceCalibrationStartPulses);
  turnDistanceCalibrationActive = false;
  updateTurnDistanceCalibrationUi();
  wsSend({cmd:'finishTurnDistanceCal'});
  if (captured > 0) toast(t('turn_cal_done'), 'success');
  else toast(t('turn_cal_failed'), 'error', 3200);
}

function startScan(){
  $('scanCard').style.display='block';
  $('scanResults').innerHTML='<div style="color:var(--dim);padding:12px">'+t('scanning')+'</div>';
  $('btnScan').disabled = true;
  addLog(t('ble_scan_request'));
  bleDebug('startScan command sent');
  toast(t('ble_scan_debug'), 'success', 3000);
  wsSend({cmd:'startScan'});
  if (scanWatchdogTimer) clearTimeout(scanWatchdogTimer);
  scanWatchdogTimer = setTimeout(() => {
    bleDebug('still waiting for scan results after 6s');
    addLog(t('ble_scan_waiting'));
  }, 6000);
}
function stopScan(){
  $('scanCard').style.display='none';
  $('btnScan').disabled = false;
  addLog(t('ble_scan_stop'));
  bleDebug('stopScan command sent');
  if (scanWatchdogTimer) {
    clearTimeout(scanWatchdogTimer);
    scanWatchdogTimer = null;
  }
  wsSend({cmd:'stopScan'});
}

function updateScan(m){
  const sr = $('scanResults');
  const count = (m.devices && m.devices.length) ? m.devices.length : 0;
  bleDebug('scan results received', {count});
  if (scanWatchdogTimer && count > 0) {
    clearTimeout(scanWatchdogTimer);
    scanWatchdogTimer = null;
  }
  if(!m.devices||m.devices.length===0){
    sr.innerHTML='<div style="color:var(--dim);padding:12px">'+t('no_devices_found')+'</div>';
    return;
  }
  sr.innerHTML = m.devices.map(d => `
    <div class="ble-device">
      <div class="icon">\ud83d\udcf2</div>
      <div class="info">
        <div style="font-weight:500">${d.name||t('unknown')}</div>
        <div class="mac">${d.mac}</div>
      </div>
      <div class="rssi">${d.rssi} dBm</div>
      <button class="btn primary" style="padding:5px 12px;font-size:.72rem" onclick="pairDevice('${d.mac}')">${t('btn_pair')}</button>
    </div>`).join('');
}

function pairDevice(mac){
  wsSend({cmd:'pairDevice', mac});
  toast(t('pairing_device'),'success');
  stopScan();
}
function removePaired(mac){
  if(confirm(t('confirm_remove'))){
    wsSend({cmd:'removePaired', mac});
  }
}

// ─── Log ───
function addLog(text){
  const now = new Date().toLocaleTimeString(currentLang==='de'?'de-DE':'en-US',{hour:'2-digit',minute:'2-digit',second:'2-digit'});
  logLines.push(`<span style="color:var(--accent)">${now}</span> ${text}`);
  if(logLines.length>80) logLines.shift();
  const box = $('logBox');
  box.innerHTML = logLines.join('<br>');
  box.scrollTop = box.scrollHeight;
}

// ─── Toast ───
function toast(text, level='success', durationMs=2500){
  const t = $('toast');
  t.textContent = text;
  t.className = 'toast show ' + level;
  if (toastTimer) clearTimeout(toastTimer);
  toastTimer = setTimeout(()=> t.className='toast', durationMs);
}

// ─── Tabs ───
document.querySelectorAll('.tab').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.tab').forEach(b=>b.classList.remove('active'));
    document.querySelectorAll('.tab-content').forEach(c=>c.classList.remove('active'));
    btn.classList.add('active');
    $('tab-'+btn.dataset.tab).classList.add('active');
  });
});

// ─── Boot ───
// Restore language from localStorage
document.querySelectorAll('.lang-btn').forEach(b => {
  b.classList.toggle('active', b.dataset.lang === currentLang);
});
applyI18n();
populateSelects();
initTiles();
wsConnect();
</script>
</body>
</html>
)WEBUI";

static constexpr size_t WEB_UI_HTML_LEN = sizeof(WEB_UI_HTML) - 1;
