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

/* ─── Home / Landing ─── */
.home-hero{
  background:radial-gradient(circle at top right,rgba(0,230,138,.14),transparent 52%),
             linear-gradient(145deg,#0f2034 0%,#0b1725 100%);
}
.home-greeting{
  font-size:1.25rem;font-weight:700;line-height:1.3;
}
.home-bike{
  margin-top:6px;font-size:.92rem;color:var(--muted);
}
.home-meta{
  margin-top:14px;display:flex;gap:14px;flex-wrap:wrap;
  font-size:.78rem;color:var(--dim);
}
.home-meta strong{
  font-family:'JetBrains Mono',monospace;color:var(--text);font-weight:700;
}
.home-summary{
  display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:10px;
}
.home-stat{
  background:var(--bg);border:1px solid var(--border);border-radius:10px;
  padding:10px 11px;
}
.home-stat .k{
  font-size:.68rem;letter-spacing:.35px;color:var(--dim);
  font-family:'JetBrains Mono',monospace;text-transform:uppercase;
  display:flex;align-items:center;gap:6px;
}
.home-stat .k .icon{
  font-size:.92rem;line-height:1;
}
.home-stat .v{
  margin-top:6px;font-size:.95rem;font-weight:600;color:var(--text);
}
@media(max-width:700px){
  .home-summary{grid-template-columns:repeat(2,minmax(0,1fr))}
}
@media(max-width:420px){
  .home-summary{grid-template-columns:1fr}
}

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
.card-title .info-btn{
  margin-left:auto;
  width:18px;height:18px;border-radius:50%;
  border:1px solid var(--border2);
  background:var(--bg);color:var(--muted);
  font-family:'JetBrains Mono',monospace;font-size:.66rem;font-weight:700;
  display:inline-flex;align-items:center;justify-content:center;
  cursor:pointer;transition:.2s;
}
.card-title .info-btn:hover{
  color:var(--accent);border-color:var(--accent);
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
  color:var(--dim);margin-left:auto;display:none;
}
body.advanced-mode .io-tile .pin{display:inline}
.io-tile .val{
  font-family:'JetBrains Mono',monospace;font-size:1.15rem;
  font-weight:700;color:var(--off);
}
.io-tile.active .val{color:var(--on)}
.io-tile .sub{font-size:.7rem;color:var(--dim);font-family:'JetBrains Mono',monospace}
.io-tile.manual-forced{
  border-color:rgba(255,194,70,.7);
  box-shadow:0 0 14px rgba(255,194,70,.18);
}
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
.adv-hold-hint{
  margin-top:8px;font-size:.72rem;color:var(--warn);
  font-family:'JetBrains Mono',monospace;display:none;
}
body.advanced-mode .adv-hold-hint{display:block}
select,input[type=number],input[type=text]{
  font-family:'JetBrains Mono',monospace;font-size:.82rem;
  background:var(--bg);border:1px solid var(--border);
  border-radius:6px;padding:8px 12px;color:var(--text);
  outline:none;min-width:140px;transition:.2s;
}
select:focus,input[type=number]:focus,input[type=text]:focus{border-color:var(--accent)}
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

/* ─── Info Overlay ─── */
.info-overlay{
  position:fixed;inset:0;z-index:1200;
  display:none;align-items:center;justify-content:center;
  background:rgba(3,7,12,.82);
  backdrop-filter:blur(6px);
  padding:16px;
}
.info-overlay.show{display:flex}
.info-overlay-panel{
  width:min(760px,100%);
  max-height:85vh;overflow:auto;
  background:linear-gradient(165deg,var(--panel2) 0%,var(--panel) 100%);
  border:1px solid var(--border2);border-radius:14px;
  box-shadow:0 20px 70px rgba(0,0,0,.55);
  padding:18px 18px 14px;
}
.info-overlay-title{
  font-size:.9rem;font-weight:700;letter-spacing:.4px;
  color:var(--accent);text-transform:uppercase;
  margin-bottom:10px;
}
.info-overlay-body{
  font-size:.84rem;line-height:1.65;color:var(--text);
  white-space:pre-line;
}
.info-overlay-hint{
  margin-top:12px;font-size:.72rem;color:var(--dim);
  font-family:'JetBrains Mono',monospace;
}

/* ─── Onboarding Overlay ─── */
.onboarding-overlay{
  position:fixed;inset:0;z-index:1400;display:none;
  align-items:center;justify-content:center;
  background:rgba(4,8,14,.88);backdrop-filter:blur(6px);
  padding:16px;
}
.onboarding-overlay.show{display:flex}
.onboarding-panel{
  width:min(560px,100%);
  background:linear-gradient(165deg,var(--panel2) 0%,var(--panel) 100%);
  border:1px solid var(--border2);border-radius:14px;
  box-shadow:0 20px 70px rgba(0,0,0,.6);
  padding:18px;
}
.onboarding-title{
  font-size:1.05rem;font-weight:700;margin-bottom:8px;color:var(--accent);
}
.onboarding-desc{
  font-size:.82rem;color:var(--muted);line-height:1.6;margin-bottom:10px;
}

/* ─── Footer ─── */
.footer{
  text-align:center;padding:30px 20px;font-size:.72rem;color:var(--dim);
  font-family:'JetBrains Mono',monospace;
}
.footer a{
  color:var(--accent);
  text-decoration:none;
}
.footer a:hover{text-decoration:underline}
</style>
</head>
<body>

<!-- ═══ Header ═══ -->
<div class="header">
  <div class="logo">MOTO32<span>v0.1 Alpha Dashboard</span></div>
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
  <button class="tab active" data-tab="home" data-i18n="tab_home"></button>
  <button class="tab" data-tab="diag" data-i18n="tab_diag"></button>
  <button class="tab" data-tab="settings" data-i18n="tab_settings"></button>
  <button class="tab" data-tab="ble" data-i18n="tab_ble"></button>
</div>
<div class="mode-banner" id="modeBanner">ADVANCED MODE ACTIVE</div>

<!-- ══════════════════════════════════════════════════ -->
<!-- TAB: HOME                                          -->
<!-- ══════════════════════════════════════════════════ -->
<div class="tab-content active" id="tab-home">
  <div class="card home-hero">
    <div class="home-greeting" id="homeGreeting">Hello Rider</div>
    <div class="home-bike" id="homeBike">Bike profile not configured yet.</div>
    <div class="home-meta">
      <span><span data-i18n="home_fw"></span> <strong id="homeFirmware">--</strong></span>
      <span><span data-i18n="home_mode"></span> <strong id="homeMode">BASIC</strong></span>
    </div>
  </div>

  <div class="card">
    <div class="card-title" data-i18n="card_home_status"></div>
    <div class="home-summary">
      <div class="home-stat">
        <div class="k"><span class="icon">&#9889;</span><span data-i18n="home_voltage"></span></div>
        <div class="v" id="homeVoltage">N/A</div>
      </div>
      <div class="home-stat">
        <div class="k"><span class="icon">&#128273;</span><span data-i18n="home_ignition"></span></div>
        <div class="v" id="homeIgnition">--</div>
      </div>
      <div class="home-stat">
        <div class="k"><span class="icon">&#127949;</span><span data-i18n="home_engine"></span></div>
        <div class="v" id="homeEngine">--</div>
      </div>
    </div>
  </div>
</div>

<!-- ══════════════════════════════════════════════════ -->
<!-- TAB: DIAGNOSTICS                                   -->
<!-- ══════════════════════════════════════════════════ -->
<div class="tab-content" id="tab-diag">

  <!-- Battery & System -->
  <div class="card">
    <div class="card-title" data-i18n="card_battery"></div>
    <div class="grid3" style="margin-bottom:12px">
      <div>
        <div style="font-size:.72rem;color:var(--muted);margin-bottom:4px" data-i18n="lbl_voltage"></div>
        <div id="batVolt" style="font-family:'JetBrains Mono',monospace;font-size:1.6rem;font-weight:700;color:var(--accent)">N/A</div>
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
    <div class="card-title">
      <span data-i18n="card_language"></span>
      <button class="info-btn" type="button" data-info-i18n="info_language" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
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

  <!-- Rider & Bike Profile -->
  <div class="card">
    <div class="card-title">
      <span data-i18n="card_profile"></span>
      <button class="info-btn" type="button" data-info-i18n="info_profile" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_driver_name"></div>
        <div class="desc" data-i18n="set_driver_desc"></div>
      </div>
      <input type="text" id="s_driverName" maxlength="23" autocomplete="name">
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_brand_name"></div>
        <div class="desc" data-i18n="set_brand_desc"></div>
      </div>
      <input type="text" id="s_bikeBrand" maxlength="23" autocomplete="organization">
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_model_name"></div>
        <div class="desc" data-i18n="set_model_desc"></div>
      </div>
      <input type="text" id="s_bikeModel" maxlength="23" autocomplete="off">
    </div>
  </div>

  <!-- Operation Mode -->
  <div class="card">
    <div class="card-title">
      <span data-i18n="card_mode"></span>
      <button class="info-btn" type="button" data-info-i18n="info_mode" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
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
    <div class="adv-hold-hint" id="advHoldHint" data-i18n="adv_hold_hint"></div>
  </div>

  <!-- Handlebar -->
  <div class="card">
    <div class="card-title">
      <span data-i18n="card_handlebar"></span>
      <button class="info-btn" type="button" data-info-i18n="info_handlebar" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
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
    <div class="card-title">
      <span data-i18n="card_lights"></span>
      <button class="info-btn" type="button" data-info-i18n="info_lights" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
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
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_drl_source_name"></div>
        <div class="desc" data-i18n="set_drl_source_desc"></div>
      </div>
      <select id="s_drlSource"></select>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_drl_dim_name"></div>
        <div class="desc" data-i18n="set_drl_dim_desc"></div>
      </div>
      <select id="s_drlDim"></select>
    </div>
  </div>

  <!-- Turn Signals -->
  <div class="card">
    <div class="card-title">
      <span data-i18n="card_turn"></span>
      <button class="info-btn" type="button" data-info-i18n="info_turn" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
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
  </div>

  <!-- Brake Light -->
  <div class="card">
    <div class="card-title">
      <span data-i18n="card_brake"></span>
      <button class="info-btn" type="button" data-info-i18n="info_brake" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
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
    <div class="card-title">
      <span data-i18n="card_safety"></span>
      <button class="info-btn" type="button" data-info-i18n="info_safety" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
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
    <div class="card-title">
      <span data-i18n="card_aux"></span>
      <button class="info-btn" type="button" data-info-i18n="info_aux" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
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
    <div class="card-title">
      <span data-i18n="card_keyless_settings"></span>
      <button class="info-btn" type="button" data-info-i18n="info_keyless_settings" onclick="showInfo(this.dataset.infoI18n)">i</button>
    </div>
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
        <div id="rssiLevelInfo" style="font-size:.72rem;color:var(--dim);margin-top:4px"></div>
      </div>
      <select id="s_rssiLevel" onchange="updateRssiLevelUi();sendKeylessConfig()"></select>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_active_name"></div>
        <div class="desc" data-i18n="set_active_desc"></div>
      </div>
      <select id="s_activeTime" onchange="sendKeylessConfig()"></select>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_act_mode_name"></div>
        <div class="desc" data-i18n="set_act_mode_desc"></div>
      </div>
      <select id="s_actMode" onchange="updateKeylessActivationUi();sendKeylessConfig()"></select>
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_act_button_name"></div>
        <div class="desc" data-i18n="set_act_button_desc"></div>
      </div>
      <select id="s_actButton" onchange="sendKeylessConfig()"></select>
    </div>
  </div>
</div>

<!-- ═══ Toast ═══ -->
<div class="toast" id="toast"></div>
<div class="info-overlay" id="infoOverlay" onclick="closeInfoOverlay()">
  <div class="info-overlay-panel">
    <div class="info-overlay-title" id="infoOverlayTitle"></div>
    <div class="info-overlay-body" id="infoOverlayBody"></div>
    <div class="info-overlay-hint" id="infoOverlayHint" data-i18n="info_overlay_hint"></div>
  </div>
</div>
<div class="onboarding-overlay" id="profileOverlay">
  <div class="onboarding-panel">
    <div class="onboarding-title" data-i18n="profile_setup_title"></div>
    <div class="onboarding-desc" data-i18n="profile_setup_desc"></div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_driver_name"></div>
      </div>
      <input type="text" id="onb_driverName" maxlength="23" autocomplete="name">
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_brand_name"></div>
      </div>
      <input type="text" id="onb_bikeBrand" maxlength="23" autocomplete="organization">
    </div>
    <div class="setting-row">
      <div class="setting-label">
        <div class="name" data-i18n="set_model_name"></div>
      </div>
      <input type="text" id="onb_bikeModel" maxlength="23" autocomplete="off">
    </div>
    <div class="btn-row" style="justify-content:flex-end">
      <button class="btn" type="button" onclick="skipProfileOnboarding()" data-i18n="btn_profile_skip"></button>
      <button class="btn primary" type="button" onclick="saveProfileOnboarding()" data-i18n="btn_profile_save"></button>
    </div>
  </div>
</div>

<div class="footer">
  MOTO32 OPEN SOURCE &middot; ESP32-S3 N16R8 &middot; MIT LICENSE &middot;
  <a href="https://github.com/StefanMeGit/moto32-firmware/" target="_blank" rel="noopener noreferrer">GitHub Repo</a>
</div>

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
    tab_home: '\ud83c\udfe0 Home',
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
    ble_next_scan: 'Next scan in {sec}s',
    ble_next_scan_na: 'Next scan: -',
    ble_scanning: 'Scan running...',
    info_overlay_hint: 'Tap anywhere on this overlay to close',

    // Home
    card_home_status: 'Device Overview',
    home_fw: 'Firmware:',
    home_mode: 'Mode:',
    home_web: 'Web Link',
    home_ble: 'BLE Detection',
    home_ble_next: 'BLE Next Scan',
    home_voltage: 'Voltage',
    home_ignition: 'Ignition',
    home_engine: 'Engine',
    home_hello_driver: 'Hello, {driver}.',
    home_hello_generic: 'Hello, Rider.',
    home_bike_named: 'Bike: {brand} {model}',
    home_bike_missing: 'Bike profile not configured yet.',
    home_ble_detected: 'Detected',
    home_ble_not_detected: 'Not detected',
    state_on: 'ON',
    state_off: 'OFF',
    val_na: 'N/A',

    // Diagnostics
    card_battery: 'Battery & System',
    lbl_voltage: 'Voltage',
    lbl_status: 'Status',
    lbl_engine: 'Engine',
    card_inputs: 'Inputs',
    card_outputs: 'Outputs',
    adv_outputs_hint: 'In ADVANCED mode, tap input/output cards to force states manually.',
    adv_hold_hint: 'Tip and hold any input/output tile for 1 second to return it to normal mode.',
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
    err_input_implausible: 'Input Plausibility',

    // I/O labels
    io_active: 'Active',
    io_inactive: 'Inactive',
    manual_on: 'MANUAL ON',
    manual_off: 'MANUAL OFF',

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
    info_language: 'This card changes only the dashboard language.\n- EN/DE switch is immediate.\n- No firmware behavior is changed.\n- Your selection is saved in the browser.',

    card_profile: 'Rider & Bike Profile',
    set_driver_name: 'Driver Name',
    set_driver_desc: 'Name used for greeting on the home screen',
    set_brand_name: 'Bike Brand',
    set_brand_desc: 'Manufacturer (e.g. BMW, Ducati, Honda)',
    set_model_name: 'Bike Model',
    set_model_desc: 'Model (e.g. R nineT, Monster 1200)',
    info_profile: 'Defines your personal landing profile.\n- Driver name is used in the greeting.\n- Brand + model build the bike name shown on Home.\n- First boot requires these fields before normal use.',
    profile_setup_title: 'Initial Device Setup',
    profile_setup_desc: 'Please enter rider and bike information before using the dashboard.',
    profile_setup_required: 'Please fill driver, brand and model.',
    profile_setup_done: 'Profile saved',
    profile_setup_skipped: 'Profile setup skipped',
    btn_profile_skip: 'Skip for now',
    btn_profile_save: 'Save and Continue',

    // Settings - Handlebar
    card_mode: 'Operation Mode',
    set_mode_name: 'Control Level',
    set_mode_desc: 'BASIC = safe display. ADVANCED = direct input/output control.',
    info_mode: 'Operation mode defines control permissions.\n- BASIC: monitor and configure settings only.\n- ADVANCED: manual forcing of inputs/outputs is allowed.\n- ADVANCED can bypass normal logic, so use it only for diagnostics and testing.',

    // Settings - Handlebar
    card_handlebar: 'Handlebar Configuration',
    set_handlebar_name: 'Switch Unit',
    set_handlebar_desc: 'Type of handlebar switch unit',
    opt_hbar_a: 'A \u2013 5 Pushbuttons',
    opt_hbar_b: 'B \u2013 Harley / BMW',
    opt_hbar_c: 'C \u2013 Japanese / European',
    opt_hbar_d: 'D \u2013 Ducati',
    opt_hbar_e: 'E \u2013 4 Pushbuttons',
    info_handlebar: 'Select the physical switch-unit type so button presses are interpreted correctly.\n- A is default (5 pushbuttons).\n- B/C/D/E adapt mapping for other common layouts.\n- Wrong selection can cause unexpected button behavior.',

    // Settings - Lights
    card_lights: 'Lighting',
    set_rear_name: 'Tail Light Mode',
    set_rear_desc: 'Tail light behavior',
    opt_rear_0: 'Standard',
    opt_rear_1: 'Dimmed',
    opt_rear_2: 'Always On',
    set_lowbeam_name: 'Low Beam Mode',
    set_lowbeam_desc: 'When the low beam turns on',
    opt_low_0: 'Standard (button controlled)',
    opt_low_1: 'Always on with ignition',
    opt_low_2: 'Manual only',
    set_poslight_name: 'Position Light',
    set_poslight_desc: 'Brightness 0 (off) to 9 (50%)',
    set_parking_name: 'Parking Light',
    set_parking_desc: 'Lights when ignition is off',
    opt_park_0: 'Off',
    opt_park_1: 'Position light',
    opt_park_2: 'Left indicator',
    opt_park_3: 'Right indicator',
    set_drl_source_name: 'Daytime Running Light',
    set_drl_source_desc: 'Select light source for DRL while ignition is on',
    opt_drlsrc_0: 'Off',
    opt_drlsrc_1: 'Low Beam',
    opt_drlsrc_2: 'High Beam',
    opt_drlsrc_3: 'AUX 1',
    opt_drlsrc_4: 'AUX 2',
    set_drl_dim_name: 'DRL Brightness',
    set_drl_dim_desc: 'Dim level for selected DRL source',
    opt_drldim_25: '25%',
    opt_drldim_50: '50%',
    opt_drldim_75: '75%',
    opt_drldim_100: '100%',
    info_lights: 'LIGHTING CARD\n\n1) Tail Light Mode\n- Standard: brake output is only active while braking.\n- Dimmed: tail light is dim with ignition ON; braking still goes full brightness.\n- Always On: tail light output stays fully ON while ignition is ON.\n\n2) Low Beam Mode\n- Standard: controlled by normal button logic.\n- Always on with ignition: low beam is forced ON whenever ignition is ON.\n- Manual only: low beam changes only by manual input logic.\n\n3) Position Light\n- Range 0..9 (PWM).\n- 0 = off.\n- Higher values increase dim brightness when low beam is not active.\n\n4) Parking Light (Ignition OFF)\n- Off: no parking output.\n- Position light: dim low-beam channel as parking light.\n- Left/Right indicator: steady parking light on selected side.\n\n5) Daytime Running Light (DRL)\n- Source: Off, Low Beam, High Beam, AUX1, AUX2.\n- Brightness: 25%, 50%, 75%, 100%.\n- Active while ignition is ON (suppressed during starter engagement).',

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
    info_turn: 'Turn-signal automation is configured here.\n- Auto-cancel can be disabled, time-based, or distance-based.\n- Distance mode uses speed pulses and target pulse count.\n- 10m calibration workflow: start, push bike 10m, confirm.',

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
    info_brake: 'Brake output pattern selection.\n- Continuous: always solid when braking.\n- PWM fade and flash modes: attention-grabbing warning patterns.\n- Emergency mode can trigger hazard behavior during hard braking.',

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
    info_safety: 'Safety-related input behavior.\n- Sidestand/Kill combined setting defines NO/NC logic.\n- Alarm enables vibration-triggered warning while parked.\n- Verify wiring and logic carefully before road use.',

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
    info_aux: 'AUX output strategies.\n- On with ignition.\n- On only with engine running.\n- Manual mode (via dashboard control).\n- Disabled.',

    // Buttons
    btn_save: '\ud83d\udcbe Save Settings',
    btn_reload: '\ud83d\udd04 Reload',
    btn_factory: '\u26a0 Factory Reset',

    // BLE Keyless
    card_keyless: 'BLE Keyless Ignition',
    keyless_desc: 'Pair a smartphone via BLE with the Moto32. Once detected, a timed keyless session is armed. Ignition switches on only after the configured button trigger. During the session the unit scans every 30 seconds and extends the timer when the phone is still in range.',
    keyless_inactive: 'Inactive',
    keyless_no_device: 'No device paired',
    keyless_disabled: 'Disabled',
    keyless_disabled_detail: 'Keyless function is turned off',
    keyless_unlocked: 'Unlocked',
    keyless_unlocked_detail: 'Detected (L{level}, {rssi} dBm) \u2013 remaining {time}',
    keyless_unlocked_time_only: 'Ignition granted \u2013 remaining {time}',
    keyless_waiting_button: 'Ready',
    keyless_waiting_button_detail: 'Phone detected. Press {button} to enable ignition.',
    keyless_scan_active: 'Searching...',
    keyless_scan_active_detail: 'Refresh scan running \u2013 remaining {time}',
    keyless_searching: 'Searching',
    keyless_searching_detail: 'Scanning for paired device...',
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
    set_rssi_name: 'Signal Level Threshold',
    set_rssi_desc: 'Select 1..6. Lower accepts weaker signal.',
    set_rssi_live: 'Current threshold: Level {level} ({dbm} dBm)',
    opt_rssi_1: '1 (very weak, -100 dBm)',
    opt_rssi_2: '2 (-92 dBm)',
    opt_rssi_3: '3 (-84 dBm)',
    opt_rssi_4: '4 (-76 dBm)',
    opt_rssi_5: '5 (-68 dBm)',
    opt_rssi_6: '6 (strong, -60 dBm)',
    set_active_name: 'Ignition Session',
    set_active_desc: 'How long ignition stays active after phone detection',
    opt_active_1: '1 min',
    opt_active_5: '5 min',
    opt_active_10: '10 min',
    set_act_mode_name: 'Ignition Trigger',
    set_act_mode_desc: 'How BLE keyless is allowed to switch ignition on',
    opt_act_mode_any: 'Any button',
    opt_act_mode_selected: 'Selected button',
    set_act_button_name: 'Trigger Button',
    set_act_button_desc: 'Used when "Selected button" is active',
    opt_act_btn_start: 'Starter',
    opt_act_btn_light: 'Light',
    opt_act_btn_horn: 'Horn',
    opt_act_btn_left: 'Turn Left',
    opt_act_btn_right: 'Turn Right',
    info_keyless_settings: 'BLE keyless control center.\n- Enable/disable keyless detection.\n- Set signal threshold level (1..6).\n- Define session runtime.\n- Choose ignition trigger logic (any button or selected button).',

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
    tab_home: '\ud83c\udfe0 Start',
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
    ble_next_scan: 'N\u00e4chster Scan in {sec}s',
    ble_next_scan_na: 'N\u00e4chster Scan: -',
    ble_scanning: 'Scan l\u00e4uft...',
    info_overlay_hint: 'Tippe auf das Overlay, um es zu schlie\u00dfen',

    card_home_status: 'Ger\u00e4te\u00fcbersicht',
    home_fw: 'Firmware:',
    home_mode: 'Modus:',
    home_web: 'Web-Verbindung',
    home_ble: 'BLE-Erkennung',
    home_ble_next: 'BLE N\u00e4chster Scan',
    home_voltage: 'Spannung',
    home_ignition: 'Z\u00fcndung',
    home_engine: 'Motor',
    home_hello_driver: 'Hallo, {driver}.',
    home_hello_generic: 'Hallo Fahrer.',
    home_bike_named: 'Bike: {brand} {model}',
    home_bike_missing: 'Bike-Profil noch nicht konfiguriert.',
    home_ble_detected: 'Erkannt',
    home_ble_not_detected: 'Nicht erkannt',
    state_on: 'AN',
    state_off: 'AUS',
    val_na: 'N/A',

    card_battery: 'Batterie & System',
    lbl_voltage: 'Spannung',
    lbl_status: 'Status',
    lbl_engine: 'Motor',
    card_inputs: 'Eing\u00e4nge',
    card_outputs: 'Ausg\u00e4nge',
    adv_outputs_hint: 'Im ADVANCED-Modus kannst du Ein-/Ausgangskarten direkt manuell schalten.',
    adv_hold_hint: 'Tippe und halte eine Ein-/Ausgangskarte 1 Sekunde, um sie in den Normalmodus zur\u00fcckzusetzen.',
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
    err_input_implausible: 'Eingangs-Plausibilit\u00e4t',

    io_active: 'Aktiv',
    io_inactive: 'Inaktiv',
    manual_on: 'MANUELL AN',
    manual_off: 'MANUELL AUS',

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
    info_language: 'Diese Karte \u00e4ndert nur die Dashboard-Sprache.\n- EN/DE-Wechsel erfolgt sofort.\n- Das Firmware-Verhalten bleibt unver\u00e4ndert.\n- Die Auswahl wird im Browser gespeichert.',

    card_profile: 'Fahrer- & Bike-Profil',
    set_driver_name: 'Fahrername',
    set_driver_desc: 'Wird als Begr\u00fc\u00dfung auf der Startseite angezeigt',
    set_brand_name: 'Bike-Marke',
    set_brand_desc: 'Hersteller (z. B. BMW, Ducati, Honda)',
    set_model_name: 'Bike-Modell',
    set_model_desc: 'Modell (z. B. R nineT, Monster 1200)',
    info_profile: 'Definiert dein pers\u00f6nliches Startprofil.\n- Fahrername wird in der Begr\u00fc\u00dfung verwendet.\n- Marke + Modell bilden den Bike-Namen auf der Startseite.\n- Beim ersten Start sind diese Felder Pflicht.',
    profile_setup_title: 'Erstkonfiguration',
    profile_setup_desc: 'Bitte trage Fahrer- und Bike-Daten ein, bevor du das Dashboard nutzt.',
    profile_setup_required: 'Bitte Fahrer, Marke und Modell ausf\u00fcllen.',
    profile_setup_done: 'Profil gespeichert',
    profile_setup_skipped: 'Profil-Setup \u00fcbersprungen',
    btn_profile_skip: 'Jetzt \u00fcberspringen',
    btn_profile_save: 'Speichern und Weiter',

    card_mode: 'Betriebsmodus',
    set_mode_name: 'Steuerstufe',
    set_mode_desc: 'BASIC = sichere Anzeige. ADVANCED = direkte Ein-/Ausgangssteuerung.',
    info_mode: 'Der Betriebsmodus definiert die Berechtigungen.\n- BASIC: nur Monitoring und Einstellungen.\n- ADVANCED: manuelles Forcen von Ein-/Ausg\u00e4ngen ist erlaubt.\n- ADVANCED kann normale Logik umgehen, daher nur f\u00fcr Diagnose/Tests nutzen.',

    card_handlebar: 'Lenker-Konfiguration',
    set_handlebar_name: 'Lenkerarmatur',
    set_handlebar_desc: 'Art der Schaltereinheit am Lenker',
    opt_hbar_a: 'A \u2013 5 Taster',
    opt_hbar_b: 'B \u2013 Harley / BMW',
    opt_hbar_c: 'C \u2013 Japan / Europa',
    opt_hbar_d: 'D \u2013 Ducati',
    opt_hbar_e: 'E \u2013 4 Taster',
    info_handlebar: 'W\u00e4hlt den Typ der Schaltereinheit, damit Taster korrekt interpretiert werden.\n- A ist Standard (5 Taster).\n- B/C/D/E passen das Mapping f\u00fcr andere Layouts an.\n- Falsche Auswahl kann unerwartetes Tasterverhalten verursachen.',

    card_lights: 'Beleuchtung',
    set_rear_name: 'R\u00fccklicht-Modus',
    set_rear_desc: 'Verhalten des R\u00fccklichts',
    opt_rear_0: 'Standard',
    opt_rear_1: 'Gedimmt',
    opt_rear_2: 'Immer an',
    set_lowbeam_name: 'Abblendlicht-Modus',
    set_lowbeam_desc: 'Wann schaltet das Abblendlicht ein',
    opt_low_0: 'Standard (per Taster)',
    opt_low_1: 'Immer an mit Zündung',
    opt_low_2: 'Nur manuell',
    set_poslight_name: 'Positionslicht',
    set_poslight_desc: 'Helligkeit 0 (aus) bis 9 (50%)',
    set_parking_name: 'Parkbeleuchtung',
    set_parking_desc: 'Licht bei ausgeschalteter Z\u00fcndung',
    opt_park_0: 'Aus',
    opt_park_1: 'Standlicht',
    opt_park_2: 'Blinker links',
    opt_park_3: 'Blinker rechts',
    set_drl_source_name: 'Tagfahrlicht',
    set_drl_source_desc: 'W\u00e4hle die Lichtquelle f\u00fcr DRL bei aktiver Z\u00fcndung',
    opt_drlsrc_0: 'Aus',
    opt_drlsrc_1: 'Abblendlicht',
    opt_drlsrc_2: 'Fernlicht',
    opt_drlsrc_3: 'AUX 1',
    opt_drlsrc_4: 'AUX 2',
    set_drl_dim_name: 'DRL-Helligkeit',
    set_drl_dim_desc: 'Dimmstufe der gew\u00e4hlten DRL-Lichtquelle',
    opt_drldim_25: '25%',
    opt_drldim_50: '50%',
    opt_drldim_75: '75%',
    opt_drldim_100: '100%',
    info_lights: 'BELEUCHTUNGS-KARTE\n\n1) R\u00fccklicht-Modus\n- Standard: Bremslicht-Ausgang ist nur beim Bremsen aktiv.\n- Gedimmt: R\u00fccklicht ist bei Z\u00fcndung EIN gedimmt aktiv; beim Bremsen volle Helligkeit.\n- Immer an: R\u00fccklicht-Ausgang ist bei Z\u00fcndung EIN dauerhaft voll aktiv.\n\n2) Abblendlicht-Modus\n- Standard: normale Taster-/Logiksteuerung.\n- Immer an mit Z\u00fcndung: Abblendlicht wird bei Z\u00fcndung EIN erzwungen.\n- Nur manuell: \u00c4nderung nur \u00fcber manuelle Eingangslogik.\n\n3) Positionslicht\n- Bereich 0..9 (PWM).\n- 0 = aus.\n- H\u00f6here Werte erh\u00f6hen die Dimmhelligkeit, wenn Abblendlicht nicht aktiv ist.\n\n4) Parkbeleuchtung (Z\u00fcndung AUS)\n- Aus: keine Parklicht-Ausgabe.\n- Standlicht: gedimmter Abblendlicht-Kanal als Parklicht.\n- Blinker links/rechts: dauerhaftes Parklicht auf der gew\u00e4hlten Seite.\n\n5) Tagfahrlicht (DRL)\n- Quelle: Aus, Abblendlicht, Fernlicht, AUX1, AUX2.\n- Helligkeit: 25%, 50%, 75%, 100%.\n- Aktiv bei Z\u00fcndung EIN (w\u00e4hrend Starterlauf unterdr\u00fcckt).',

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
    info_turn: 'Einstellungen f\u00fcr die automatische Blinker-Abschaltung.\n- Auto-Abschaltung kann deaktiviert, zeit- oder streckenbasiert sein.\n- Streckenmodus nutzt Geschwindigkeitspulse und einen Zielwert.\n- 10m-Kalibrierung: starten, Motorrad 10m schieben, best\u00e4tigen.',

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
    info_brake: 'Auswahl des Bremslicht-Musters.\n- Dauerlicht: stabil bei Bremsen.\n- PWM-Fade/Blitzmodi: erh\u00f6hte Sichtbarkeit.\n- Notbremsmodus kann Warnblinker bei starker Bremsung aktivieren.',

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
    info_safety: 'Sicherheitsrelevante Eingangslogik.\n- Kombinierte Einstellung f\u00fcr Seitenst\u00e4nder/Kill (NO/NC).\n- Alarm aktiviert Ersch\u00fctterungserkennung im Stand.\n- Vor Stra\u00dfennutzung Verdrahtung und Logik zwingend pr\u00fcfen.',

    card_aux: 'Zusatzausg\u00e4nge',
    set_aux1_name: 'AUX 1 Modus',
    set_aux1_desc: 'Verhalten des Zusatzausgangs 1',
    set_aux2_name: 'AUX 2 Modus',
    set_aux2_desc: 'Verhalten des Zusatzausgangs 2',
    opt_aux_0: 'An bei Z\u00fcndung',
    opt_aux_1: 'An bei Motor',
    opt_aux_2: 'Manuell',
    opt_aux_3: 'Deaktiviert',
    info_aux: 'AUX-Ausgangsstrategien.\n- An bei Z\u00fcndung.\n- An nur bei laufendem Motor.\n- Manueller Modus (per Dashboard).\n- Deaktiviert.',

    btn_save: '\ud83d\udcbe Einstellungen speichern',
    btn_reload: '\ud83d\udd04 Neu laden',
    btn_factory: '\u26a0 Werkseinstellungen',

    card_keyless: 'BLE Keyless-Z\u00fcndung',
    keyless_desc: 'Verbinde ein Smartphone per BLE mit der Moto32. Nach Erkennung wird eine zeitbasierte Keyless-Session aktiviert. Die Z\u00fcndung wird erst nach dem konfigurierten Taster-Trigger eingeschaltet. W\u00e4hrend der Session wird alle 30 Sekunden gesucht und bei erkanntem Handy der Timer verl\u00e4ngert.',
    keyless_inactive: 'Inaktiv',
    keyless_no_device: 'Kein Ger\u00e4t angelernt',
    keyless_disabled: 'Deaktiviert',
    keyless_disabled_detail: 'Keyless-Funktion ist ausgeschaltet',
    keyless_unlocked: 'Entsperrt',
    keyless_unlocked_detail: 'Erkannt (L{level}, {rssi} dBm) \u2013 Restzeit {time}',
    keyless_unlocked_time_only: 'Z\u00fcndung freigegeben \u2013 Restzeit {time}',
    keyless_waiting_button: 'Bereit',
    keyless_waiting_button_detail: 'Handy erkannt. Dr\u00fccke {button}, um die Z\u00fcndung einzuschalten.',
    keyless_scan_active: 'Suche l\u00e4uft...',
    keyless_scan_active_detail: 'Refresh-Scan aktiv \u2013 Restzeit {time}',
    keyless_searching: 'Suche',
    keyless_searching_detail: 'Suche nach angelerntem Ger\u00e4t...',
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
    set_rssi_name: 'Signalstufen-Schwelle',
    set_rssi_desc: 'W\u00e4hle 1..6. Niedrig akzeptiert schw\u00e4cheres Signal.',
    set_rssi_live: 'Aktuelle Schwelle: Stufe {level} ({dbm} dBm)',
    opt_rssi_1: '1 (sehr schwach, -100 dBm)',
    opt_rssi_2: '2 (-92 dBm)',
    opt_rssi_3: '3 (-84 dBm)',
    opt_rssi_4: '4 (-76 dBm)',
    opt_rssi_5: '5 (-68 dBm)',
    opt_rssi_6: '6 (stark, -60 dBm)',
    set_active_name: 'Z\u00fcndungs-Session',
    set_active_desc: 'Wie lange die Z\u00fcndung nach Handy-Erkennung aktiv bleibt',
    opt_active_1: '1 min',
    opt_active_5: '5 min',
    opt_active_10: '10 min',
    set_act_mode_name: 'Z\u00fcndungs-Trigger',
    set_act_mode_desc: 'Wie BLE Keyless die Z\u00fcndung einschalten darf',
    opt_act_mode_any: 'Beliebiger Taster',
    opt_act_mode_selected: 'Ausgew\u00e4hlter Taster',
    set_act_button_name: 'Trigger-Taster',
    set_act_button_desc: 'Wird bei "Ausgew\u00e4hlter Taster" verwendet',
    opt_act_btn_start: 'Starter',
    opt_act_btn_light: 'Licht',
    opt_act_btn_horn: 'Hupe',
    opt_act_btn_left: 'Blinker links',
    opt_act_btn_right: 'Blinker rechts',
    info_keyless_settings: 'Steuerzentrale f\u00fcr BLE-Keyless.\n- Keyless aktivieren/deaktivieren.\n- Signal-Schwelle als Stufe 1..6 setzen.\n- Session-Laufzeit definieren.\n- Z\u00fcndungs-Triggerlogik w\u00e4hlen (jeder Taster oder ein gew\u00e4hlter Taster).',

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
  if (activeInfoKey) renderInfoOverlay();
  updateModeUi();
  if ($('connText')) $('connText').textContent = connected ? t('conn_connected') : t('conn_disconnected');
  if (lastKeylessState) {
    updateBleConnectionUi({
      detected: (lastKeylessState.statusDetected !== undefined)
          ? !!lastKeylessState.statusDetected
          : !!lastKeylessState.phoneDetected,
      scanning: !!lastKeylessState.autoSearching || !!lastKeylessState.sessionRefreshSearching,
      nextScanIn: lastKeylessState.nextScanIn
    });
  } else {
    updateBleConnectionUi({detected:false, scanning:false, nextScanIn:-1});
  }
  updateTurnDistanceCalibrationUi();
  updateRssiLevelUi();
  updateHomeOverview();
  updateProfileOverlay();
}

