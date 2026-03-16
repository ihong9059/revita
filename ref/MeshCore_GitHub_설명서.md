# MeshCore GitHub 저장소 설명서

> **URL**: https://github.com/meshcore-dev/MeshCore

## 1. 프로젝트 개요

MeshCore는 **LoRa 라디오를 사용하는 임베디드 프로젝트를 위한 경량 C++ 라이브러리**입니다. 인터넷 없이 작동하는 탄력적이고 분산화된 통신 네트워크를 구축할 수 있습니다.

### 저장소 통계
| 항목 | 값 |
|------|-----|
| **Stars** | 2,200+ |
| **Forks** | 627+ |
| **Contributors** | 108명 |
| **Releases** | 79개 |
| **라이선스** | MIT |
| **최신 버전** | Companion v1.14.0 |

### 기술 스택
| 언어 | 비율 |
|------|------|
| **C** | 63.2% |
| **C++** | 35.4% |
| **기타** | 1.4% |

---

## 2. 핵심 기능

### 2.1 멀티홉 라우팅 (Multi-Hop Routing)
- 메시지가 중간 노드를 통해 릴레이
- 통신 범위 자동 확장
- 동적 경로 탐색

### 2.2 LoRa 하드웨어 지원
- **Heltec** 시리즈
- **RAK Wireless** (RAK4630 포함)
- 기타 LoRa 모듈

### 2.3 분산형 네트워크
- 중앙 서버 불필요
- 자가 치유(Self-healing) 아키텍처
- 노드 추가/제거 시 자동 재구성

### 2.4 저전력 설계
- 배터리/태양광 전원 장치에 최적화
- 임베디드 환경 특화 설계
- 셋업 단계 외 동적 메모리 할당 없음

### 2.5 사전 빌드된 바이너리
- 컴파일 없이 바로 플래싱 가능
- 웹 플래셔 제공 (https://flasher.meshcore.co.uk)

---

## 3. 지원 장치

### 3.1 RAK Wireless
| 장치 | 칩셋 | 특징 |
|------|------|------|
| **RAK4630** | nRF52840 + SX1262 | BLE + LoRa 지원 |
| **RAK4631** | nRF52840 + SX1262 | WisBlock 모듈 |
| **RAK19007** | - | WisBlock 베이스보드 |

### 3.2 Heltec
- Heltec WiFi LoRa 32 시리즈
- Heltec Wireless Stick
- Heltec Wireless Shell

### 3.3 기타
- LilyGO T-Beam
- TTGO LoRa32
- ESP32 기반 LoRa 모듈

---

## 4. 노드 타입

### 4.1 Companion Radio
- 스마트폰 앱과 BLE/USB/WiFi 연결
- 사용자 인터페이스 제공
- 메시지 송수신 기능

### 4.2 Repeater
- 메시지 릴레이 전용
- 네트워크 범위 확장
- 저전력 운영 가능

### 4.3 Room Server
- 그룹 채팅 서버 기능
- 메시지 저장 및 전달
- CLI 기반 관리

### 4.4 Sensor Node
- IoT 센서 데이터 전송
- 저전력 모드 지원
- 간헐적 데이터 전송

### 4.5 Terminal Chat
- 직접 CLI 채팅
- 시리얼 콘솔 사용
- 디버깅 용도

---

## 5. 개발 환경 설정

### 5.1 필수 도구
- **Visual Studio Code**
- **PlatformIO IDE** (VS Code Extension)
- **Git**

### 5.2 저장소 클론
```bash
git clone https://github.com/meshcore-dev/MeshCore.git
cd MeshCore
```

### 5.3 PlatformIO 프로젝트 열기
1. VS Code 실행
2. PlatformIO Home 열기
3. "Open Project" 선택
4. MeshCore 폴더 선택

---

## 6. 예제 프로젝트

### 6.1 Companion Radio
BLE/USB/WiFi를 통해 스마트폰 앱과 연결되는 라디오

### 6.2 KISS Modem
시리얼 프로토콜 기반 모뎀 구현

### 6.3 Simple Repeater
기본 리피터 기능 구현

### 6.4 Simple Secure Chat
암호화된 채팅 애플리케이션

### 6.5 Sensor Nodes
다양한 센서 데이터 전송 예제

---

## 7. 문서 구조

저장소의 `docs/` 폴더에 다음 문서들이 있습니다:

| 파일 | 설명 |
|------|------|
| `cli_commands.md` | CLI 명령어 레퍼런스 |
| `companion_protocol.md` | 컴패니언 라디오 프로토콜 |
| `faq.md` | 자주 묻는 질문 |
| `kiss_modem_protocol.md` | KISS 모뎀 프로토콜 |
| `nrf52_power_management.md` | nRF52 전력 관리 |
| `packet_format.md` | 패킷 형식 |
| `payloads.md` | 페이로드 구조 |
| `qr_codes.md` | QR 코드 사용법 |
| `stats_binary_frames.md` | 통계 바이너리 프레임 |
| `terminal_chat_cli.md` | 터미널 채팅 CLI |

---

## 8. 빌드 및 플래싱

### 8.1 웹 플래셔 사용 (권장)
1. https://flasher.meshcore.co.uk 접속
2. 장치 선택
3. 펌웨어 유형 선택
4. "Flash" 버튼 클릭

### 8.2 PlatformIO 빌드
```bash
# 프로젝트 빌드
pio run -e <environment_name>

# 플래싱
pio run -e <environment_name> -t upload

# 시리얼 모니터
pio device monitor
```

---

## 9. 클라이언트 연결

### 9.1 스마트폰 앱
- **Android**: Google Play Store
- **iOS**: Apple App Store
- MeshCore Companion 앱 검색

### 9.2 웹 클라이언트
- Web Serial API 사용
- 브라우저에서 직접 연결

### 9.3 Python 클라이언트
```bash
pip install meshcore
meshcore-cli --port /dev/ttyUSB0
```

---

## 10. 커뮤니티 참여

### 10.1 이슈 제출
- 버그 리포트
- 기능 요청
- 문서 개선

### 10.2 풀 리퀘스트
- 코드 기여
- 문서 번역
- 예제 추가

### 10.3 디스코드
- https://discord.gg/BMwCtwHj5V
- 실시간 지원
- 커뮤니티 토론

---

## 11. 라이선스

**MIT License**
- 자유로운 사용, 수정, 배포 가능
- 상업적 사용 허용
- 저작권 표시 필요

---

## 12. 관련 링크

| 항목 | URL |
|------|-----|
| **GitHub** | https://github.com/meshcore-dev/MeshCore |
| **공식 사이트** | https://meshcore.co.uk |
| **펌웨어 플래셔** | https://flasher.meshcore.co.uk |
| **Discord** | https://discord.gg/BMwCtwHj5V |
| **문서** | https://github.com/meshcore-dev/MeshCore/tree/main/docs |

---

*작성일: 2026-03-11*
