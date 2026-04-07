/**
 * @file    tower.c
 * @brief   REVITA TOWER 보드 드라이버 — MCP23017 I/O Expander 제어
 * @details tower.h에 선언된 API의 구현부.
 *          I2C 액세스는 HAL 추상화 계층(tower_hal_i2c_*)을 통해 호출하므로
 *          하부 SDK(nRF5 SDK / Zephyr / NimBLE 등)에 종속되지 않는다.
 *          포팅 시 tower_hal_i2c_write_reg / tower_hal_i2c_read_reg 두 함수만 구현하면 된다.
 */

#include "tower.h"
#include <string.h>

/* ============================================================
 *  HAL 추상화 — 보드 BSP에서 구현해야 함
 *
 *  bool tower_hal_i2c_write_reg(uint8_t addr7, uint8_t reg, uint8_t val);
 *  bool tower_hal_i2c_read_reg (uint8_t addr7, uint8_t reg, uint8_t *val);
 *
 *  반환값: true = 성공, false = NACK / 타임아웃 / 버스 에러
 *  addr7 : 7-bit 슬레이브 주소 (R/W 비트 제외)
 * ============================================================ */
extern bool tower_hal_i2c_write_reg(uint8_t addr7, uint8_t reg, uint8_t val);
extern bool tower_hal_i2c_read_reg (uint8_t addr7, uint8_t reg, uint8_t *val);

/* ============================================================
 *  내부 캐시 — 매 set/clr마다 read-modify-write 하지 않기 위해
 *  현재 OLAT 값을 RAM에 보존한다.
 * ============================================================ */
static uint8_t s_olat_a = MCP_OLATA_INIT;
static uint8_t s_olat_b = MCP_OLATB_INIT;
static bool    s_initialized = false;

/* ============================================================
 *  저수준 헬퍼
 * ============================================================ */
static inline bool mcp_write(uint8_t reg, uint8_t val)
{
    return tower_hal_i2c_write_reg(MCP23017_I2C_ADDR, reg, val);
}

static inline bool mcp_read(uint8_t reg, uint8_t *val)
{
    return tower_hal_i2c_read_reg(MCP23017_I2C_ADDR, reg, val);
}

/* ============================================================
 *  공개 API
 * ============================================================ */

bool tower_mcp_init(void)
{
    /* 1) IOCON 설정
     *    - BANK   = 0 (레지스터 페어 인터리브)
     *    - MIRROR = 1 (INTA/INTB 합쳐서 출력 — 필요 시 사용)
     *    - SEQOP  = 0 (시퀀셜 자동 증가 사용)
     *    - ODR    = 0, INTPOL = 0 (active low INT)
     */
    if (!mcp_write(MCP_REG_IOCON, 0x40)) return false;

    /* 2) 출력 초기값을 먼저 OLAT에 써서 IODIR 변경 시 글리치 방지 */
    if (!mcp_write(MCP_REG_OLATA, MCP_OLATA_INIT)) return false;
    if (!mcp_write(MCP_REG_OLATB, MCP_OLATB_INIT)) return false;
    s_olat_a = MCP_OLATA_INIT;
    s_olat_b = MCP_OLATB_INIT;

    /* 3) 방향 설정 (출력=0, 입력=1) */
    if (!mcp_write(MCP_REG_IODIRA, MCP_IODIRA_INIT)) return false;
    if (!mcp_write(MCP_REG_IODIRB, MCP_IODIRB_INIT)) return false;

    /* 4) 입력 비트는 풀업 활성화 (예약 비트가 플로팅되지 않도록) */
    if (!mcp_write(MCP_REG_GPPUA, MCP_IODIRA_INIT)) return false;
    if (!mcp_write(MCP_REG_GPPUB, MCP_IODIRB_INIT)) return false;

    /* 5) 인터럽트는 기본 비활성화 */
    if (!mcp_write(MCP_REG_GPINTENA, 0x00)) return false;
    if (!mcp_write(MCP_REG_GPINTENB, 0x00)) return false;

    s_initialized = true;
    return true;
}

/* ───────── 일괄 read/write ───────── */

bool tower_mcp_write_gpa(uint8_t value)
{
    if (!mcp_write(MCP_REG_OLATA, value)) return false;
    s_olat_a = value;
    return true;
}

bool tower_mcp_write_gpb(uint8_t value)
{
    if (!mcp_write(MCP_REG_OLATB, value)) return false;
    s_olat_b = value;
    return true;
}

bool tower_mcp_read_gpa(uint8_t *value)
{
    if (value == NULL) return false;
    return mcp_read(MCP_REG_GPIOA, value);
}

bool tower_mcp_read_gpb(uint8_t *value)
{
    if (value == NULL) return false;
    return mcp_read(MCP_REG_GPIOB, value);
}

/* ───────── 비트 set / clear (캐시 사용) ───────── */

bool tower_mcp_gpa_set(uint8_t mask)
{
    uint8_t next = s_olat_a | mask;
    if (next == s_olat_a) return true;            /* 변화 없음 → I2C skip */
    if (!mcp_write(MCP_REG_OLATA, next)) return false;
    s_olat_a = next;
    return true;
}

bool tower_mcp_gpa_clr(uint8_t mask)
{
    uint8_t next = s_olat_a & (uint8_t)~mask;
    if (next == s_olat_a) return true;
    if (!mcp_write(MCP_REG_OLATA, next)) return false;
    s_olat_a = next;
    return true;
}

bool tower_mcp_gpb_set(uint8_t mask)
{
    uint8_t next = s_olat_b | mask;
    if (next == s_olat_b) return true;
    if (!mcp_write(MCP_REG_OLATB, next)) return false;
    s_olat_b = next;
    return true;
}

bool tower_mcp_gpb_clr(uint8_t mask)
{
    uint8_t next = s_olat_b & (uint8_t)~mask;
    if (next == s_olat_b) return true;
    if (!mcp_write(MCP_REG_OLATB, next)) return false;
    s_olat_b = next;
    return true;
}

/* ============================================================
 *  편의 함수 (선택 사용) — 보드 부팅 시퀀스 예시
 * ============================================================
 *
 *  void tower_power_up_sequence(void)
 *  {
 *      tower_mcp_init();
 *      TOWER_3V3_ON();          nrf_delay_ms(10);
 *      TOWER_5V_ON();           nrf_delay_ms(10);
 *      TOWER_12V_ON();          nrf_delay_ms(20);
 *
 *      // LTE 모뎀 리셋 펄스 (≥100ms)
 *      TOWER_LTE_RESET_HOLD();  nrf_delay_ms(150);
 *      TOWER_LTE_RESET_REL();
 *
 *      // SBC 리셋 해제
 *      TOWER_SBC_RESET_REL();
 *
 *      // 카메라 전원
 *      TOWER_CAM_ON();
 *  }
 */