// ─── Select population (language-aware) ───
function populateSelects() {
  const fill = (id, opts) => {
    const sel = document.getElementById(id);
    if (!sel) return;
    const cur = sel.value;
    sel.innerHTML = opts.map((o,i) => `<option value="${i}">${o}</option>`).join('');
    if (cur !== '') sel.value = cur;
  };
  const fillValues = (id, options, normalizeFn, defaultValue = null) => {
    const sel = document.getElementById(id);
    if (!sel) return;
    const cur = sel.value;
    sel.innerHTML = options.map(o =>
      `<option value="${o.value}">${o.label}</option>`).join('');
    if (cur !== '') {
      const normalized = normalizeFn ? normalizeFn(cur) : cur;
      sel.value = String(normalized);
    } else if (defaultValue !== null) {
      const normalized = normalizeFn ? normalizeFn(defaultValue) : defaultValue;
      sel.value = String(normalized);
    }
  };
  fill('s_handlebar', [t('opt_hbar_a'),t('opt_hbar_b'),t('opt_hbar_c'),t('opt_hbar_d'),t('opt_hbar_e')]);
  fill('s_rear', [t('opt_rear_0'),t('opt_rear_1'),t('opt_rear_2')]);
  fill('s_lowBeam', [t('opt_low_0'),t('opt_low_1'),t('opt_low_2')]);
  fill('s_parking', [t('opt_park_0'),t('opt_park_1'),t('opt_park_2'),t('opt_park_3')]);
  fill('s_drlSource', [
    t('opt_drlsrc_0'),
    t('opt_drlsrc_1'),
    t('opt_drlsrc_2'),
    t('opt_drlsrc_3'),
    t('opt_drlsrc_4')
  ]);
  fillValues('s_drlDim', [
    {value: 25, label: t('opt_drldim_25')},
    {value: 50, label: t('opt_drldim_50')},
    {value: 75, label: t('opt_drldim_75')},
    {value: 100, label: t('opt_drldim_100')}
  ], v => {
    const n = Number(v);
    if (n === 25 || n === 50 || n === 75 || n === 100) return n;
    return 50;
  }, 50);
  fill('s_turnMode', [t('opt_turn_0'),t('opt_turn_1'),t('opt_turn_2'),t('opt_turn_3'),t('opt_turn_4')]);
  fill('s_brakeMode', [t('opt_brake_0'),t('opt_brake_1'),t('opt_brake_2'),t('opt_brake_3'),t('opt_brake_4'),t('opt_brake_5'),t('opt_brake_6')]);

  const skOpts = [];
  for (let i = 0; i <= 8; i++) skOpts.push(t('opt_sk_'+i));
  fill('s_standKill', skOpts);
  fill('s_alarm', [t('opt_alarm_0'),t('opt_alarm_1')]);
  fill('s_aux1', [t('opt_aux_0'),t('opt_aux_1'),t('opt_aux_2'),t('opt_aux_3')]);
  fill('s_aux2', [t('opt_aux_0'),t('opt_aux_1'),t('opt_aux_2'),t('opt_aux_3')]);

  const rssiOptions = [];
  for (let level = 1; level <= 6; level++) {
    rssiOptions.push({value: level, label: t('opt_rssi_' + level)});
  }
  fillValues('s_rssiLevel', rssiOptions, normalizeRssiLevel, 4);

  const activeOptions = ACTIVE_MIN_OPTIONS.map(min => ({
    value: min,
    label: t('opt_active_' + min)
  }));
  fillValues('s_activeTime', activeOptions, normalizeActiveMinutes, 5);

  fillValues('s_actMode', [
    {value: 0, label: t('opt_act_mode_any')},
    {value: 1, label: t('opt_act_mode_selected')}
  ], normalizeActivationMode, 0);

  fillValues('s_actButton', KEYLESS_BUTTON_OPTIONS.map(id => ({
    value: id,
    label: t('opt_act_btn_' + id)
  })), normalizeActivationButton, 0);

  updateRssiLevelUi();
  updateKeylessActivationUi();
}

