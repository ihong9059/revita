# /work-start Skill

세션 시작 시 작업 컨텍스트를 로드하고 다음 작업을 안내합니다.

## 실행 절차

1. **작업보고서 확인**
   - 최신 보고서 읽기: `~/revita/작업보고서/` 폴더의 가장 최근 파일
   - 마지막 작업 상태, 다음 단계 확인

2. **개발 가이드 확인**
   - `~/revita/hw_tests/DEV_GUIDE.md` 읽기
   - 빌드/업로드 절차, 알려진 문제 확인

3. **현재 테스트 상태 확인**
   - `~/revita/hw_tests/TEST_STATUS.md` 읽기
   - 완료된 테스트, 진행 중인 테스트 확인

4. **사용자에게 요약 제공**
   - 마지막 작업 내용
   - 다음 작업 제안
   - 알려진 문제점 및 주의사항

---

## 반복 실수 방지 (필수 확인!)

### Bare-metal UART 출력 안됨

**증상**: 코드 빌드/업로드 성공, 하지만 시리얼 출력 없음

**올바른 GPIO 설정**:
```c
P0_PIN_CNF(TX_PIN) = 0x00000003;  // DIR=Output, INPUT=Disconnect
P0_PIN_CNF(RX_PIN) = 0x0000000C;  // DIR=Input, PULL=Pullup
```

**검증 방법**:
```bash
# Zephyr로 하드웨어 먼저 확인
JLinkExe -SelectEmuBySN 001050234191 << EOF
device NRF52840_XXAA
si SWD
speed 4000
connect
erase
loadfile ~/revita/hwTest/serial_counter/build/serial_counter/zephyr/zephyr.hex
r
g
q
EOF
```

### QSPI 핀 매핑 (REVITA_LINK_v1)

| 신호 | 핀 |
|------|-----|
| SCK  | P0.03 |
| CS   | P0.26 |
| IO0  | P0.30 |
| IO1  | P0.29 |
| IO2  | P0.28 |
| IO3  | P0.02 |

---

## 환경 정보

- **PlatformIO**: `~/revita/hw_tests/.venv/bin/platformio`
- **J-Link S/N**: `001050234191`
- **시리얼 포트**: `/dev/ttyUSB0` (115200 baud)

---

## 출력 형식

```
========================================
  REVITA_LINK HW Test - 작업 시작
========================================

## 마지막 작업 (날짜)
- [작업 내용 요약]

## 테스트 진행 상황
- 완료: test_01, test_02, test_15
- 대기: test_03 ~ test_14

## 다음 작업
- [구체적인 작업 내용]

## 주의사항
- Bare-metal UART: GPIO PIN_CNF 설정 확인!
  TX=0x03, RX=0x0C
========================================
```
