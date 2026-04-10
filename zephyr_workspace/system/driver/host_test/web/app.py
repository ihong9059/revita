"""
REVITA Host — LTE Session-based Web UI

Tower가 주기적으로 LTE 연결 → 데이터 업로드 + 명령 다운로드 → 연결 해제
Host는 세션 간에 명령을 큐에 쌓아두고, 세션 시 일괄 전달.

현재 단계: USB CDC로 세션 시뮬레이션 (실제 LTE 미연동)
"""

import sys
import time
import json
import threading
import uuid
from datetime import datetime
from flask import Flask, render_template, jsonify, request
import serial

app = Flask(__name__)

# ---------------------------------------------------------------------------
# Serial (USB CDC — 실제 시스템에서는 LTE/MQTT로 대체)
# ---------------------------------------------------------------------------
SERIAL_PORT = "/dev/ttyACM4"
SERIAL_BAUD = 115200
ser = None
ser_lock = threading.Lock()


def serial_connect():
    global ser
    try:
        ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.5)
        print(f"[OK] Connected to {SERIAL_PORT}")
        return True
    except Exception as e:
        print(f"[ERR] Serial open failed: {e}")
        ser = None
        return False


def serial_send(cmd):
    """시리얼로 명령 보내고 응답 수신"""
    with ser_lock:
        if ser is None or not ser.is_open:
            return None
        ser.write((cmd + "\n").encode())
        time.sleep(0.2)
        resp = ""
        while ser.in_waiting > 0:
            resp += ser.read(ser.in_waiting).decode(errors="replace")
            time.sleep(0.05)
        return resp.strip()


# ---------------------------------------------------------------------------
# Command Queue — 세션 간 명령 보관
# ---------------------------------------------------------------------------
cmd_queue = []       # pending commands
cmd_history = []     # delivered/completed commands
MAX_HISTORY = 100


def queue_command(target, cmd, args=None):
    entry = {
        "req_id": uuid.uuid4().hex[:8],
        "target": target,
        "cmd": cmd,
        "args": args or {},
        "status": "pending",
        "created_at": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "delivered_at": None,
    }
    cmd_queue.append(entry)
    return entry


def cancel_command(req_id):
    for i, c in enumerate(cmd_queue):
        if c["req_id"] == req_id and c["status"] == "pending":
            c["status"] = "cancelled"
            cmd_history.append(cmd_queue.pop(i))
            if len(cmd_history) > MAX_HISTORY:
                cmd_history.pop(0)
            return True
    return False


# ---------------------------------------------------------------------------
# Session Management
# ---------------------------------------------------------------------------
sessions = []        # session history
MAX_SESSIONS = 50
session_seq = 0

# Latest telemetry snapshot
telemetry = {
    "ts": None,
    "soil1_temp": None, "soil1_humi": None,
    "soil2_temp": None, "soil2_humi": None,
    "leaf_temp": None, "leaf_humi": None,
    "btn": None, "vib": None,
    "flow1": None, "flow2": None,
    "battery_mv": None,
}

# Schedule config
schedule = {
    "period_s": 1800,       # 기본 30분
    "mode": "normal",       # normal / frequent / power_save
}

# Communication log
comm_log = []
MAX_LOG = 200


def log_entry(direction, text):
    entry = {
        "time": datetime.now().strftime("%H:%M:%S"),
        "dir": direction,
        "text": text.strip(),
    }
    comm_log.append(entry)
    if len(comm_log) > MAX_LOG:
        comm_log.pop(0)


def parse_kv(s):
    """'key1=val1,key2=val2' → dict"""
    result = {}
    for pair in s.split(","):
        if "=" in pair:
            k, v = pair.split("=", 1)
            result[k.strip()] = v.strip()
    return result