// ═══════════════════════════════════════════════════════
// CONFIG
// ═══════════════════════════════════════════════════════

const INPUT_PINS = {lock:14,turnL:15,turnR:16,light:17,start:18,horn:21,brake:38,kill:39,stand:40,aux1:41,aux2:42,speed:47};
const INPUT_IDS = ['lock','turnL','turnR','light','start','horn','brake','kill','stand','aux1','aux2','speed'];
const OUTPUT_PINS = {turnLOut:2,turnROut:4,lightOut:5,hibeam:6,brakeOut:7,hornOut:8,start1:9,start2:10,ignOut:11,aux1Out:12,aux2Out:13};
const OUTPUT_IDS = ['turnLOut','turnROut','lightOut','hibeam','brakeOut','hornOut','start1','start2','ignOut','aux1Out','aux2Out'];

// ─── State ───
let ws = null;
let connected = false;
let reconnTimer = null;
let logLines = [];
let advancedMode = false;
let lastDiagState = null;
let lastKeylessState = null;
let toastTimer = null;
let scanWatchdogTimer = null;
let keylessUiLockUntil = 0;
let keylessPendingConfig = null;
let lastBleScanSignature = '';
let currentSpeedPulses = 0;
let turnDistanceCalibrationActive = false;
let turnDistanceCalibrationStartPulses = 0;
let activeInfoKey = '';
let settingsProfileReady = true;
let profileSetupSkipped = false;
let profileData = {driverName:'', bikeBrand:'', bikeModel:''};
let firmwareVersion = '--';

