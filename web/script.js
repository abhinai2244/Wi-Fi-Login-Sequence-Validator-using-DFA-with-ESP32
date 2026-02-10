/*
 * DFA Wi-Fi Login Validator — Client Script
 * Handles DFA symbol transmission and UI state updates
 */

// ─── State Tracking ───
let dfaState = 'Q0';
let usernameProcessed = false;
let passwordProcessed = false;

// ─── DOM Elements ───
const usernameInput = document.getElementById('username');
const passwordInput = document.getElementById('password');
const btnLogin = document.getElementById('btnLogin');
const btnReset = document.getElementById('btnReset');
const resultMsg = document.getElementById('resultMsg');

// ─── DFA State UI Update ───
function updateStateUI(state) {
  dfaState = state;
  const dots = ['q0', 'q1', 'q2', 'q3'];
  const conns = ['01', '12', '23'];
  const stateIndex = { 'Q0': 0, 'Q1': 1, 'Q2': 2, 'Q3': 3, 'QE': -1 };
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

// ─── Send Symbol to ESP32 DFA Engine ───
async function sendSymbol(sym) {
  try {
    const resp = await fetch('/input?sym=' + sym);
    const text = await resp.text();
    return text;
  } catch (e) {
    console.error('DFA request failed:', e);
    return 'Error';
  }
}

// ─── Show Result Message ───
function showResult(msg, type) {
  resultMsg.textContent = msg;
  resultMsg.className = 'result-msg ' + type;
}

// ─── Username Field — onBlur → send 'u' ───
usernameInput.addEventListener('blur', async function () {
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

// ─── Password Field — onBlur → send 'p' ───
passwordInput.addEventListener('blur', async function () {
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

// ─── Login Button — onClick → send 's' ───
btnLogin.addEventListener('click', async function () {
  const result = await sendSymbol('s');

  if (result === 'Login Successful') {
    updateStateUI('Q3');
    showResult('🎉 Login Successful! DFA reached accepting state Q3', 'success');
    btnLogin.disabled = true;
    btnLogin.textContent = '✓ Authenticated';
  } else {
    updateStateUI('QE');
    showResult('✗ Invalid sequence! Submit rejected. DFA → Error State', 'error');
  }
});

// ─── Reset Button ───
btnReset.addEventListener('click', async function () {
  await fetch('/reset');

  usernameProcessed = false;
  passwordProcessed = false;

  usernameInput.value = '';
  passwordInput.value = '';
  usernameInput.className = '';
  passwordInput.className = '';

  btnLogin.disabled = false;
  btnLogin.textContent = 'Login →';

  resultMsg.className = 'result-msg';
  updateStateUI('Q0');
});
