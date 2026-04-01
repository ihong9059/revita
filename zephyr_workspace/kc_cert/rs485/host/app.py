"""
KC 기능시험 Host - 웹 기반 RS485 Modbus RTU 제어기

Flask 웹서버로 브라우저에서 리비타 링크를 제어하고 상태를 모니터링.
"""

import sys
import os
import struct
import time
import threading

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from flask import Flask, render_template, jsonify, request
import serial
from modbus_crc import append_crc, check_crc

app = Flask(__name__)

# kc_firmware slave address (센서=0x01이므로 0x10 사용)
SLAVE_ADDR = 0x10

# Serial connection
ser = None
ser_lock = threading.Lock()

# Communication log
MAX_LOG = 200
comm_log = []


def log_comm(direction, data_bytes, desc=""):
    hex_str = " ".join(f"{b:02X}" for b in data_bytes)
    entry = {
        "time": time.strftime("%H:%M:%S"),
        "dir": direction,
        "hex": hex_str,
        "desc": desc,
    }
    comm_log.append(entry)
    if len(comm_log) > MAX_LOG:
        comm_log.pop(0)


def send_modbus(frame_no_crc: bytes, desc="") -> bytes | None:
    """Send Modbus frame (auto-appends CRC) and read response."""
    frame = append_crc(frame_no_crc)
    with ser_lock:
        if ser is None or not ser.is_open:
            return None
        ser.reset_input_buffer()
        ser.write(frame)
        log_comm("TX", frame, desc)

        time.sleep(0.05)
        resp = ser.read(256)
        time.sleep(0.02)
        resp += ser.read(256)

    if resp:
        log_comm("RX", resp)
        if check_crc(resp):
            return resp
    return None


def read_registers(start, count, desc=""):
    """Modbus 0x03: Read Holding Registers."""
    frame = struct.pack(">BBHH", SLAVE_ADDR, 0x03, start, count)
    return send_modbus(frame, desc)


def write_register(addr, value, desc=""):
    """Modbus 0x06: Write Single Register."""
    frame = struct.pack(">BBHH", SLAVE_ADDR, 0x06, addr, value & 0xFFFF)
    return send_modbus(frame, desc)


def write_multiple(start, values, desc=""):
    """Modbus 0x10: Write Multiple Registers."""
    count = len(values)
    frame = struct.pack(">BBHHB", SLAVE_ADDR, 0x10, start, count, count * 2)
    for v in values:
        frame += struct.pack(">H", v & 0xFFFF)
    return send_modbus(frame, desc)


def parse_read_response(resp, count):
    """Parse 0x03 response into list of uint16 values."""
    if resp is None or len(resp) < 3 + count * 2 + 2:
        return None
    if resp[1] & 0x80:
        return None
    values = []
    for i in range(count):
        v = (resp[3 + i*2] << 8) | resp[3 + i*2 + 1]
        values.append(v)
    return values


def signed16(v):
    return v - 0x10000 if v >= 0x8000 else v


# ===== Routes =====

@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/connect", methods=["POST"])
def api_connect():
    global ser
    data = request.json
    port = data.get("port", "COM15")
    baud = data.get("baud", 9600)
    try:
        with ser_lock:
            if ser and ser.is_open:
                ser.close()
            ser = serial.Serial(port, baud, timeout=0.3)
        return jsonify({"ok": True, "port": port, "baud": baud})
    except Exception as e:
        return jsonify({"ok": False, "error": str(e)})


@app.route("/api/disconnect", methods=["POST"])
def api_disconnect():
    global ser
    with ser_lock:
        if ser and ser.is_open:
            ser.close()
        ser = None
    return jsonify({"ok": True})


@app.route("/api/status")
def api_status():
    connected = ser is not None and ser.is_open
    return jsonify({"connected": connected,
                     "port": ser.port if connected else None})


@app.route("/api/log")
def api_log():
    return jsonify(comm_log[-50:])


