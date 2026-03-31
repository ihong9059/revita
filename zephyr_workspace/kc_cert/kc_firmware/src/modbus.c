#include "modbus.h"
#include "rs485.h"
#include "registers.h"
#include <zephyr/kernel.h>
#include <string.h>

uint16_t modbus_crc16(const uint8_t *data, uint16_t len)
{
	uint16_t crc = 0xFFFF;
	for (uint16_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (uint8_t j = 0; j < 8; j++) {
			if (crc & 0x0001) {
				crc = (crc >> 1) ^ 0xA001;
			} else {
				crc >>= 1;
			}
		}
	}
	return crc;
}

bool modbus_check_crc(const uint8_t *frame, uint16_t len)
{
	if (len < 4) {
		return false;
	}
	uint16_t calc = modbus_crc16(frame, len - 2);
	uint16_t recv = frame[len - 2] | (frame[len - 1] << 8);
	return calc == recv;
}

void modbus_append_crc(uint8_t *frame, uint16_t len)
{
	uint16_t crc = modbus_crc16(frame, len);
	frame[len] = crc & 0xFF;
	frame[len + 1] = (crc >> 8) & 0xFF;
}

static void send_exception(uint8_t func, uint8_t exception)
{
	uint8_t resp[5];
	resp[0] = MODBUS_SLAVE_ADDR;
	resp[1] = func | 0x80;
	resp[2] = exception;
	modbus_append_crc(resp, 3);
	rs485_send(resp, 5);
	printk("  TX exception: FC=0x%02X EX=0x%02X\n", func, exception);
}

static void handle_read_holding(const uint8_t *frame, uint16_t len)
{
	if (len < 6) {
		return;
	}

	uint16_t start = (frame[2] << 8) | frame[3];
	uint16_t count = (frame[4] << 8) | frame[5];

	if (count == 0 || count > 125) {
		send_exception(FC_READ_HOLDING, EX_ILLEGAL_VALUE);
		return;
	}

	uint8_t resp[5 + 250]; /* header + data + CRC */
	resp[0] = MODBUS_SLAVE_ADDR;
	resp[1] = FC_READ_HOLDING;
	resp[2] = count * 2;

	for (uint16_t i = 0; i < count; i++) {
		int32_t val = reg_read(start + i);
		if (val < 0) {
			send_exception(FC_READ_HOLDING, EX_ILLEGAL_ADDR);
			return;
		}
		resp[3 + i * 2] = (val >> 8) & 0xFF;
		resp[3 + i * 2 + 1] = val & 0xFF;
	}

	uint16_t resp_len = 3 + count * 2;
	modbus_append_crc(resp, resp_len);
	rs485_send(resp, resp_len + 2);

	printk("  TX read resp: start=0x%04X count=%d\n", start, count);
}

static void handle_write_single(const uint8_t *frame, uint16_t len)
{
	if (len < 6) {
		return;
	}

	uint16_t addr = (frame[2] << 8) | frame[3];
	uint16_t value = (frame[4] << 8) | frame[5];

	int ret = reg_write(addr, value);
	if (ret == -1) {
		send_exception(FC_WRITE_SINGLE, EX_ILLEGAL_ADDR);
		return;
	}
	if (ret == -2) {
		send_exception(FC_WRITE_SINGLE, EX_ILLEGAL_VALUE);
		return;
	}

	/* Echo response */
	uint8_t resp[8];
	memcpy(resp, frame, 6);
	resp[0] = MODBUS_SLAVE_ADDR;
	modbus_append_crc(resp, 6);
	rs485_send(resp, 8);

	printk("  TX write resp: addr=0x%04X val=0x%04X\n", addr, value);
}

static void handle_write_multiple(const uint8_t *frame, uint16_t len)
{
	uint16_t start = (frame[2] << 8) | frame[3];
	uint16_t count = (frame[4] << 8) | frame[5];
	uint8_t byte_count = frame[6];

	if (count == 0 || count > 123 || byte_count != count * 2) {
		send_exception(FC_WRITE_MULTIPLE, EX_ILLEGAL_VALUE);
		return;
	}

	if (len < (uint16_t)(7 + byte_count)) {
		return;
	}

	for (uint16_t i = 0; i < count; i++) {
		uint16_t value = (frame[7 + i * 2] << 8) | frame[7 + i * 2 + 1];
		int ret = reg_write(start + i, value);
		if (ret == -1) {
			send_exception(FC_WRITE_MULTIPLE, EX_ILLEGAL_ADDR);
			return;
		}
		if (ret == -2) {
			send_exception(FC_WRITE_MULTIPLE, EX_ILLEGAL_VALUE);
			return;
		}
	}

	/* Response: addr, func, start, count */
	uint8_t resp[8];
	resp[0] = MODBUS_SLAVE_ADDR;
	resp[1] = FC_WRITE_MULTIPLE;
	resp[2] = (start >> 8) & 0xFF;
	resp[3] = start & 0xFF;
	resp[4] = (count >> 8) & 0xFF;
	resp[5] = count & 0xFF;
	modbus_append_crc(resp, 6);
	rs485_send(resp, 8);

	printk("  TX write_multi resp: start=0x%04X count=%d\n", start, count);
}

void modbus_process_frame(const uint8_t *frame, uint16_t len)
{
	/* Print RX frame */
	printk("RX[%d]:", len);
	for (int i = 0; i < len && i < 32; i++) {
		printk(" %02X", frame[i]);
	}
	if (len > 32) {
		printk(" ...");
	}
	printk("\n");

	/* Check minimum length */
	if (len < 4) {
		printk("  Frame too short\n");
		return;
	}

	/* Check slave address */
	if (frame[0] != MODBUS_SLAVE_ADDR) {
		printk("  Wrong addr: 0x%02X\n", frame[0]);
		return;
	}

	/* Check CRC */
	if (!modbus_check_crc(frame, len)) {
		printk("  CRC error\n");
		return;
	}

	/* Dispatch by function code */
	uint8_t func = frame[1];
	switch (func) {
	case FC_READ_HOLDING:
		handle_read_holding(frame, len - 2);
		break;
	case FC_WRITE_SINGLE:
		handle_write_single(frame, len - 2);
		break;
	case FC_WRITE_MULTIPLE:
		handle_write_multiple(frame, len - 2);
		break;
	default:
		send_exception(func, EX_ILLEGAL_FUNC);
		break;
	}
}
