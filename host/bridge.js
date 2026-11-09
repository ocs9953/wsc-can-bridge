#!/usr/bin/env node
'use strict';

const dgram = require('dgram');
const net = require('net');
const os = require('os');
const crypto = require('crypto');
const { spawn } = require('child_process');


const CUBE = 'C:\\ST\\STM32CubeIDE_2.2.0\\STM32CubeIDE\\plugins';
const DEFAULTS = {
  openocd: process.env.OPENOCD ||
    `${CUBE}\\com.st.stm32cube.ide.mcu.externaltools.openocd.win32_2.4.500.202604080855\\tools\\bin\\openocd.exe`,
  scripts: process.env.OPENOCD_SCRIPTS ||
    `${CUBE}\\com.st.stm32cube.ide.mcu.debug.openocd_2.3.400.202606220929\\resources\\openocd\\st_scripts`,
  sn: process.env.WSC_BOARD_SN || '7',
  rttPort: 19021,
  bitrate: 500,
  bus: 13,
  proto: 1,
  ttl: 0,
};

const GROUP = '239.255.60.60';
const PORT = 4876;
const RTT_CB_ADDR = '0x20000000';
const SLCAN_BITRATE = { 10: 0, 20: 1, 50: 2, 100: 3, 125: 4, 250: 5, 500: 6, 800: 7, 1000: 8 };

const HELP = `
WSC CAN 브리지 — 송신보드(FDCAN1) ↔ Profinity

  node bridge.js [옵션]

  --bitrate <kbps>   CAN 비트레이트 (10 20 50 100 125 250 500 800 1000, 기본 ${DEFAULTS.bitrate})
  --bus <n>          Profinity 버스 번호 (기본 ${DEFAULTS.bus})
  --proto <1|2>      Tritium 프로토콜 버전 (기본 ${DEFAULTS.proto})
  --listen-only      수신 전용 — 보드가 ACK 도 송신도 하지 않는다
  --iface <IPv4>     멀티캐스트 인터페이스 (기본: Profinity 하트비트가 오는 곳)
  --ttl <n>          멀티캐스트 TTL (기본 0 = 이 PC 밖으로 안 나감)
  --allow-remote     다른 PC 가 보낸 UDP 도 버스로 내보낸다 (기본: 이 PC 것만)
  --sn <serial>      ST-Link 일련번호 (기본: 환경변수 WSC_BOARD_SN, 없으면 "${DEFAULTS.sn}")
  --rtt-port <n>     OpenOCD RTT 서버 포트 (기본 ${DEFAULTS.rttPort})
  --no-openocd       OpenOCD 를 띄우지 않고 이미 열린 RTT 포트에 붙는다
  --verbose          프레임을 하나하나 출력
  --duration <초>    정해진 시간 뒤 Ctrl+C 와 같은 방법으로 끝낸다 (점검용)
`;

function parseArgs(argv) {
  const o = { ...DEFAULTS, listenOnly: false, iface: null, allowRemote: false, spawnOpenocd: true, verbose: false };
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i], v = () => {
      if (i + 1 >= argv.length) die(`${a} 에 값이 필요합니다`);
      return argv[++i];
    };
    switch (a) {
      case '--bitrate': o.bitrate = Number(v()); break;
      case '--bus': o.bus = Number(v()); break;
      case '--proto': o.proto = Number(v()); break;
      case '--listen-only': o.listenOnly = true; break;
      case '--iface': o.iface = v(); break;
      case '--ttl': o.ttl = Number(v()); break;
      case '--allow-remote': o.allowRemote = true; break;
      case '--sn': o.sn = v(); break;
      case '--rtt-port': o.rttPort = Number(v()); break;
      case '--no-openocd': o.spawnOpenocd = false; break;
      case '--verbose': o.verbose = true; break;
      case '--duration': o.duration = Number(v()); break;
      case '-h': case '--help': console.log(HELP); process.exit(0);
      default: die(`알 수 없는 옵션: ${a}\n${HELP}`);
    }
  }
  if (!(o.bitrate in SLCAN_BITRATE)) die(`지원하지 않는 비트레이트: ${o.bitrate}`);
  if (o.proto !== 1 && o.proto !== 2) die('--proto 는 1 또는 2');
  if (!Number.isInteger(o.bus) || o.bus < 0 || o.bus > (o.proto === 1 ? 15 : 65535))
    die(`버스 번호 범위 밖: ${o.bus}`);
  if (!Number.isInteger(o.ttl) || o.ttl < 0 || o.ttl > 255) die(`TTL 범위 밖: ${o.ttl}`);
  return o;
}