@app.route("/api/log-clear", methods=["POST"])
def api_log_clear():
    comm_log.clear()
    return jsonify({"ok": True})


# --- Test Mode ---
@app.route("/api/test-mode", methods=["POST"])
def api_test_mode():
    enter = request.json.get("enter", True)
    val = 0x0001 if enter else 0x0000
    resp = write_register(0x01FF, val, f"Test Mode {'ENTER' if enter else 'EXIT'}")
    return jsonify({"ok": resp is not None})


# --- Read All Status ---
@app.route("/api/read-all")
def api_read_all():
    result = {}

    # Device info
    r = parse_read_response(read_registers(0x0000, 5, "Device Info"), 5)
    if r:
        result["device"] = {
            "id": f"{r[0]:04X}{r[1]:04X}",
            "fw": f"v{r[2]>>8}.{r[2]&0xFF}",
            "hw": f"v{r[3]>>8}.{r[3]&0xFF}",
            "test_mode": r[4],
        }

    # Power/sensors (0x0010~0x0016, 7 registers)
    r = parse_read_response(read_registers(0x0010, 7, "Power/Sensors"), 7)
    if r:
        changed = r[6]  # 0x0016: input_changed bitmap
        result["power"] = {
            "bat_mv": r[0], "bat_pct": r[1],
            "vib": r[2], "vib_count": r[3],
            "btn": r[4], "v12": r[5],
            "input_changed": changed,
            "btn_changed": bool(changed & 0x01),
            "vib_changed": bool(changed & 0x02),
        }

    # Valves
    r = parse_read_response(read_registers(0x0020, 4, "Valves"), 4)
    if r:
        result["valve"] = {
            "x_state": r[0], "y_state": r[1],
            "x_pin": r[2], "y_pin": r[3],
        }

    # Flow
    r = parse_read_response(read_registers(0x0030, 6, "Flow"), 6)
    if r:
        result["flow"] = {
            "x_count": (r[0] << 16) | r[1],
            "y_count": (r[2] << 16) | r[3],
            "x_active": r[4], "y_active": r[5],
        }

    # Output
    r = parse_read_response(read_registers(0x0040, 2, "Output"), 2)
    if r:
        result["output"] = {"buzzer": r[0], "led": r[1]}

    # LoRa (0x0050~0x005C, 13 registers)
    r = parse_read_response(read_registers(0x0050, 13, "LoRa"), 13)
    if r:
        result["lora"] = {
            "state": r[0], "freq": r[1],
            "power": signed16(r[2]),
            "rssi": signed16(r[3]), "snr": signed16(r[4]) / 10,
            "tx_cnt": r[5], "rx_cnt": r[6],
            "rx_err": r[7],
            "last_count": (r[9] << 16) | r[8],  # hi=0x59, lo=0x58
            "last_seq": r[10],
            "last_uptime": (r[12] << 16) | r[11],  # hi=0x5C, lo=0x5B
        }

    # BLE
    r = parse_read_response(read_registers(0x0060, 2, "BLE"), 2)
    if r:
        result["ble"] = {"state": r[0], "power": signed16(r[1])}

    # Soil Sensor (0x0080~0x0085, 6 registers)
    r = parse_read_response(read_registers(0x0080, 6, "Sensor"), 6)
    if r:
        temp_raw = signed16(r[1])
        result["sensor"] = {
            "mois_raw": r[0],
            "mois": r[0] / 10,
            "temp_raw": temp_raw,
            "temp": temp_raw / 10,
            "ec": r[2],
            "status": r[3],
            "read_cnt": r[4],
            "err_cnt": r[5],
        }

    # Sensor2 온습도 (0x0090~0x0094, 5 registers)
    r = parse_read_response(read_registers(0x0090, 5, "Sensor2"), 5)
    if r:
        temp_raw = signed16(r[1])
        result["sensor2"] = {
            "humi_raw": r[0],
            "humi": r[0] / 10,
            "temp_raw": temp_raw,
            "temp": temp_raw / 10,
            "status": r[2],
            "read_cnt": r[3],
            "err_cnt": r[4],
        }

    # Flash
    r = parse_read_response(read_registers(0x0070, 2, "Flash"), 2)
    if r:
        result["flash"] = {"state": r[0], "test": r[1]}

    # Self-test result
    r = parse_read_response(read_registers(0x01F3, 2, "Self-test"), 2)
    if r:
        result["selftest"] = {"result": r[0], "detail": r[1]}

    return jsonify(result)