// ─── DOM refs ───
const $ = id => document.getElementById(id);

const RSSI_LEVEL_TO_DBM = {
  1: -100,
  2: -92,
  3: -84,
  4: -76,
  5: -68,
  6: -60
};
const ACTIVE_MIN_OPTIONS = [1, 5, 10];
const KEYLESS_BUTTON_OPTIONS = ['start', 'light', 'horn', 'left', 'right'];

function normalizeRssiLevel(level){
  const n = Number(level);
  if (!Number.isFinite(n)) return 4;
  return Math.max(1, Math.min(6, Math.round(n)));
}

function rssiDbmForLevel(level){
  return RSSI_LEVEL_TO_DBM[normalizeRssiLevel(level)] ?? RSSI_LEVEL_TO_DBM[4];
}

function rssiLevelForDbm(dbm){
  const val = Number(dbm);
  if (!Number.isFinite(val)) return 4;
  let bestLevel = 4;
  let bestDiff = Math.abs(val - rssiDbmForLevel(bestLevel));
  for (let level = 1; level <= 6; level++) {
    const diff = Math.abs(val - rssiDbmForLevel(level));
    if (diff < bestDiff) {
      bestDiff = diff;
      bestLevel = level;
    }
  }
  return bestLevel;
}

function normalizeActiveMinutes(min){
  const n = Number(min);
  if (!Number.isFinite(n)) return 5;
  let best = ACTIVE_MIN_OPTIONS[0];
  let bestDiff = Math.abs(n - best);
  for (const candidate of ACTIVE_MIN_OPTIONS) {
    const diff = Math.abs(n - candidate);
    if (diff < bestDiff) {
      best = candidate;
      bestDiff = diff;
    }
  }
  return best;
}