function log(...a) { console.log(new Date().toISOString().slice(11, 23), ...a); }
function die(msg) { console.error(`[오류] ${msg}`); process.exit(1); }


const V1_MARK = Buffer.from('547269746975', 'hex');
const V2_MARK = Buffer.from('547269fdd6', 'hex');

function busIdBytes(proto, bus) {
  const b = Buffer.alloc(8);
  if (proto === 1) { V1_MARK.copy(b, 1); b[7] = 0x60 | bus; }
  else { V2_MARK.copy(b, 1); b.writeUInt16BE(bus, 6); }
  return b;
}

function parseBusId(p) {
  if (p.subarray(1, 7).equals(V1_MARK) && (p[7] & 0xf0) === 0x60) return { proto: 1, bus: p[7] & 0x0f };
  if (p.subarray(1, 6).equals(V2_MARK)) return { proto: 2, bus: p.readUInt16BE(6) };
  return null;
}

function encodeDatagram(hdr, f) {
  const p = Buffer.alloc(30);
  hdr.copy(p, 0);
  p.writeUInt32BE(f.id >>> 0, 16);
  p[20] = (f.ext ? 0x01 : 0) | (f.rtr ? 0x02 : 0);
  p[21] = f.len;
  if (!f.rtr) f.data.copy(p, 22, 0, f.len);
  return p;
}


const hex = (n, w) => n.toString(16).toUpperCase().padStart(w, '0');

function frameToSlcan(f) {
  const t = f.ext ? (f.rtr ? 'R' : 'T') : (f.rtr ? 'r' : 't');
  const id = f.ext ? hex(f.id, 8) : hex(f.id, 3);
  const data = f.rtr ? '' : f.data.subarray(0, f.len).toString('hex').toUpperCase();
  return `${t}${id}${f.len}${data}\r`;
}

function slcanToFrame(s) {
  const ext = s[0] === 'T' || s[0] === 'R';
  const rtr = s[0] === 'r' || s[0] === 'R';
  const w = ext ? 8 : 3;
  const id = parseInt(s.substr(1, w), 16);
  const len = parseInt(s[1 + w], 16);
  if (Number.isNaN(id) || !(len >= 0 && len <= 8)) return null;
  const data = Buffer.alloc(8);
  if (!rtr) {
    const h = s.substr(2 + w, len * 2);
    if (h.length !== len * 2) return null;
    Buffer.from(h, 'hex').copy(data);
  }
  return { id, ext, rtr, len, data };
}

function fmtFrame(f) {
  const id = f.ext ? hex(f.id, 8) : hex(f.id, 3);
  return `${id}${f.rtr ? ' RTR' : ''} [${f.len}] ${f.rtr ? '' : f.data.subarray(0, f.len).toString('hex')}`;
}


const opt = parseArgs(process.argv.slice(2));
const localIps = new Set(['127.0.0.1']);
for (const list of Object.values(os.networkInterfaces()))
  for (const i of list || []) if (i.family === 'IPv4') localIps.add(i.address);

const clientId = Buffer.concat([Buffer.from([0x00, 0x57]), crypto.randomBytes(6)]);
const hdr = Buffer.concat([busIdBytes(opt.proto, opt.bus), clientId]);

const st = { canToUdp: 0, udpToCan: 0, txRejected: 0, ignoredRemote: 0, ignoredOtherProto: 0 };
let openocd = null, rtt = null, udp = null, synced = false, quitting = false;
let lastBoardStatus = 0, lastStatusLine = '';

main().catch(e => die(e.message));