# --- Vibration Sensor ---
@app.route("/api/vibration")
def api_vibration():
    r = parse_read_response(read_registers(0x0012, 3, "Vibration"), 3)
    if r:
        return jsonify({"ok": True, "vib": r[0], "vib_count": r[1], "btn": r[2]})
    return jsonify({"ok": False})


# --- Valve ---
@app.route("/api/valve", methods=["POST"])
def api_valve():
    d = request.json
    axis = d.get("axis", "x")
    cmd = d.get("cmd", 2)  # 0=close,1=open,2=stop
    dur = d.get("duration", 5)
    base = 0x0100 if axis == "x" else 0x0102
    desc = f"Valve {axis.upper()} cmd={cmd} dur={dur}"
    resp = write_multiple(base, [cmd, dur], desc)
    return jsonify({"ok": resp is not None})


# --- Flow ---
@app.route("/api/flow", methods=["POST"])
def api_flow():
    d = request.json
    axis = d.get("axis", "x")
    action = d.get("action", "start")  # start, stop, reset
    base = 0x0110 if axis == "x" else 0x0112
    if action == "start":
        resp = write_register(base, 1, f"Flow {axis.upper()} START")
    elif action == "stop":
        resp = write_register(base, 0, f"Flow {axis.upper()} STOP")
    elif action == "reset":
        resp = write_register(base + 1, 1, f"Flow {axis.upper()} RESET")
    else:
        return jsonify({"ok": False})
    return jsonify({"ok": resp is not None})


# --- Buzzer ---
@app.route("/api/buzzer", methods=["POST"])
def api_buzzer():
    d = request.json
    sec = d.get("seconds", 0)
    resp = write_register(0x0120, sec, f"Buzzer {'OFF' if sec==0 else f'{sec}s'}")
    return jsonify({"ok": resp is not None})


# --- LED ---
@app.route("/api/led", methods=["POST"])
def api_led():
    d = request.json
    mode = d.get("mode", 0)
    resp = write_register(0x0121, mode, f"LED mode={mode}")
    return jsonify({"ok": resp is not None})


# --- 12V ---
@app.route("/api/supply12v", methods=["POST"])
def api_supply12v():
    d = request.json
    on = d.get("on", False)
    resp = write_register(0x0122, 1 if on else 0, f"12V {'ON' if on else 'OFF'}")
    return jsonify({"ok": resp is not None})


# --- LoRa ---
@app.route("/api/lora/cmd", methods=["POST"])
def api_lora_cmd():
    d = request.json
    cmd = d.get("cmd", 0)
    names = {0:"STOP",1:"TX_CARRIER",2:"TX_MOD",3:"TX_PKT",4:"RX",5:"SWEEP",6:"DEFAULT"}
    resp = write_register(0x0130, cmd, f"LoRa {names.get(cmd,cmd)}")
    return jsonify({"ok": resp is not None})


@app.route("/api/lora/config", methods=["POST"])
def api_lora_config():
    d = request.json
    ok = True
    if "freq" in d:
        r = write_register(0x0131, d["freq"], f"LoRa Freq={d['freq']}")
        ok = ok and r is not None
    if "power" in d:
        r = write_register(0x0132, d["power"] & 0xFFFF, f"LoRa Pwr={d['power']}")
        ok = ok and r is not None
    if "sf" in d:
        r = write_register(0x0133, d["sf"], f"LoRa SF={d['sf']}")
        ok = ok and r is not None
    if "bw" in d:
        r = write_register(0x0134, d["bw"], f"LoRa BW={d['bw']}")
        ok = ok and r is not None
    return jsonify({"ok": ok})