function normalizeActivationMode(mode){
  return Number(mode) === 1 ? 1 : 0;
}

function normalizeActivationButton(button){
  const str = String(button);
  if (KEYLESS_BUTTON_OPTIONS.includes(str)) return str;
  const n = Number(button);
  if (Number.isFinite(n) && n >= 0 && n < KEYLESS_BUTTON_OPTIONS.length) {
    return KEYLESS_BUTTON_OPTIONS[Math.round(n)];
  }
  return 'start';
}

function activeMinutesFromLegacyGrace(sec){
  const n = Number(sec);
  if (!Number.isFinite(n) || n <= 0) return 5;
  return normalizeActiveMinutes(Math.round(n / 60));
}

function activationButtonIndex(button){
  const idx = KEYLESS_BUTTON_OPTIONS.indexOf(normalizeActivationButton(button));
  return idx >= 0 ? idx : 0;
}

function activationButtonLabel(button){
  const id = normalizeActivationButton(button);
  return t('opt_act_btn_' + id);
}

function formatSessionTime(totalSeconds){
  const sec = Math.max(0, Math.floor(Number(totalSeconds) || 0));
  const m = Math.floor(sec / 60);
  const s = String(sec % 60).padStart(2, '0');
  return `${m}:${s}`;
}

function sanitizeProfileText(value){
  return String(value || '').replace(/\s+/g, ' ').trim().slice(0, 23);
}