async function main() {
  log(`버스 ${opt.bus} (프로토콜 v${opt.proto}), ${opt.bitrate} kbps${opt.listenOnly ? ', 수신 전용' : ''}`);
  const iface = opt.iface || await findProfinity() || firstExternalIp();
  if (!iface) die('멀티캐스트에 쓸 IPv4 인터페이스가 없습니다. --iface 로 지정하세요');
  if (!localIps.has(iface)) die(`--iface ${iface} 는 이 PC 의 주소가 아닙니다`);

  if (opt.spawnOpenocd) await startOpenocd();
  await connectRtt();
  await startUdp(iface);
  setInterval(watchdog, 1000).unref();
  for (const sig of ['SIGINT', 'SIGHUP', 'SIGBREAK', 'SIGTERM']) process.on(sig, shutdown);
  if (opt.duration > 0) setTimeout(shutdown, opt.duration * 1000);
}

process.on('exit', () => { if (openocd && openocd.exitCode === null) openocd.kill(); });

function findProfinity() {
  return new Promise(resolve => {
    const s = dgram.createSocket({ type: 'udp4', reuseAddr: true });
    const done = ip => { clearTimeout(t); try { s.close(); } catch (_) {} resolve(ip); };
    const t = setTimeout(() => {
      log('[주의] Profinity 하트비트가 3초 동안 없습니다. Profinity 에서 Virtual CAN to Ethernet Bridge');
      log(`       어댑터가 연결돼 있고 버스 번호가 ${opt.bus} 인지 확인하세요. 일단 계속합니다.`);
      done(null);
    }, 3000);
    s.on('error', e => { log(`[주의] 하트비트 탐색 실패: ${e.message}`); done(null); });
    s.on('message', (m, r) => {
      if (m.length < 30 || !(m[20] & 0x80) || !localIps.has(r.address)) return;
      const b = parseBusId(m);
      if (!b || b.bus !== opt.bus) return;
      const kbps = m.readUInt16BE(22);
      log(`Profinity 하트비트 확인: ${r.address}, 버스 ${b.bus}, ${kbps} kbps`);
      if (kbps !== opt.bitrate) log(`[주의] Profinity 는 ${kbps} kbps 로 설정돼 있습니다 (브리지 ${opt.bitrate} kbps)`);
      done(r.address);
    });
    s.bind(PORT, () => {
      for (const ip of localIps) { try { s.addMembership(GROUP, ip); } catch (_) {} }
    });
  });
}

function firstExternalIp() {
  for (const list of Object.values(os.networkInterfaces()))
    for (const i of list || []) if (i.family === 'IPv4' && !i.internal) return i.address;
  return null;
}


function startOpenocd() {
  return new Promise((resolve, reject) => {
    const args = [
      '-s', opt.scripts,
      '-c', `adapter serial ${opt.sn}`,
      '-c', 'set CONNECT_UNDER_RESET 1',
      '-f', 'interface/stlink-dap.cfg',
      '-f', 'target/stm32u5x.cfg',
      '-c', 'bindto 127.0.0.1',
      '-c', 'gdb_port disabled', '-c', 'telnet_port disabled', '-c', `tcl_port ${opt.rttPort + 1}`,
      '-c', 'init',
      '-c', 'reset run',
      '-c', 'sleep 300',
      '-c', `rtt setup ${RTT_CB_ADDR} 64 "SEGGER RTT"`,
      '-c', 'rtt polling_interval 2',
      '-c', 'rtt start',
      '-c', `rtt server start ${opt.rttPort} 0`,
    ];
    log(`OpenOCD 시작 (ST-Link SN ${opt.sn})`);
    openocd = spawn(opt.openocd, args, { stdio: ['ignore', 'pipe', 'pipe'] });
    let out = '', settled = false;
    const fail = msg => { if (!settled) { settled = true; reject(new Error(msg)); } };
    const onData = d => {
      out += d.toString();
      if (!settled && /Control block found at/i.test(out)) {
        settled = true;
        log('RTT 연결됨');
        resolve();
      } else if (/No control block found/i.test(out)) {
        fail('보드에서 RTT 제어 블록을 찾지 못했습니다. 브리지 펌웨어가 구워져 실행 중인지 확인하세요 (firmware\\build.ps1 -Flash)');
      }
    };
    openocd.stdout.on('data', onData);
    openocd.stderr.on('data', onData);
    openocd.on('error', e => fail(`OpenOCD 실행 실패: ${e.message}`));
    openocd.on('exit', code => {
      const tail = out.trim().split(/\r?\n/).slice(-6).join('\n  ');
      if (!settled) fail(`OpenOCD 가 종료됐습니다 (code ${code})\n  ${tail}`);
      else if (!quitting) die(`OpenOCD 가 종료됐습니다 (code ${code}) — ST-Link 연결을 확인하세요\n  ${tail}`);
    });
    setTimeout(() => fail(`OpenOCD 응답 없음\n  ${out.trim().split(/\r?\n/).slice(-6).join('\n  ')}`), 15000);
  });
}