@app.route("/api/lora/reset", methods=["POST"])
def api_lora_reset():
    resp = write_register(0x013F, 1, "LoRa RESET defaults")
    return jsonify({"ok": resp is not None})


# --- BLE ---
@app.route("/api/ble/cmd", methods=["POST"])
def api_ble_cmd():
    d = request.json
    cmd = d.get("cmd", 0)
    names = {0:"STOP",1:"ADVERT",2:"TX_CARRIER",3:"TX_MOD",4:"RX",5:"DEFAULT"}
    resp = write_register(0x0140, cmd, f"BLE {names.get(cmd,cmd)}")
    return jsonify({"ok": resp is not None})


@app.route("/api/ble/config", methods=["POST"])
def api_ble_config():
    d = request.json
    ok = True
    if "channel" in d:
        r = write_register(0x0141, d["channel"], f"BLE Ch={d['channel']}")
        ok = ok and r is not None
    if "power" in d:
        r = write_register(0x0142, d["power"] & 0xFFFF, f"BLE Pwr={d['power']}")
        ok = ok and r is not None
    return jsonify({"ok": ok})


@app.route("/api/ble/reset", methods=["POST"])
def api_ble_reset():
    resp = write_register(0x014F, 1, "BLE RESET defaults")
    return jsonify({"ok": resp is not None})


# --- Flash ---
@app.route("/api/flash", methods=["POST"])
def api_flash():
    d = request.json
    cmd = d.get("cmd", 0)
    names = {0: "SLEEP", 1: "WAKEUP", 2: "SELF-TEST"}
    resp = write_register(0x0150, cmd, f"Flash {names.get(cmd,cmd)}")
    return jsonify({"ok": resp is not None})


# --- All Stop ---
@app.route("/api/all-stop", methods=["POST"])
def api_all_stop():
    resp = write_register(0x01F0, 1, "ALL STOP")
    return jsonify({"ok": resp is not None})


# --- Self Test ---
@app.route("/api/self-test", methods=["POST"])
def api_self_test():
    d = request.json
    with_rf = d.get("with_rf", False)
    reg = 0x01F2 if with_rf else 0x01F1
    tag = "RF" if with_rf else "NO-RF"
    resp = write_register(reg, 1, f"Self-test ({tag})")
    return jsonify({"ok": resp is not None})


# --- Raw Modbus ---
@app.route("/api/raw", methods=["POST"])
def api_raw():
    """Send raw hex bytes (without CRC - auto appended)."""
    d = request.json
    hex_str = d.get("hex", "").strip()
    try:
        raw = bytes.fromhex(hex_str.replace(" ", ""))
    except ValueError:
        return jsonify({"ok": False, "error": "Invalid hex"})
    resp = send_modbus(raw, "Raw command")
    if resp:
        return jsonify({"ok": True, "response": " ".join(f"{b:02X}" for b in resp)})
    return jsonify({"ok": False, "error": "No response"})


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else "COM15"
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 9600
    web_port = int(sys.argv[3]) if len(sys.argv) > 3 else 5000

    global ser
    try:
        ser = serial.Serial(port, baud, timeout=0.3)
        print(f"[HOST] Serial connected: {port} @ {baud}")
    except Exception as e:
        print(f"[HOST] Serial not connected: {e}")
        print(f"[HOST] Use web UI to connect manually")

    print(f"[HOST] Web UI: http://localhost:{web_port}")
    app.run(host="0.0.0.0", port=web_port, debug=False)


if __name__ == "__main__":
    main()
