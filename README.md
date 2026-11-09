# WSC CAN 브리지 — 송신보드를 Profinity 용 CAN 어댑터로 쓰기

송신보드의 FDCAN1(Bus A, 차량 CAN)을 ST-Link 로 PC 에 연결해, Profinity 가
WaveSculptor 등 차량 CAN 장치와 통신할 수 있게 한다. PCAN 이나 CAN-Ethernet
브리지 대신 쓰는 것이다.

```
WaveSculptor ──CAN 500k── 송신보드 FDCAN1 ──ST-Link(SWD, RTT)── OpenOCD ── host/bridge.js ──UDP── Profinity
```

- 보드의 ST-Link 커넥터 J1 은 GND/SWDIO/SWCLK/RESET/3.3V 5핀이라 UART(가상 COM 포트)가
  없다. 그래서 SWD 로 RAM 의 링버퍼를 직접 읽고 쓰는 **RTT** 로 데이터를 옮긴다.
- PCAN 은 PEAK 전용 USB 장치·드라이버라 흉내 낼 수 없다. 대신 Profinity 의
  **"Prohelion Virtual CAN to Ethernet Bridge"** 어댑터(Tritium 브리지 프로토콜)에 붙는다.

## 폴더

| 경로 | 내용 |
|---|---|
| `firmware/` | 보드 펌웨어 (STM32U575, HAL, RTOS 없음) |
| `firmware/Core/Src/can_if.c` | FDCAN1 — 필터 없이 전부 수신, 송신 대기열, 버스오프 자동 복구 |
| `firmware/Core/Src/slcan.c` | RTT 위의 SLCAN(Lawicel) 텍스트 명령 |
| `firmware/Core/Src/rtt.c` | SEGGER RTT 호환 링버퍼 (제어 블록 0x20000000 고정) |
| `host/bridge.js` | OpenOCD 실행 + RTT ↔ Profinity UDP 중계 (Node.js, 외부 패키지 없음) |

## 사용 순서

### 1. 펌웨어 굽기 (한 번)

```powershell
cd firmware
.\build.ps1 -Flash
```

> 송신보드의 텔레메트리 펌웨어를 **덮어쓴다.** 되돌리려면
> `WSC_Transmitter_Code_V1\build.ps1 -Flash` 로 다시 굽는다.

ST-Link 선택 규칙은 텔레메트리 펌웨어와 같다: 환경변수 `WSC_BOARD_SN`, 없으면 `7`.
ST-Link 일련번호는 PC마다 다르다.
`STM32_Programmer_CLI -l st-link` 로 확인하고 맞춰 준다:

```powershell
$env:WSC_BOARD_SN = "2"
```

### 2. Profinity 설정

Profinity 프로필의 **Prohelion Virtual CAN to Ethernet Bridge** 어댑터를 연결한다.

| 항목 | 값 |
|---|---|
| Bus No | 13 (브리지 기본값과 같게) |
| Network Interface | IP 가 있는 인터페이스 (지금은 Wi-Fi) |
| Protocol | **UDP** 권장 — 아래 "확인 안 된 것" 참고 |

PCAN 어댑터는 드라이버가 없어 실패하고 있으므로 꺼 두어도 된다.

### 3. 브리지 실행

```powershell
cd host
node bridge.js
```

정상이면 다음이 차례로 나온다:

```
Profinity 하트비트 확인: 10.101.x.x, 버스 13, 500 kbps
OpenOCD 시작 (ST-Link SN 7)
RTT 연결됨
UDP 239.255.60.60:4876 @ 10.101.x.x, TTL 0 (이 PC 안에서만), ...
중계 시작. 끝내려면 Ctrl+C
CAN 버스 열림 (정상 모드, 500 kbps)
```

이후 10초마다 `CAN→Profinity n, Profinity→CAN n` 통계가 나온다. Ctrl+C 로 끝내면
보드는 버스를 닫는다.

### 옵션