function connectRtt() {
  return new Promise((resolve, reject) => {
    let tries = 0;
    const attempt = () => {
      const s = net.connect(opt.rttPort, '127.0.0.1');
      s.once('connect', () => { rtt = s; s.setNoDelay(true); onRttConnected(); resolve(); });
      s.once('error', e => {
        if (++tries >= 20) reject(new Error(`RTT 포트 ${opt.rttPort} 에 연결 실패: ${e.message}`));
        else setTimeout(attempt, 250);
      });
    };
    attempt();
  });
}

const pending = [];
let rxBuf = '';
let nonce = '';

function sendCmd(cmd, kind) {
  pending.push(kind);
  rtt.write(cmd + '\r');
}

function onRttConnected() {
  rtt.on('data', d => onRttData(d.toString('latin1')));
  rtt.on('close', () => { if (!quitting) die('RTT 연결이 끊겼습니다'); });
  rtt.on('error', e => { if (!quitting) die(`RTT 오류: ${e.message}`); });
  startSession();
}

function startSession() {
  synced = false;
  pending.length = 0;
  nonce = crypto.randomBytes(4).toString('hex');
  rtt.write(`C\rS${SLCAN_BITRATE[opt.bitrate]}\r#E${nonce}\r`);
}

function onRttData(text) {
  rxBuf += text;
  let i;
  while ((i = rxBuf.search(/[\r\x07]/)) >= 0) {
    const line = rxBuf.slice(0, i);
    const bell = rxBuf[i] === '\x07';
    rxBuf = rxBuf.slice(i + 1);
    if (bell) {
      if (line) handleLine(line);
      handleResponse(false);
    } else {
      handleLine(line);
    }
  }
  if (rxBuf.length > 4096) rxBuf = '';
}

function handleLine(line) {
  if (!synced) {
    if (line === `#E${nonce}`) {
      synced = true;
      sendCmd(opt.listenOnly ? 'L' : 'O', 'open');
    }
    return;
  }
  if (line === '') return handleResponse(true);
  const c = line[0];
  if (line === 'z' || line === 'Z') return handleResponse(true);
  if (c === 't' || c === 'T' || c === 'r' || c === 'R') return onBoardFrame(line);
  if (line.startsWith('#S ')) return onBoardStatus(line);
  if (line.startsWith('#BOOT')) {
    log(`[주의] 보드가 재시작됐습니다 (${line.slice(1)}). 버스를 다시 엽니다`);
    return startSession();
  }
  if (line === '#TIMEOUT') {
    log('[주의] 보드가 생존 신호를 3초간 못 받아 버스를 닫았습니다 (PC 가 멈췄었나?). 다시 엽니다');
    return startSession();
  }
  if (c === '#') return;
  log(`[보드] ${line}`);
}

function handleResponse(ok) {
  const kind = pending.shift();
  if (kind === 'open') {
    if (!ok) die('보드가 버스 열기를 거부했습니다');
    log(`CAN 버스 열림 (${opt.listenOnly ? '수신 전용' : '정상 모드'}, ${opt.bitrate} kbps)`);
  } else if (kind === 'tx' && !ok) {
    st.txRejected++;
  }
}

function onBoardFrame(line) {
  const f = slcanToFrame(line);
  if (!f) return log(`[주의] 해석 못 한 줄: ${line}`);
  if (opt.verbose) log(`CAN→PC  ${fmtFrame(f)}`);
  if (!udp) return;
  udp.send(encodeDatagram(hdr, f), PORT, GROUP);
  st.canToUdp++;
}