function profileReadyFromValues(brand, model, driver){
  return sanitizeProfileText(brand).length > 0
      && sanitizeProfileText(model).length > 0
      && sanitizeProfileText(driver).length > 0;
}

function syncProfileInputs(brand, model, driver){
  if ($('s_bikeBrand') && document.activeElement !== $('s_bikeBrand')) {
    $('s_bikeBrand').value = brand;
  }
  if ($('s_bikeModel') && document.activeElement !== $('s_bikeModel')) {
    $('s_bikeModel').value = model;
  }
  if ($('s_driverName') && document.activeElement !== $('s_driverName')) {
    $('s_driverName').value = driver;
  }
  if ($('onb_bikeBrand') && document.activeElement !== $('onb_bikeBrand')) {
    $('onb_bikeBrand').value = brand;
  }
  if ($('onb_bikeModel') && document.activeElement !== $('onb_bikeModel')) {
    $('onb_bikeModel').value = model;
  }
  if ($('onb_driverName') && document.activeElement !== $('onb_driverName')) {
    $('onb_driverName').value = driver;
  }
}

function updateHomeOverview(){
  const diag = lastDiagState || {};
  const brand = sanitizeProfileText(profileData.bikeBrand);
  const model = sanitizeProfileText(profileData.bikeModel);
  const driver = sanitizeProfileText(profileData.driverName);

  if ($('homeGreeting')) {
    $('homeGreeting').textContent = driver
        ? t('home_hello_driver', {driver})
        : t('home_hello_generic');
  }
  if ($('homeBike')) {
    $('homeBike').textContent = (brand && model)
        ? t('home_bike_named', {brand, model})
        : t('home_bike_missing');
  }
  if ($('homeFirmware')) $('homeFirmware').textContent = firmwareVersion || '--';
  if ($('homeMode')) $('homeMode').textContent = advancedMode ? t('mode_advanced') : t('mode_basic');
  if ($('homeVoltage')) {
    const hasVoltage = !!diag.voltageAvailable;
    const v = Number(diag.voltage);
    $('homeVoltage').textContent = (hasVoltage && Number.isFinite(v))
        ? `${v.toFixed(1)} V`
        : t('val_na');
  }
  if ($('homeIgnition')) $('homeIgnition').textContent = diag.ignitionOn ? t('state_on') : t('state_off');
  if ($('homeEngine')) {
    let engineText = t('state_off');
    if (diag.engineRunning) engineText = t('eng_running');
    else if (diag.starterEngaged) engineText = t('eng_starter');
    else if (diag.ignitionOn) engineText = t('eng_ignition');
    $('homeEngine').textContent = engineText;
  }
}

function updateProfileOverlay(){
  const overlay = $('profileOverlay');
  if (!overlay) return;
  overlay.classList.toggle('show', !settingsProfileReady && !profileSetupSkipped);
}

function skipProfileOnboarding(){
  profileSetupSkipped = true;
  updateProfileOverlay();
  saveSettings({silentToast: true, profileSkip: true});
  toast(t('profile_setup_skipped'), 'success', 2200);
}

function saveProfileOnboarding(){
  const brand = sanitizeProfileText($('onb_bikeBrand') ? $('onb_bikeBrand').value : '');
  const model = sanitizeProfileText($('onb_bikeModel') ? $('onb_bikeModel').value : '');
  const driver = sanitizeProfileText($('onb_driverName') ? $('onb_driverName').value : '');
  if (!profileReadyFromValues(brand, model, driver)) {
    toast(t('profile_setup_required'), 'error', 3600);
    return;
  }
  if ($('s_bikeBrand')) $('s_bikeBrand').value = brand;
  if ($('s_bikeModel')) $('s_bikeModel').value = model;
  if ($('s_driverName')) $('s_driverName').value = driver;
  profileSetupSkipped = false;
  saveSettings({silentToast: true, profileSkip: false});
  toast(t('profile_setup_done'), 'success', 2400);
}

function bleDebug(message, payload){
  if (payload === undefined) console.debug('[BLE]', message);
  else console.debug('[BLE]', message, payload);
}

function updateRssiLevelUi(dbmOverride){
  const input = $('s_rssiLevel');
  const info = $('rssiLevelInfo');
  if (!input || !info) return;
  const level = normalizeRssiLevel(input.value);
  input.value = level;
  const dbm = Number.isFinite(Number(dbmOverride))
      ? Math.round(Number(dbmOverride))
      : rssiDbmForLevel(level);
  info.textContent = t('set_rssi_live', {level, dbm});
}

function updateKeylessActivationUi(){
  const modeInput = $('s_actMode');
  const buttonInput = $('s_actButton');
  if (!modeInput || !buttonInput) return;
  const mode = normalizeActivationMode(modeInput.value);
  modeInput.value = String(mode);
  buttonInput.disabled = mode !== 1;
}

function isKeylessUiLocked(){
  return Date.now() < keylessUiLockUntil;
}

function updateModeUi(){
  document.body.classList.toggle('advanced-mode', advancedMode);
  if ($('modeChip')) $('modeChip').textContent = advancedMode ? t('mode_advanced') : t('mode_basic');
  if ($('modeBanner')) $('modeBanner').textContent = t('mode_banner');
  if ($('advHoldHint')) $('advHoldHint').textContent = t('adv_hold_hint');
  if ($('btnModeBasic')) $('btnModeBasic').classList.toggle('active', !advancedMode);
  if ($('btnModeAdvanced')) $('btnModeAdvanced').classList.toggle('active', advancedMode);

  document.querySelectorAll('#outputGrid .io-tile').forEach(tile => {
    tile.classList.add('clickable');
    tile.classList.toggle('locked', !advancedMode);
  });
  document.querySelectorAll('#inputGrid .io-tile').forEach(tile => {
    tile.classList.add('clickable');
    tile.classList.toggle('locked', !advancedMode);
  });
  updateHomeOverview();
}

