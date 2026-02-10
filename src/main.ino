/*
 * Wi-Fi Login Sequence Validator using DFA — ESP32
 *
 * System Architecture:
 *   User Device (Browser)
 *         ↓
 *    ESP32 Wi-Fi AP
 *         ↓
 *   Web Login Page
 *         ↓
 *  DFA Engine (ESP32)
 *         ↓
 *  Accept / Reject Login
 *
 * Endpoints:
 *   GET /           → Serves the login HTML page
 *   GET /style.css  → Serves the CSS stylesheet
 *   GET /script.js  → Serves the JavaScript file
 *   GET /input?sym= → Processes DFA input symbol
 *   GET /reset      → Resets DFA to initial state
 *   GET /state      → Returns current DFA state
 */

#include "dfa.h"
#include "wifi_config.h"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>


// Web server on port 80
AsyncWebServer server(80);

// Current DFA state
State currentState = Q0;

// ─── HTML Login Page (embedded) ───
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>DFA Wi-Fi Login Validator</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap');

    * { margin: 0; padding: 0; box-sizing: border-box; }

    body {
      font-family: 'Inter', sans-serif;
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      background: linear-gradient(135deg, #0f0c29 0%, #302b63 50%, #24243e 100%);
      overflow: hidden;
    }

    /* Animated background particles */
    .bg-particles {
      position: fixed;
      top: 0; left: 0; width: 100%; height: 100%;
      pointer-events: none; z-index: 0;
    }
    .bg-particles::before, .bg-particles::after {
      content: '';
      position: absolute;
      border-radius: 50%;
      animation: float 6s ease-in-out infinite;
    }
    .bg-particles::before {
      width: 300px; height: 300px;
      background: radial-gradient(circle, rgba(99,102,241,0.15), transparent);
      top: -50px; right: -50px;
    }
    .bg-particles::after {
      width: 400px; height: 400px;
      background: radial-gradient(circle, rgba(168,85,247,0.1), transparent);
      bottom: -100px; left: -100px;
      animation-delay: -3s;
    }
    @keyframes float {
      0%, 100% { transform: translateY(0) scale(1); }
      50% { transform: translateY(-30px) scale(1.05); }
    }

    .login-container {
      position: relative; z-index: 1;
      width: 420px; max-width: 95vw;
      background: rgba(255,255,255,0.05);
      backdrop-filter: blur(20px);
      -webkit-backdrop-filter: blur(20px);
      border: 1px solid rgba(255,255,255,0.1);
      border-radius: 24px;
      padding: 48px 40px;
      box-shadow: 0 25px 50px rgba(0,0,0,0.4);
      animation: slideUp 0.6s ease-out;
    }
    @keyframes slideUp {
      from { opacity: 0; transform: translateY(40px); }
      to { opacity: 1; transform: translateY(0); }
    }

    .logo {
      text-align: center; margin-bottom: 32px;
    }
    .logo .icon {
      width: 64px; height: 64px;
      background: linear-gradient(135deg, #6366f1, #a855f7);
      border-radius: 16px;
      display: inline-flex; align-items: center; justify-content: center;
      font-size: 28px; margin-bottom: 16px;
      box-shadow: 0 8px 24px rgba(99,102,241,0.4);
    }
    .logo h1 {
      color: #fff; font-size: 22px; font-weight: 600;
      letter-spacing: -0.5px;
    }
    .logo p {
      color: rgba(255,255,255,0.5); font-size: 13px;
      margin-top: 6px;
    }

    /* DFA State Indicator */
    .dfa-state {
      display: flex; justify-content: center; gap: 8px;
      margin-bottom: 28px;
    }
    .state-dot {
      width: 36px; height: 36px;
      border-radius: 50%;
      background: rgba(255,255,255,0.08);
      border: 2px solid rgba(255,255,255,0.15);
      display: flex; align-items: center; justify-content: center;
      font-size: 10px; font-weight: 600; color: rgba(255,255,255,0.4);
      transition: all 0.4s cubic-bezier(0.4,0,0.2,1);
      position: relative;
    }
    .state-dot.active {
      background: linear-gradient(135deg, #6366f1, #a855f7);
      border-color: #a855f7;
      color: #fff;
      box-shadow: 0 0 20px rgba(99,102,241,0.5);
      transform: scale(1.15);
    }
    .state-dot.completed {
      background: rgba(34,197,94,0.2);
      border-color: #22c55e;
      color: #22c55e;
    }
    .state-dot.error {
      background: rgba(239,68,68,0.2);
      border-color: #ef4444;
      color: #ef4444;
      animation: shake 0.4s ease-in-out;
    }
    @keyframes shake {
      0%, 100% { transform: translateX(0); }
      25% { transform: translateX(-4px); }
      75% { transform: translateX(4px); }
    }
    .state-connector {
      width: 24px; height: 2px;
      background: rgba(255,255,255,0.1);
      align-self: center;
      border-radius: 1px;
      transition: background 0.4s;
    }
    .state-connector.active {
      background: linear-gradient(90deg, #22c55e, #6366f1);
    }

    .input-group {
      margin-bottom: 20px; position: relative;
    }
    .input-group label {
      display: block; color: rgba(255,255,255,0.7);
      font-size: 13px; font-weight: 500;
      margin-bottom: 8px;
    }
    .input-group input {
      width: 100%; padding: 14px 16px 14px 44px;
      background: rgba(255,255,255,0.06);
      border: 1px solid rgba(255,255,255,0.12);
      border-radius: 12px;
      color: #fff; font-size: 15px;
      font-family: 'Inter', sans-serif;
      transition: all 0.3s;
      outline: none;
    }
    .input-group input:focus {
      border-color: #6366f1;
      background: rgba(99,102,241,0.08);
      box-shadow: 0 0 0 3px rgba(99,102,241,0.15);
    }
    .input-group input::placeholder {
      color: rgba(255,255,255,0.25);
    }
    .input-group .input-icon {
      position: absolute; left: 14px; top: 40px;
      font-size: 16px; opacity: 0.5;
    }
    .input-group input.success {
      border-color: #22c55e;
      background: rgba(34,197,94,0.08);
    }
    .input-group input.error {
      border-color: #ef4444;
      background: rgba(239,68,68,0.08);
    }

    .btn-login {
      width: 100%; padding: 15px;
      background: linear-gradient(135deg, #6366f1, #a855f7);
      border: none; border-radius: 12px;
      color: #fff; font-size: 16px; font-weight: 600;
      font-family: 'Inter', sans-serif;
      cursor: pointer;
      transition: all 0.3s;
      margin-top: 8px;
      position: relative; overflow: hidden;
    }
    .btn-login:hover {
      transform: translateY(-2px);
      box-shadow: 0 8px 25px rgba(99,102,241,0.4);
    }
    .btn-login:active { transform: translateY(0); }
    .btn-login:disabled {
      opacity: 0.5; cursor: not-allowed;
      transform: none; box-shadow: none;
    }

    .result-msg {
      text-align: center; margin-top: 20px;
      padding: 14px; border-radius: 12px;
      font-size: 14px; font-weight: 500;
      display: none; animation: fadeIn 0.3s ease-out;
    }
    @keyframes fadeIn {
      from { opacity: 0; transform: translateY(8px); }
      to { opacity: 1; transform: translateY(0); }
    }
    .result-msg.success {
      display: block;
      background: rgba(34,197,94,0.12);
      border: 1px solid rgba(34,197,94,0.3);
      color: #22c55e;
    }
    .result-msg.error {
      display: block;
      background: rgba(239,68,68,0.12);
      border: 1px solid rgba(239,68,68,0.3);
      color: #ef4444;
    }
    .result-msg.info {
      display: block;
      background: rgba(99,102,241,0.12);
      border: 1px solid rgba(99,102,241,0.3);
      color: #818cf8;
    }

    .btn-reset {
      display: block; margin: 16px auto 0;
      padding: 10px 28px;
      background: rgba(255,255,255,0.06);
      border: 1px solid rgba(255,255,255,0.12);
      border-radius: 10px;
      color: rgba(255,255,255,0.6);
      font-size: 13px; font-weight: 500;
      font-family: 'Inter', sans-serif;
      cursor: pointer;
      transition: all 0.3s;
    }
    .btn-reset:hover {
      background: rgba(255,255,255,0.1);
      color: #fff;
    }

    .footer {
      text-align: center; margin-top: 24px;
      color: rgba(255,255,255,0.3);
      font-size: 11px;
    }
  </style>
</head>
<body>
  <div class="bg-particles"></div>

  <div class="login-container">
    <div class="logo">
      <div class="icon">📶</div>
      <h1>DFA Wi-Fi Login</h1>
      <p>Sequence Validator • ESP32</p>
    </div>

    <!-- DFA State Indicator -->
    <div class="dfa-state">
      <div class="state-dot active" id="dot-q0">Q0</div>
      <div class="state-connector" id="conn-01"></div>
      <div class="state-dot" id="dot-q1">Q1</div>
      <div class="state-connector" id="conn-12"></div>
      <div class="state-dot" id="dot-q2">Q2</div>
      <div class="state-connector" id="conn-23"></div>
      <div class="state-dot" id="dot-q3">Q3</div>
    </div>

    <div class="input-group">
      <label>Username</label>
      <span class="input-icon">👤</span>
      <input type="text" id="username" placeholder="Enter username"
             onfocus="onFieldFocus('username')">
    </div>

    <div class="input-group">
      <label>Password</label>
      <span class="input-icon">🔒</span>
      <input type="password" id="password" placeholder="Enter password"
             onfocus="onFieldFocus('password')">
    </div>

    <button class="btn-login" id="btnLogin" onclick="handleSubmit()">
      Login →
    </button>

    <div class="result-msg" id="resultMsg"></div>

    <button class="btn-reset" onclick="handleReset()">↺ Reset DFA</button>

    <div class="footer">
      DFA = (Q, Σ, δ, q₀, F) • Deterministic Finite Automaton
    </div>
  </div>

  <script>
    let dfaState = 'Q0';
    let usernameProcessed = false;
    let passwordProcessed = false;

    function updateStateUI(state) {
      dfaState = state;
      const dots = ['q0','q1','q2','q3'];
      const conns = ['01','12','23'];
      const stateIndex = {'Q0':0,'Q1':1,'Q2':2,'Q3':3,'QE':-1};
      const idx = stateIndex[state];

      dots.forEach((d, i) => {
        const el = document.getElementById('dot-' + d);
        el.className = 'state-dot';
        if (idx === -1) {
          el.classList.add('error');
        } else if (i < idx) {
          el.classList.add('completed');
        } else if (i === idx) {
          el.classList.add('active');
        }
      });

      conns.forEach((c, i) => {
        const el = document.getElementById('conn-' + c);
        el.className = 'state-connector';
        if (idx > i && idx !== -1) el.classList.add('active');
      });
    }

    async function sendSymbol(sym) {
      try {
        const resp = await fetch('/input?sym=' + sym);
        const text = await resp.text();
        return text;
      } catch (e) {
        return 'Error';
      }
    }

    function showResult(msg, type) {
      const el = document.getElementById('resultMsg');
      el.textContent = msg;
      el.className = 'result-msg ' + type;
    }

    async function onFieldFocus(field) {
      if (field === 'username' && !usernameProcessed) return;
      if (field === 'password' && !passwordProcessed) return;
    }

    // Process username when user leaves the field
    document.getElementById('username').addEventListener('blur', async function() {
      if (usernameProcessed) return;
      const val = this.value.trim();
      if (val.length === 0) {
        showResult('⚠ Username cannot be empty', 'error');
        this.classList.add('error');
        return;
      }
      const result = await sendSymbol('u');
      usernameProcessed = true;
      if (result === 'Continue') {
        this.classList.add('success');
        updateStateUI('Q1');
        showResult('✓ Username accepted → DFA moved to Q1', 'info');
      } else {
        this.classList.add('error');
        updateStateUI('QE');
        showResult('✗ Invalid sequence! DFA → Error State', 'error');
      }
    });

    // Process password when user leaves the field
    document.getElementById('password').addEventListener('blur', async function() {
      if (passwordProcessed) return;
      const val = this.value.trim();
      if (val.length === 0) return;
      const result = await sendSymbol('p');
      passwordProcessed = true;
      if (result === 'Continue') {
        this.classList.add('success');
        updateStateUI('Q2');
        showResult('✓ Password accepted → DFA moved to Q2', 'info');
      } else {
        this.classList.add('error');
        updateStateUI('QE');
        showResult('✗ Invalid sequence! DFA → Error State', 'error');
      }
    });

    async function handleSubmit() {
      const result = await sendSymbol('s');
      if (result === 'Login Successful') {
        updateStateUI('Q3');
        showResult('🎉 Login Successful! DFA reached accepting state Q3', 'success');
        document.getElementById('btnLogin').disabled = true;
        document.getElementById('btnLogin').textContent = '✓ Authenticated';
      } else {
        updateStateUI('QE');
        showResult('✗ Invalid sequence! Submit rejected. DFA → Error State', 'error');
      }
    }

    async function handleReset() {
      await fetch('/reset');
      usernameProcessed = false;
      passwordProcessed = false;
      document.getElementById('username').value = '';
      document.getElementById('password').value = '';
      document.getElementById('username').className = '';
      document.getElementById('password').className = '';
      document.getElementById('btnLogin').disabled = false;
      document.getElementById('btnLogin').textContent = 'Login →';
      document.getElementById('resultMsg').className = 'result-msg';
      updateStateUI('Q0');
    }
  </script>
</body>
</html>
)rawliteral";

// ─── Setup ───
void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println(" DFA Wi-Fi Login Sequence Validator");
  Serial.println(" ESP32 Access Point Mode");
  Serial.println("========================================");

  // Start Wi-Fi Access Point
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("AP SSID: ");
  Serial.println(WIFI_SSID);
  Serial.print("AP IP:   ");
  Serial.println(WiFi.softAPIP());
  Serial.println("----------------------------------------");

  // Serve login page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  // DFA input endpoint
  server.on("/input", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->hasParam("sym")) {
      request->send(400, "text/plain", "Missing symbol parameter");
      return;
    }

    char symbol = request->getParam("sym")->value()[0];
    State prevState = currentState;
    currentState = transition(currentState, symbol);

    Serial.print("δ(");
    Serial.print(stateName(prevState));
    Serial.print(", '");
    Serial.print(symbol);
    Serial.print("') → ");
    Serial.println(stateName(currentState));

    if (isFinal(currentState)) {
      Serial.println("★ LOGIN SUCCESSFUL — Accepting state reached!");
      request->send(200, "text/plain", "Login Successful");
    } else if (isError(currentState)) {
      Serial.println("✗ INVALID SEQUENCE — Error state reached!");
      request->send(200, "text/plain", "Invalid Sequence");
    } else {
      request->send(200, "text/plain", "Continue");
    }
  });

  // Reset DFA endpoint
  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    currentState = resetDFA();
    Serial.println("↺ DFA Reset → Q0");
    request->send(200, "text/plain", "Reset OK");
  });

  // State query endpoint
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", stateName(currentState));
  });

  server.begin();
  Serial.println("Web server started on port 80");
  Serial.println("Connect to Wi-Fi '" WIFI_SSID "' and open http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("========================================\n");
}

// ─── Loop ───
void loop() {
  // Nothing needed — async web server handles requests
}