| 옵션 | 설명 |
|---|---|
| `--bitrate <kbps>` | 10 20 50 100 125 250 500 800 1000 (기본 500) |
| `--listen-only` | 수신 전용 — ACK 도 송신도 하지 않는다. 처음 차량에 붙여 볼 때 권장 |
| `--bus <n>` | Profinity 버스 번호 (기본 13) |
| `--proto <1\|2>` | Tritium 프로토콜 버전 (기본 1) |
| `--iface <IPv4>` | 멀티캐스트 인터페이스 (기본: Profinity 하트비트가 오는 곳) |
| `--ttl <n>` | 기본 0 = 이 PC 밖으로 안 나감 |
| `--allow-remote` | 다른 PC 가 보낸 UDP 도 차량 버스로 내보낸다 |
| `--verbose` | 프레임을 하나하나 출력 |
| `--no-openocd` | 이미 떠 있는 OpenOCD RTT 서버(`--rtt-port`, 기본 19021)에 붙기만 한다 |

## 안전장치

- **부팅 직후 버스는 닫혀 있다.** 펌웨어는 호스트가 `O` 를 보내기 전까지 ACK 도 하지
  않는다. 닫혀 있는 동안 TX 핀(PA12)은 열성(High)으로 고정한다. PC 없이 차에 꽂혀 있어도
  버스에 영향이 없다.
- **PC 가 사라지면 3초 안에 버스를 닫는다.** 호스트는 1초마다 생존 신호(`#K`)를 보낸다.
  USB 를 뽑거나 프로그램이 강제 종료되면 보드가 스스로 버스를 닫고 대기 중인 송신을 버린다.
  이것이 없으면 ACK 를 못 받은 프레임(예: WaveSculptor 가 꺼져 있을 때 Profinity 가 보낸
  명령)을 보드가 전원이 꺼질 때까지 재전송한다 — 실물 시험에서 실제로 그렇게 됐다.
- **다른 PC 의 패킷은 버스로 내보내지 않는다.** UDP 멀티캐스트는 같은 망의 누구나 보낼
  수 있다. 대회장 Wi-Fi 에서 다른 팀의 Tritium 브리지(같은 기본 버스 13)가 보낸 프레임이
  우리 차량 버스로 나가지 않도록, 이 PC 의 주소에서 온 것만 받는다.
- **차량 데이터를 망으로 내보내지 않는다.** 멀티캐스트 TTL 0 으로 보내 이 PC 안에서만
  전달된다(같은 PC 의 Profinity 에는 전달됨을 확인함).
- **같은 명령이 두 번 나가지 않는다.** Profinity 는 같은 내용을 v1·v2 두 형식으로 보낼 수
  있어서(하트비트가 실제로 두 형식으로 온다), 한 형식(`--proto`)만 버스로 내보낸다.
- 하트비트·설정 플래그(0x80/0x40)가 붙은 패킷은 버스로 내보내지 않는다.

## LED

| LED | 의미 |
|---|---|
| BOARD_CHECK (PE12) | 켜짐 = 버스 열림, 1 Hz 깜빡임 = 닫힘(호스트 대기) |
| CAN (PB15) | 송수신이 있으면 깜빡임 |
| ERR (PE11) | 에러 패시브·버스오프·수신 유실이 생기면 1초간 켜짐. 계속 켜져 있으면 치명 오류 |

## 보드 ↔ PC 프로토콜 (SLCAN)

RTT 채널 0, 줄 끝은 `\r`. 성공 응답 `\r`(송신은 `z\r`/`Z\r`), 실패 `\a`.

| PC → 보드 | 의미 |
|---|---|
| `S0`~`S8` | 10k 20k 50k 100k 125k 250k 500k 800k 1M |
| `O` / `L` / `C` | 열기 / 수신 전용 열기 / 닫기 |
| `tIIILDD..` `TIIIIIIIILDD..` | 표준 / 확장 데이터 프레임 송신 |
| `rIIIL` `RIIIIIIIIL` | 표준 / 확장 리모트 프레임 송신 |
| `#E<텍스트>` | 그대로 돌려줌 (호스트가 이전 세션의 잔여 데이터를 가르는 데 씀) |
| `#K` | 생존 신호, 응답 없음. 버스가 열린 동안 3초간 입력이 없으면 보드가 버스를 닫는다 |

| 보드 → PC | 의미 |
|---|---|
| `t…` `T…` `r…` `R…` | 수신 프레임 |
| `#BOOT …` | 부팅 직후 한 번 (호스트는 이것을 보면 버스를 다시 연다) |
| `#S …` | 1초마다 상태: 수신/송신 수, 유실, 버스오프, TEC/REC |
| `#TIMEOUT` | 생존 신호가 끊겨 버스를 닫음 (호스트는 이것을 보면 다시 연다) |