function onBoardStatus(line) {
  lastBoardStatus = Date.now();
  const kv = Object.fromEntries(line.slice(3).split(' ').map(p => p.split('=')));
  const key = [kv.rxlost, kv.txdrop, kv.busoff, kv.ep, kv.bo, kv.rttdrop].join(',');
  if (key !== lastStatusLine) {
    if (lastStatusLine)
      log(`[보드] 수신유실 ${kv.rxlost}, 송신버림 ${kv.txdrop}, 버스오프 ${kv.busoff}, ` +
          `TEC ${kv.tec} REC ${kv.rec}${kv.ep === '1' ? ', 에러패시브' : ''}${kv.bo === '1' ? ', 버스오프 중' : ''}`);
    lastStatusLine = key;
  }
}


function startUdp(iface) {
  return new Promise((resolve, reject) => {
    const s = dgram.createSocket({ type: 'udp4', reuseAddr: true });
    s.on('error', e => reject(e));
    s.on('message', onDatagram);
    s.bind(PORT, () => {
      try {
        s.addMembership(GROUP, iface);
        s.setMulticastInterface(iface);
        s.setMulticastLoopback(true);
        s.setMulticastTTL(opt.ttl);
      } catch (e) { return reject(e); }
      udp = s;
      log(`UDP ${GROUP}:${PORT} @ ${iface}, TTL ${opt.ttl}` +
          `${opt.ttl === 0 ? ' (이 PC 안에서만)' : ''}, 클라이언트 ID ${clientId.toString('hex')}`);
      log('중계 시작. 끝내려면 Ctrl+C');
      resolve();
    });
  });
}

function onDatagram(m, r) {
  if (m.length < 30 || (m.length - 16) % 14 !== 0) return;
  const b = parseBusId(m);
  if (!b || b.bus !== opt.bus) return;
  if (m.subarray(8, 16).equals(clientId)) return;
  if (!opt.allowRemote && !localIps.has(r.address)) { st.ignoredRemote++; return; }

  for (let o = 16; o + 14 <= m.length; o += 14) {
    const flags = m[o + 4];
    if (flags & 0xc0) continue;
    if (b.proto !== opt.proto) { st.ignoredOtherProto++; continue; }
    const ext = !!(flags & 0x01);
    const f = {
      id: m.readUInt32BE(o) & (ext ? 0x1fffffff : 0x7ff),
      ext,
      rtr: !!(flags & 0x02),
      len: Math.min(m[o + 5], 8),
      data: Buffer.from(m.subarray(o + 6, o + 14)),
    };
    if (!synced || !rtt) continue;
    if (opt.listenOnly) { st.txRejected++; continue; }
    if (opt.verbose) log(`PC→CAN  ${fmtFrame(f)}`);
    sendCmd(frameToSlcan(f).slice(0, -1), 'tx');
    st.udpToCan++;
  }
}


let lastReport = Date.now();
function watchdog() {
  if (rtt && !rtt.destroyed && !quitting) rtt.write('#K\r');

  if (synced && lastBoardStatus && Date.now() - lastBoardStatus > 5000) {
    log('[주의] 보드 상태 보고가 5초째 없습니다 (보드 정지 또는 RTT 끊김)');
    lastBoardStatus = Date.now();
  }
  if (Date.now() - lastReport >= 10000) {
    lastReport = Date.now();
    log(`CAN→Profinity ${st.canToUdp}, Profinity→CAN ${st.udpToCan}` +
        `${st.txRejected ? `, 송신 거부 ${st.txRejected}` : ''}` +
        `${st.ignoredRemote ? `, 외부 PC 패킷 무시 ${st.ignoredRemote}` : ''}`);
  }
}

function shutdown() {
  if (quitting) process.exit(0);
  quitting = true;
  log('종료 — 버스를 닫습니다');
  if (rtt && !rtt.destroyed) rtt.write('C\r');
  setTimeout(() => {
    if (!openocd || openocd.exitCode !== null) return process.exit(0);
    openocd.once('exit', () => process.exit(0));
    const t = net.connect(opt.rttPort + 1, '127.0.0.1', () => t.write('shutdown\x1a'));
    t.on('error', () => {});
    setTimeout(() => process.exit(0), 2000);
  }, 300);
}