function updateBleConnectionUi(meta){
  const dot = $('bleDot');
  const text = $('bleText');
  if (!dot || !text) return;
  let detected = false;
  let scanning = false;
  let nextScanIn = -1;

  if (meta && typeof meta === 'object') {
    detected = !!meta.detected;
    scanning = !!meta.scanning;
    const n = Number(meta.nextScanIn);
    nextScanIn = Number.isFinite(n) ? Math.floor(n) : -1;
  } else if (typeof meta === 'boolean') {
    detected = !!meta;
  }

  dot.classList.toggle('ok', detected);
  if (scanning) {
    text.textContent = t('ble_scanning');
  } else if (nextScanIn >= 0) {
    text.textContent = t('ble_next_scan', {sec: nextScanIn});
  } else {
    text.textContent = t('ble_next_scan_na');
  }
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
  const wasAdvanced = advancedMode;
  if (!force && ((mode === 'advanced') === advancedMode)) return;
  if (mode === 'advanced' && !skipConfirm) {
    if (!confirm(t('confirm_advanced'))) {
      updateModeUi();
      return;
    }
  }
  advancedMode = mode === 'advanced';
  updateModeUi();
  if (wasAdvanced && !advancedMode) {
    wsSend({cmd:'clearInputOverrides'});
  }
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

function handleOutputTileHold(tile){
  const id = tile.dataset.outputId;
  if (!id || !advancedMode) return;
  bleDebug('clear output override requested', {id});
  wsSend({cmd:'clearOutputOverride', id});
}

function handleInputTileClick(ev){
  const tile = ev.currentTarget;
  const id = tile.dataset.inputId;
  if (!id) return;
  if (!advancedMode) {
    toast(t('toast_adv_required'), 'error', 4200);
    return;
  }
  bleDebug('toggle input requested', {id});
  wsSend({cmd:'toggleInput', id});
}

function handleInputTileHold(tile){
  const id = tile.dataset.inputId;
  if (!id || !advancedMode) return;
  bleDebug('clear input override requested', {id});
  wsSend({cmd:'clearInputOverride', id});
}

function bindTilePressHandlers(tile, onTap, onHold){
  let holdTimer = null;
  let holdFired = false;

  const startHold = () => {
    if (!advancedMode) return;
    holdFired = false;
    if (holdTimer) clearTimeout(holdTimer);
    holdTimer = setTimeout(() => {
      holdFired = true;
      onHold(tile);
    }, 1000);
  };

  const stopHold = () => {
    if (holdTimer) {
      clearTimeout(holdTimer);
      holdTimer = null;
    }
  };

  tile.addEventListener('mousedown', startHold);
  tile.addEventListener('touchstart', startHold, {passive:true});
  tile.addEventListener('mouseup', stopHold);
  tile.addEventListener('mouseleave', stopHold);
  tile.addEventListener('touchend', stopHold);
  tile.addEventListener('touchcancel', stopHold);
  tile.addEventListener('click', ev => {
    if (holdFired) {
      holdFired = false;
      return;
    }
    onTap(ev);
  });
}

// ─── Init Tiles ───
function initTiles(){
  const ig = $('inputGrid');
  ig.innerHTML = INPUT_IDS.map(id => `
    <div class="io-tile clickable locked" id="in_${id}" data-input-id="${id}">
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
    bindTilePressHandlers(tile, handleOutputTileClick, handleOutputTileHold);
  });
  document.querySelectorAll('#inputGrid .io-tile').forEach(tile => {
    bindTilePressHandlers(tile, handleInputTileClick, handleInputTileHold);
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
    updateBleConnectionUi({detected:false, scanning:false, nextScanIn:-1});
    addLog(t('ws_connected'));
    bleDebug('websocket connected');
    ws.send(JSON.stringify({cmd:'getSettings'}));
    ws.send(JSON.stringify({cmd:'getKeyless'}));
    updateHomeOverview();
  };
  ws.onclose = () => {
    connected = false;
    $('connDot').classList.remove('ok');
    $('connText').textContent = t('conn_disconnected');
    updateBleConnectionUi({detected:false, scanning:false, nextScanIn:-1});
    addLog(t('conn_lost'));
    clearTimeout(reconnTimer);
    reconnTimer = setTimeout(wsConnect, 2000);
    updateHomeOverview();
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
  if (typeof s.turnCalActive === 'boolean') {
    turnDistanceCalibrationActive = s.turnCalActive;
    if (turnDistanceCalibrationActive && typeof s.turnCalPulses === 'number') {
      turnDistanceCalibrationStartPulses =
          Math.max(0, currentSpeedPulses - Number(s.turnCalPulses));
    }
    updateTurnDistanceCalibrationUi();
  }

  // Battery
  const hasVoltage = !!s.voltageAvailable;
  const v = Number(s.voltage);
  $('batVolt').textContent = (hasVoltage && Number.isFinite(v))
      ? (v.toFixed(1) + ' V')
      : t('val_na');

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
  if(ef&32) errs.push('<span class="err-chip warn">'+t('err_input_implausible')+'</span>');
  $('errChips').innerHTML = errs.join('');

  // Inputs
  const ins = s.inputs||{};
  INPUT_IDS.forEach(id => {
    const tile = $('in_'+id);
    if(!tile) return;
    const active = !!ins[id];
    const manual = !!ins[id+'_manual'];
    const manualState = !!ins[id+'_manual_state'];
    tile.classList.toggle('active', active);
    tile.classList.toggle('manual-forced', manual);
    tile.querySelector('.val').textContent = active?'ON':'OFF';
    const extra = ins[id+'_info'];
    if(manual) tile.querySelector('.sub').textContent = manualState ? t('manual_on') : t('manual_off');
    else if(extra) tile.querySelector('.sub').textContent = extra;
    else tile.querySelector('.sub').textContent = active?t('io_active'):t('io_inactive');
  });

  // Outputs
  const outs = s.outputs||{};
  OUTPUT_IDS.forEach(id => {
    const tile = $('out_'+id);
    if(!tile) return;
    const active = !!outs[id];
    const manual = !!outs[id+'_manual'];
    tile.classList.toggle('active', active);
    tile.classList.toggle('manual-forced', manual);
    tile.querySelector('.val').textContent = active?'ON':'OFF';
    const pwm = outs[id+'_pwm'];
    if(manual) tile.querySelector('.sub').textContent = t('manual_on');
    else if(pwm!==undefined && pwm>0) tile.querySelector('.sub').textContent = 'PWM '+Math.round(pwm/255*100)+'%';
    else if (advancedMode) tile.querySelector('.sub').textContent = t('adv_tap_toggle');
    else tile.querySelector('.sub').textContent = active?'\u26a1 '+t('io_active'):t('io_inactive');
  });
  updateHomeOverview();
}

// ─── Settings ───
function applySettings(s){
  $('s_handlebar').value = s.handlebar??0;
  $('s_rear').value      = s.rear??0;
  $('s_turnMode').value  = s.turn??2;
  $('s_brakeMode').value = s.brake??0;
  $('s_alarm').value     = s.alarm??0;
  $('s_posLight').value  = s.pos??0;
  $('s_lowBeam').value   = s.low??0;
  $('s_aux1').value      = s.aux1??0;
  $('s_aux2').value      = s.aux2??0;
  $('s_standKill').value = s.stand??0;
  $('s_parking').value   = s.park??0;
  $('s_drlSource').value = s.drlsrc??0;
  $('s_drlDim').value    = s.drldim??50;
  $('s_turnDist').value  = s.tdist??50;

  const brand = sanitizeProfileText(s.bikeBrand ?? s.brand ?? '');
  const model = sanitizeProfileText(s.bikeModel ?? s.model ?? '');
  const driver = sanitizeProfileText(s.driverName ?? s.driver ?? '');
  profileData = {
    bikeBrand: brand,
    bikeModel: model,
    driverName: driver
  };
  if (typeof s.profileReady === 'boolean') {
    settingsProfileReady = s.profileReady;
  } else {
    settingsProfileReady = profileReadyFromValues(brand, model, driver);
  }
  profileSetupSkipped = !!s.profileSkip;
  firmwareVersion = s.firmware || firmwareVersion;
  syncProfileInputs(brand, model, driver);
  updateTurnDistanceCalibrationUi();
  updateHomeOverview();
  updateProfileOverlay();
}

function saveSettings(opts = {}){
  const {silentToast = false, profileSkip = null} = opts;
  const bikeBrand = sanitizeProfileText($('s_bikeBrand') ? $('s_bikeBrand').value : '');
  const bikeModel = sanitizeProfileText($('s_bikeModel') ? $('s_bikeModel').value : '');
  const driverName = sanitizeProfileText($('s_driverName') ? $('s_driverName').value : '');
  const profileReady = profileReadyFromValues(bikeBrand, bikeModel, driverName);
  let profileSkipValue;
  if (profileSkip === null || profileSkip === undefined) {
    profileSkipValue = profileReady ? false : profileSetupSkipped;
  } else {
    profileSkipValue = !!profileSkip;
  }
  profileSetupSkipped = profileSkipValue;
  settingsProfileReady = profileReady;
  if ($('s_bikeBrand')) $('s_bikeBrand').value = bikeBrand;
  if ($('s_bikeModel')) $('s_bikeModel').value = bikeModel;
  if ($('s_driverName')) $('s_driverName').value = driverName;
  syncProfileInputs(bikeBrand, bikeModel, driverName);
  updateProfileOverlay();
  wsSend({cmd:'setSettings', data:{
    handlebar: +$('s_handlebar').value,
    rear:      +$('s_rear').value,
    turn:      +$('s_turnMode').value,
    brake:     +$('s_brakeMode').value,
    alarm:     +$('s_alarm').value,
    pos:       +$('s_posLight').value,
    low:       +$('s_lowBeam').value,
    aux1:      +$('s_aux1').value,
    aux2:      +$('s_aux2').value,
    stand:     +$('s_standKill').value,
    park:      +$('s_parking').value,
    drlsrc:    +$('s_drlSource').value,
    drldim:    +$('s_drlDim').value,
    tdist:     +$('s_turnDist').value,
    bikeBrand,
    bikeModel,
    driverName,
    profileSkip: profileSkipValue,
  }});
  if (!silentToast) toast(t('toast_saved'),'success');
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
  lastKeylessState = m;
  const ring = $('keylessRing');
  const state = $('keylessState');
  const detail = $('keylessDetail');
  const level = m.rssiLevel !== undefined
      ? normalizeRssiLevel(m.rssiLevel)
      : rssiLevelForDbm(m.rssiThreshold);
  const activeMinutes = m.activeMinutes !== undefined
      ? normalizeActiveMinutes(m.activeMinutes)
      : activeMinutesFromLegacyGrace(m.graceSeconds || 300);
  const activationMode = normalizeActivationMode(m.activationMode ?? 0);
  const activationButton = normalizeActivationButton(m.activationButton ?? 0);
  updateBleConnectionUi({
    detected: (m.statusDetected !== undefined) ? !!m.statusDetected : !!m.phoneDetected,
    scanning: !!m.autoSearching || !!m.sessionRefreshSearching,
    nextScanIn: m.nextScanIn
  });

  if (keylessPendingConfig) {
    const ack =
        keylessPendingConfig.enabled === !!m.enabled
        && keylessPendingConfig.rssiLevel === level
        && keylessPendingConfig.activeMinutes === activeMinutes
        && keylessPendingConfig.activationMode === activationMode
        && keylessPendingConfig.activationButton === activationButton;
    if (ack) {
      keylessPendingConfig = null;
      keylessUiLockUntil = 0;
    }
  }

  const en = $('s_keylessEn');
  const rssiInput = $('s_rssiLevel');
  const activeInput = $('s_activeTime');
  const actModeInput = $('s_actMode');
  const actButtonInput = $('s_actButton');
  const allowSync = !isKeylessUiLocked();

  if (en && allowSync && document.activeElement !== en) {
    en.checked = !!m.enabled;
  }
  if (rssiInput && allowSync && document.activeElement !== rssiInput) {
    rssiInput.value = level;
    updateRssiLevelUi(m.rssiThresholdDbm ?? m.rssiThreshold);
  } else {
    updateRssiLevelUi();
  }
  if (activeInput && allowSync && document.activeElement !== activeInput) {
    activeInput.value = String(activeMinutes);
  }
  if (actModeInput && allowSync && document.activeElement !== actModeInput) {
    actModeInput.value = String(activationMode);
  }
  if (actButtonInput && allowSync && document.activeElement !== actButtonInput) {
    actButtonInput.value = activationButton;
  }
  updateKeylessActivationUi();

  if (m.scanning || m.autoSearching) {
    const count = Array.isArray(m.scanResults) ? m.scanResults.length : 0;
    const sig = `scan:${m.scanning}|auto:${!!m.autoSearching}|refresh:${!!m.sessionRefreshSearching}|count:${count}|ready:${m.scanReady}|running:${m.scannerRunning}`;
    if (sig !== lastBleScanSignature) {
      lastBleScanSignature = sig;
      bleDebug('scan status update', {
        scanning: m.scanning,
        autoSearching: !!m.autoSearching,
        sessionRefreshSearching: !!m.sessionRefreshSearching,
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
        <div class="rssi">${(Number.isFinite(Number(d.rssi)) && Number(d.rssi) < 0 && Number(d.rssi) > -127)
            ? `L${rssiLevelForDbm(Number(d.rssi))} (${Number(d.rssi)} dBm)` : '\u2013'}</div>
        <button class="btn danger" style="padding:5px 10px;font-size:.7rem" onclick="removePaired('${d.mac}')">\u2715</button>
      </div>`).join('');
  } else {
    pdiv.innerHTML = '<div style="color:var(--dim);font-size:.82rem;padding:12px 0">'+t('no_paired')+'</div>';
  }

  // Status
  const hasPaired = Array.isArray(m.paired) && m.paired.length > 0;
  const sessionActive = !!m.sessionActive;
  const waitingForButton = !!m.waitingForButton;
  const sessionRemaining = Math.max(0, Math.floor(Number(m.sessionRemaining) || 0));
  const sessionTime = formatSessionTime(sessionRemaining);
  const curRssi = Number.isFinite(Number(m.currentRssi)) ? Number(m.currentRssi) : null;
  const curLvl = curRssi !== null ? rssiLevelForDbm(curRssi) : '?';

  if(!m.enabled){
    ring.classList.remove('active');
    ring.innerHTML = '\ud83d\udd12';
    state.textContent = t('keyless_disabled');
    state.style.color = '';
    detail.textContent = t('keyless_disabled_detail');
  } else if (!hasPaired) {
    ring.classList.remove('active');
    ring.innerHTML = '\ud83d\udd12';
    state.textContent = t('keyless_locked');
    state.style.color = 'var(--danger)';
    detail.textContent = t('keyless_no_device');
  } else if(sessionActive){
    ring.classList.add('active');
    ring.innerHTML = m.sessionRefreshSearching ? '\ud83d\udd0d' : '\ud83d\udd13';
    state.textContent = m.sessionRefreshSearching
        ? t('keyless_scan_active')
        : (waitingForButton ? t('keyless_waiting_button') : t('keyless_unlocked'));
    state.style.color = m.sessionRefreshSearching ? 'var(--warn)' : 'var(--accent)';
    if (m.sessionRefreshSearching) {
      detail.textContent = t('keyless_scan_active_detail', {time: sessionTime});
    } else if (waitingForButton) {
      const buttonLabel = activationMode === 1
          ? activationButtonLabel(activationButton)
          : t('opt_act_mode_any');
      detail.textContent = t('keyless_waiting_button_detail', {button: buttonLabel});
    } else if (curRssi !== null) {
      detail.textContent = t('keyless_unlocked_detail', {
        level: curLvl,
        rssi: curRssi,
        time: sessionTime
      });
    } else {
      detail.textContent = t('keyless_unlocked_time_only', {time: sessionTime});
    }
  } else if(!!m.autoSearching){
    ring.classList.add('active');
    ring.innerHTML = '\ud83d\udd0d';
    state.textContent = t('keyless_searching');
    state.style.color = 'var(--warn)';
    detail.textContent = t('keyless_searching_detail');
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
  updateHomeOverview();
}

function sendKeylessConfig(){
  const cfg = {
    enabled: $('s_keylessEn').checked,
    rssiLevel: normalizeRssiLevel($('s_rssiLevel').value),
    activeMinutes: normalizeActiveMinutes($('s_activeTime').value || 5),
    activationMode: normalizeActivationMode($('s_actMode').value || 0),
    activationButton: activationButtonIndex($('s_actButton').value || 'start')
  };
  $('s_activeTime').value = String(cfg.activeMinutes);
  $('s_actMode').value = String(cfg.activationMode);
  $('s_actButton').value = normalizeActivationButton($('s_actButton').value || 'start');
  updateKeylessActivationUi();
  keylessPendingConfig = {
    ...cfg,
    activationButton: normalizeActivationButton($('s_actButton').value || 'start')
  };
  keylessUiLockUntil = Date.now() + 3000;
  wsSend({cmd:'setKeyless', data: cfg});
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

function infoTitleKeyFromInfoKey(infoKey){
  if (!infoKey || typeof infoKey !== 'string') return 'tab_settings';
  if (!infoKey.startsWith('info_')) return 'tab_settings';
  return 'card_' + infoKey.slice(5);
}

function renderInfoOverlay(){
  const overlay = $('infoOverlay');
  const title = $('infoOverlayTitle');
  const body = $('infoOverlayBody');
  const hint = $('infoOverlayHint');
  if (!overlay || !title || !body || !hint || !activeInfoKey) return;
  title.textContent = t(infoTitleKeyFromInfoKey(activeInfoKey));
  body.textContent = t(activeInfoKey);
  hint.textContent = t('info_overlay_hint');
}

function showInfo(key){
  activeInfoKey = key;
  renderInfoOverlay();
  const overlay = $('infoOverlay');
  if (overlay) overlay.classList.add('show');
}

function closeInfoOverlay(){
  const overlay = $('infoOverlay');
  if (overlay) overlay.classList.remove('show');
  activeInfoKey = '';
}

document.addEventListener('keydown', ev => {
  if (ev.key === 'Escape') closeInfoOverlay();
});

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