`telnet` 으로 직접 다룰 때는 3초 안에 다음 명령을 치지 않으면 버스가 닫힌다.

OpenOCD RTT 포트에 `telnet localhost 19021` 로 붙으면 그대로 읽고 명령을 칠 수 있다.

비트 타이밍은 FDCAN 커널 클럭 80 MHz(HSE 16 MHz → PLL1Q) 기준 16 tq, 샘플포인트
81.25%, SJW 3 tq 로 텔레메트리 펌웨어의 500 kbps 설정과 같다(800 kbps 만 20 tq, 80%).

## 검증 상태 (2026-11-06)

실물 시험 구성: 송신보드 + ST-LINK V2(V2J44S7, SN 2), WaveSculptor 미연결, 버스에 다른 노드 없음.

**확인한 것**
- **Tritium 패킷 형식:** 실행 중인 Profinity 2.2.5 가 보내는 하트비트를 직접 받아 바이트 단위로
  맞췄다. 인코딩 결과가 실제 패킷과 일치한다.
- **펌웨어 기동:** 굽기·검증 OK, RTT 제어 블록이 0x20000000 에서 바로 잡힌다.
  FDCAN 커널 클럭은 80 MHz 로 보고된다.
- **SLCAN 명령:** V/N/F/S/O/L/C/#E 응답이 맞고, 지원하지 않는 `S9` 는 거부한다.
- **수신 전용 모드:** 3초간 수신 0개다. 지금 버스에 송신하는 노드가 없다.
- **송신 경로:** 프레임 1개를 보내면 ACK 가 없어 TEC 가 128 에서 멈추고 에러 패시브가 된다
  (`F24`). 버스오프(256)까지 가지 않았으므로 오류는 ACK 부재뿐이고 비트 오류는 없다.
  즉 FDCAN → ADM3053 → 버스 → 다시 읽기의 물리 경로가 정상이다. 닫으면 TEC/REC 가 0 으로 돌아온다.
- **전체 경로:** 실제 보드와 Profinity 를 붙여 돌렸다.
  - 하트비트 탐지, OpenOCD 실행, RTT 연결, 버스 열림까지 약 1초 걸린다.
  - 가짜 클라이언트가 v1·v2 로 넣은 프레임 중 v1 하나만 보드에서 송신된다.
  - 에러 패시브가 PC 화면에 표시된다.
- **생존 신호:** 실행 중에는 버스가 열린 채 유지된다. 호스트를 강제 종료하면 보드가 스스로
  버스를 닫고 재전송을 멈춘다. 남는 OpenOCD 프로세스도 없다.
- **가짜 보드 시험:** 이전 세션 잔여 데이터를 버리는지, 설정 패킷과 다른 버스 번호를 거르는지
  확인했다.

**확인 안 된 것**
- **실제 CAN 상대와의 송수신:** WaveSculptor 가 연결되지 않아 수신 프레임과 ACK 받는 송신은
  아직 보지 못했다. WaveSculptor 를 연결하면 `--listen-only` 로 먼저 수신(0x400대 ID)을
  확인하고, 그다음 정상 모드로 바꾼다.
- **Profinity 가 UDP 로 들어온 프레임을 받아들이는지:** Profinity 가 패킷을 받는 것까지만
  확인했고, 화면에 반영되는지는 보지 못했다(Profinity API 는 로그인이 필요함). 어댑터가
  지금 `Protocol: TCP` 로 설정돼 있다. TCP 쪽은 연결 직후 클라이언트가 보내야 하는 형식이
  공개 문서에서 그림으로만 나와 있어 구현하지 않았다. Profinity 의 CAN 화면에 WaveSculptor 가
  보이지 않으면 먼저 Protocol 을 **UDP** 로 바꿔 볼 것.
- **버스오프 자동 복구:** 실제로 일어나지 않아 확인하지 못했다.
- **RTT 처리량:** 부하를 걸어 재 보지 않았다. 보드 상태 줄의 `rxlost` 가 늘면 PC 가 못
  따라가는 것이다.