def execute_session():
    """
    세션 실행: Tower에 연결하여 데이터 교환.
    현재는 USB CDC로 시뮬레이션.
    """
    global session_seq
    session_seq += 1
    session_start = datetime.now()
    delivered_cmds = []
    uploaded_records = 0

    log_entry("SYS", f"=== SESSION #{session_seq} START ===")

    # ① 텔레메트리 수신 (SENSOR_READ + INPUT_READ)
    resp = serial_send("CMD:SENSOR_READ")
    if resp:
        log_entry("TX", "CMD:SENSOR_READ")
        log_entry("RX", resp)
        for line in resp.split("\n"):
            if line.startswith("SENSOR:"):
                d = parse_kv(line[7:])
                for k, v in d.items():
                    if k in telemetry:
                        telemetry[k] = v
                uploaded_records += 1

    resp = serial_send("CMD:INPUT_READ")
    if resp:
        log_entry("TX", "CMD:INPUT_READ")
        log_entry("RX", resp)
        for line in resp.split("\n"):
            if line.startswith("INPUT:"):
                d = parse_kv(line[6:])
                for k, v in d.items():
                    if k == "bat":
                        telemetry["battery_mv"] = v
                    elif k in telemetry:
                        telemetry[k] = v
                uploaded_records += 1

    telemetry["ts"] = session_start.strftime("%Y-%m-%d %H:%M:%S")

    # ② 대기 명령 전달
    pending = [c for c in cmd_queue if c["status"] == "pending"]
    for c in pending:
        cmd_str = f"CMD:{c['cmd']}"
        resp = serial_send(cmd_str)
        if resp:
            log_entry("TX", cmd_str)
            log_entry("RX", resp)
            c["status"] = "delivered"
            c["delivered_at"] = datetime.now().strftime("%H:%M:%S")
            delivered_cmds.append(c)

    # 전달 완료 → 큐에서 히스토리로 이동
    for c in delivered_cmds:
        cmd_queue.remove(c)
        cmd_history.append(c)
        if len(cmd_history) > MAX_HISTORY:
            cmd_history.pop(0)

    # ③ 세션 기록
    duration_ms = int((datetime.now() - session_start).total_seconds() * 1000)
    session_record = {
        "seq": session_seq,
        "time": session_start.strftime("%H:%M:%S"),
        "date": session_start.strftime("%Y-%m-%d"),
        "uploaded": uploaded_records,
        "cmds_delivered": len(delivered_cmds),
        "duration_ms": duration_ms,
    }
    sessions.append(session_record)
    if len(sessions) > MAX_SESSIONS:
        sessions.pop(0)

    log_entry("SYS", f"=== SESSION #{session_seq} END ({duration_ms}ms, "
              f"{uploaded_records} records, {len(delivered_cmds)} cmds) ===")

    return session_record


# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------
@app.route("/")
def index():
    return render_template("index.html", port=SERIAL_PORT)


@app.route("/api/connect", methods=["POST"])
def api_connect():
    global SERIAL_PORT
    port = request.json.get("port", SERIAL_PORT)
    SERIAL_PORT = port
    ok = serial_connect()
    return jsonify({"ok": ok, "port": SERIAL_PORT})


@app.route("/api/session/start", methods=["POST"])
def api_session_start():
    """수동 세션 실행 (시뮬레이션)"""
    if ser is None or not ser.is_open:
        return jsonify({"ok": False, "error": "not connected"})
    result = execute_session()
    return jsonify({"ok": True, "session": result})


@app.route("/api/session/list")
def api_session_list():
    return jsonify(sessions[-20:])


@app.route("/api/telemetry")
def api_telemetry():
    return jsonify(telemetry)


@app.route("/api/cmd/queue", methods=["POST"])
def api_cmd_queue():
    """명령 큐에 적재"""
    data = request.json
    cmd = data.get("cmd", "")
    target = data.get("target", "link:01")
    args = data.get("args", {})
    if not cmd:
        return jsonify({"ok": False, "error": "empty cmd"})
    entry = queue_command(target, cmd, args)
    return jsonify({"ok": True, "entry": entry})


@app.route("/api/cmd/cancel", methods=["POST"])
def api_cmd_cancel():
    req_id = request.json.get("req_id", "")
    ok = cancel_command(req_id)
    return jsonify({"ok": ok})


@app.route("/api/cmd/pending")
def api_cmd_pending():
    return jsonify([c for c in cmd_queue if c["status"] == "pending"])


@app.route("/api/cmd/history")
def api_cmd_history():
    return jsonify(cmd_history[-30:])


@app.route("/api/schedule", methods=["GET", "POST"])
def api_schedule():
    if request.method == "POST":
        data = request.json
        if "period_s" in data:
            schedule["period_s"] = int(data["period_s"])
        if "mode" in data:
            schedule["mode"] = data["mode"]
        return jsonify({"ok": True, "schedule": schedule})
    return jsonify(schedule)


@app.route("/api/log")
def api_log():
    return jsonify(comm_log)


@app.route("/api/status")
def api_status():
    connected = ser is not None and ser.is_open
    last_session = sessions[-1] if sessions else None
    return jsonify({
        "connected": connected,
        "port": SERIAL_PORT,
        "session_seq": session_seq,
        "last_session": last_session,
        "pending_cmds": len([c for c in cmd_queue if c["status"] == "pending"]),
        "schedule": schedule,
    })


@app.route("/api/send", methods=["POST"])
def api_send():
    """직접 명령 전송 (디버그용)"""
    cmd = request.json.get("cmd", "")
    if not cmd:
        return jsonify({"ok": False, "error": "empty"})
    resp = serial_send(cmd)
    if resp is None:
        return jsonify({"ok": False, "error": "not connected"})
    log_entry("TX", cmd)
    if resp:
        log_entry("RX", resp)
    return jsonify({"ok": True, "response": resp})


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    if len(sys.argv) > 1:
        SERIAL_PORT = sys.argv[1]
    serial_connect()
    app.run(host="0.0.0.0", port=8080, debug=True)
