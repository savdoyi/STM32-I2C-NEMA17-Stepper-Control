from machine import Pin, I2C
import network
import socket
import time
import struct
import _thread # Parallel ishlov berish uchun (yoki oddiy loop ichida)

# I2C sozlamalari
i2c = I2C(0, scl=Pin(22), sda=Pin(21), freq=100000) # Sizning pinlaringiz
STM_ADDR = 0x20 # ESP32 dan ko'rinadigan STM32 ning I2C manzili (0x20 << 1)

# Wi-Fi sozlamalari
SSID = '' # Telefoningiz hotspot nomi
PASSWORD = '' # Telefoningiz hotspot paroli
# main.py (ESP32 uchun)

# --- Global o'zgaruvchilar ---
# Aktiv buyruqlar ketma-ketligini saqlash uchun ro'yxat
current_command_sequence = [] 
sequence_lock = _thread.allocate_lock() # Buyruqlar ro'yxatiga kirishni sinxronlashtirish uchun lock
sequence_running = False # Ketma-ketlikning hozirda ishlayotganligini bildiradi

# --- Wi-Fi ga ulanish funksiyasi ---
def connect_wifi(ssid, password):
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        print(f'Connecting to network "{ssid}"...')
        wlan.connect(ssid, password)
        max_attempts = 15 
        attempts = 0
        while not wlan.isconnected() and attempts < max_attempts:
            print('.')
            time.sleep(1)
            attempts += 1
        if wlan.isconnected():
            print('Wi-Fi connected!')
            print('Network config:', wlan.ifconfig())
            return wlan.ifconfig()[0] 
        else:
            print('Wi-Fi connection failed after multiple attempts!')
            return None
    return wlan.ifconfig()[0]

# --- I2C orqali STM32 ga ma'lumot yuborish funksiyasi ---
def send_data_to_stm32(data_bytes):
    length = len(data_bytes)
    try:
        # Avval STM32 mavjudligini tekshirish
        # Agar STM_ADDR skanerda ko'rinmasa, xato berishi mumkin
        if STM_ADDR not in i2c.scan():
            print(f"I2C Warning: STM32 device at {hex(STM_ADDR)} not found on bus.")
            return False

        # Birinchi bayt sifatida ma'lumot uzunligini yuborish
        i2c.writeto(STM_ADDR, bytes([length]))
        # Keyin haqiqiy ma'lumotlarni yuborish
        i2c.writeto(STM_ADDR, data_bytes)
        print(f"Sent to STM32: length={length}, data={data_bytes.hex()}")
        return True
    except OSError as e: # ENODEV kabi xatolar OSError turiga kiradi
        print(f"I2C Error sending to STM32 ({e}): Please check wiring and STM32 status.")
        return False
    except Exception as e:
        print(f"An unexpected error occurred during I2C send: {e}")
        return False

# --- Buyruqlar ketma-ketligini doimiy bajarish funksiyasi (alohida thread) ---
def sequence_runner():
    global sequence_running, current_command_sequence
    sequence_running = True
    print("Sequence runner thread started.")
    
    # Qaytmasdan avval 5 soniya kutish va STM32 ni topishga urinish
    startup_attempts = 0
    while STM_ADDR not in i2c.scan() and startup_attempts < 10:
        print(f"Waiting for STM32 at {hex(STM_ADDR)} to appear on I2C bus...")
        time.sleep(2)
        startup_attempts += 1
    if STM_ADDR not in i2c.scan():
        print("STM32 not found after multiple attempts. Sequence runner will continue checking.")
        
    while sequence_running:
        sequence_lock.acquire() # Ro'yxatni o'qish/yozishda konflikt bo'lmasligi uchun lock
        
        if not current_command_sequence:
            sequence_lock.release()
            time.sleep(1) # Agar buyruq bo'lmasa, biroz kutish
            continue
        
        for cmd in current_command_sequence:
            print(f"[Sequence] Executing: {cmd['name']} with data {cmd['data'].hex()}")
            if not send_data_to_stm32(cmd['data']):
                print(f"[Sequence Error] Failed to send {cmd['name']}. Pausing sequence for 3 seconds.")
                time.sleep(3) # Xatodan keyin biroz kutish
                # Agar xato bo'lsa, keyingi buyruqqa o'tish yoki to'liq qayta urinish logikasi qo'shilishi mumkin
                break # Joriy siklni tugatish va qayta urinish (keyingi siklda boshidan boshlaydi)
            
            if cmd['interval'] > 0:
                print(f"[Sequence] Waiting for {cmd['interval']} seconds after '{cmd['name']}'...")
                time.sleep(cmd['interval'])
        
        sequence_lock.release()
        # Bir sikl tugagandan so'ng, keyingi siklga o'tishdan oldin
        # butun ketma-ketlikni tugatish uchun minimal kutish (agar oxirgi interval 0 bo'lsa)
        time.sleep(0.1) 
    print("Sequence runner thread stopped.")


# --- Veb sahifaning HTML tarkibi ---
def get_html_page(message=""):
    message_html = ""
    if message:
        error_class = " error" if "Error" in message or "Failed" in message else ""
        message_html = f'<p class="message{error_class}">{message}</p>'

    html = f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>STM32 Motor Sequence Control</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            body {{ font-family: Arial, sans-serif; margin: 20px; background-color: #f4f4f4; color: #333; }}
            .container {{ background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,.1); max-width: 800px; margin: auto; }}
            h1 {{ color: #007bff; text-align: center; margin-bottom: 20px; }}
            .command-group {{ 
                margin-bottom: 25px;
                border: 1px solid #ddd;
                border-radius: 5px;
                padding: 10px;
                background-color: #fafafa;
            }}
            .command-group h2 {{
                margin-top: 0;
                margin-bottom: 10px;
                color: #555;
                font-size: 1.5em;
                text-align: center;
            }}
            label {{ display: block; margin-bottom: 3px; font-weight: bold; font-size: 0.85em; }}
            input[type="text"],
            input[type="number"] {{
                width: calc(100% - 12px);
                padding: 6px;
                margin-bottom: 8px;
                border: 1px solid #ccc;
                border-radius: 3px;
                font-family: 'Courier New', monospace;
                background-color: #eef;
            }}
            input[readonly] {{ background-color: #f0f0f0; cursor: not-allowed; }}
            .byte-inputs {{
                display: flex;
                flex-wrap: wrap;
                gap: 10px;
                margin-bottom: 12px;
                justify-content: center;
            }}
            .byte-input-group {{
                flex: 0 0 calc(14.28% - 10px);
                min-width: 80px;
                max-width: 120px;
                text-align: center;
            }}
            .byte-input-group input {{ width: 100%; box-sizing: border-box; text-align: center; }}
            button {{ background-color: #007bff; color: white; padding: 10px 15px; border: none; border-radius: 4px; cursor: pointer; font-size: 1.1em; width: 100%; margin-top: 15px; }}
            button:hover {{ background-color: #0056b3; }}
            .info {{ font-size: 0.9em; color: #666; margin-top: 10px; text-align: center; padding-bottom: 15px; }}
            .note {{ font-size: 0.8em; color: #888; margin-top: 5px; margin-left: 5px; text-align: center; padding-top: 5px; }}
            .flex-container {{ display: flex; justify-content: space-between; align-items: flex-end; flex-wrap: wrap; margin-top: 10px; }}
            .flex-item {{ flex: 1; min-width: 150px; margin-right: 15px; }}
            .flex-item:last-child {{ margin-right: 0; }}
            .message {{ text-align: center; font-weight: bold; margin-bottom: 15px; padding: 10px; border-radius: 5px; background-color: #e6ffe6; color: green; border: 1px solid #aaffaa; }}
            .error {{ color: red; background-color: #ffe6e6; border-color: #ffaaaa; }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>STM32 Motor Sequence Control</h1>
            <p class="info">Configure motors and sequence actions from here. Values can be decimal (0-255) or hexadecimal (e.g., 0xFF).</p>
            {message_html}

            <form action="/send_sequence" method="post">

                <div class="command-group">
                    <h2>Configure Motor 1</h2>
                    <div class="byte-inputs">
                        <div class="byte-input-group"><label>Mode</label><input type="text" name="cmd1_byte0" value="0x00" maxlength="4" readonly></div>
                        <div class="byte-input-group"><label>Motor ID</label><input type="text" name="cmd1_byte1" value="0x01" maxlength="4" readonly></div>
                        <div class="byte-input-group"><label>Enable</label><input type="text" name="cmd1_byte2" value="0x01" maxlength="4"></div>
                        <div class="byte-input-group"><label>Direction</label><input type="text" name="cmd1_byte3" value="0x01" maxlength="4"></div>
                        <div class="byte-input-group"><label>Velocity</label><input type="text" name="cmd1_byte4" value="0x1e" maxlength="4"></div>
                        <div class="byte-input-group"><label>Degree High</label><input type="text" name="cmd1_byte5" value="0xb4" maxlength="4"></div>
                        <div class="byte-input-group"><label>Degree Low</label><input type="text" name="cmd1_byte6" value="0xb4" maxlength="4"></div>
                    </div>
                    <div class="flex-container">
                        <div class="flex-item"><label>Interval (s) after this</label><input type="number" name="cmd1_interval" value="5" min="0" step="1"></div>
                        <div class="flex-item" style="display: flex; align-items: center; justify-content: flex-end;"><label for="cmd1_enabled" style="margin-bottom: 0; margin-right: 5px;">Enabled?</label><input type="checkbox" name="cmd1_enabled" id="cmd1_enabled" checked></div>
                    </div>
                    <p class="note">Example: `0x00, 0x01, 0x01, 0x01, 0x1e, 0xb4, 0xb4` (Config M1, Enabled, CCW, 30deg/s, 180.180 degrees)</p>
                </div>

                <div class="command-group">
                    <h2>Configure Motor 2</h2>
                    <div class="byte-inputs">
                        <div class="byte-input-group"><label>Mode</label><input type="text" name="cmd2_byte0" value="0x00" maxlength="4" readonly></div>
                        <div class="byte-input-group"><label>Motor ID</label><input type="text" name="cmd2_byte1" value="0x00" maxlength="4" readonly></div>
                        <div class="byte-input-group"><label>Enable</label><input type="text" name="cmd2_byte2" value="0x01" maxlength="4"></div>
                        <div class="byte-input-group"><label>Direction</label><input type="text" name="cmd2_byte3" value="0x01" maxlength="4"></div>
                        <div class="byte-input-group"><label>Velocity</label><input type="text" name="cmd2_byte4" value="0x5a" maxlength="4"></div>
                        <div class="byte-input-group"><label>Degree High</label><input type="text" name="cmd2_byte5" value="0xfa" maxlength="4"></div>
                        <div class="byte-input-group"><label>Degree Low</label><input type="text" name="cmd2_byte6" value="0x5a" maxlength="4"></div>
                    </div>
                    <div class="flex-container">
                        <div class="flex-item"><label>Interval (s) after this</label><input type="number" name="cmd2_interval" value="4" min="0" step="1"></div>
                        <div class="flex-item" style="display: flex; align-items: center; justify-content: flex-end;"><label for="cmd2_enabled" style="margin-bottom: 0; margin-right: 5px;">Enabled?</label><input type="checkbox" name="cmd2_enabled" id="cmd2_enabled" checked></div>
                    </div>
                    <p class="note">Example: `0x00, 0x00, 0x01, 0x01, 0x5a, 0xfa, 0x5a` (Config M2, Enabled, CCW, 90deg/s, 250.090 degrees)</p>
                </div>

                <div class="command-group">
                    <h2>Start Motors</h2>
                    <div class="byte-inputs" style="justify-content: flex-start;">
                        <div class="byte-input-group" style="flex: none; width: auto;"><label>Mode</label><input type="text" name="cmd3_byte0" value="0x01" maxlength="4" readonly></div>
                    </div>
                    <div class="flex-container">
                        <div class="flex-item"><label>Interval (s) after this</label><input type="number" name="cmd3_interval" value="6" min="0" step="1"></div>
                        <div class="flex-item" style="display: flex; align-items: center; justify-content: flex-end;"><label for="cmd3_enabled" style="margin-bottom: 0; margin-right: 5px;">Enabled?</label><input type="checkbox" name="cmd3_enabled" id="cmd3_enabled" checked></div>
                    </div>
                    <p class="note">Example: `0x01` (Starts previously configured motors)</p>
                </div>

                <div class="command-group">
                    <h2>Stop Motors</h2>
                    <div class="byte-inputs" style="justify-content: flex-start;">
                        <div class="byte-input-group" style="flex: none; width: auto;"><label>Mode</label><input type="text" name="cmd4_byte0" value="0x02" maxlength="4" readonly></div>
                    </div>
                    <div class="flex-container">
                        <div class="flex-item"><label>Interval (s) after this</label><input type="number" name="cmd4_interval" value="5" min="0" step="1"></div>
                        <div class="flex-item" style="display: flex; align-items: center; justify-content: flex-end;"><label for="cmd4_enabled" style="margin-bottom: 0; margin-right: 5px;">Enabled?</label><input type="checkbox" name="cmd4_enabled" id="cmd4_enabled" checked></div>
                    </div>
                    <p class="note">Example: `0x02` (Stops all motors immediately)</p>
                </div>

                <button type="submit">Send All Commands in Sequence</button>
            </form>
        </div>
    </body>
    </html>
    """
    return html

# --- Veb serverni ishga tushirish ---
def start_web_server():
    global current_command_sequence, sequence_running

    addr = socket.getaddrinfo('0.0.0.0', 80)[0][-1]
    s = socket.socket()
    s.bind(addr)
    s.listen(1)
    print('Listening on', addr)

    # Buyruqlar ketma-ketligini bajaruvchi threadni ishga tushirish
    if not sequence_running:
        _thread.start_new_thread(sequence_runner, ())

    while True:
        try:
            cl, addr = s.accept()
            print('Client connected from:', addr)
            request = cl.recv(2048).decode('utf-8') 

            response_message = ""

            if "POST /send_sequence" in request:
                content_start = request.find('\r\n\r\n') + 4
                post_data = request[content_start:]
                print("POST Data:", post_data)

                params = {}
                for pair in post_data.split('&'):
                    key_value = pair.split('=')
                    if len(key_value) == 2:
                        key = key_value[0]
                        value = key_value[1].replace('+', ' ') 
                        params[key] = value
                    elif len(key_value) == 1: 
                        params[key_value[0]] = ''

                # Vaqtinchalik buyruqlar ro'yxati
                temp_commands_to_execute = []
                parsing_error = False

                # Command 1 (Motor 1 Config)
                if 'cmd1_enabled' in params and params['cmd1_enabled'] == 'on':
                    try:
                        data1 = [
                            int(params.get('cmd1_byte0', '0'), 0),
                            int(params.get('cmd1_byte1', '0'), 0),
                            int(params.get('cmd1_byte2', '0'), 0),
                            int(params.get('cmd1_byte3', '0'), 0),
                            int(params.get('cmd1_byte4', '0'), 0),
                            int(params.get('cmd1_byte5', '0'), 0),
                            int(params.get('cmd1_byte6', '0'), 0)
                        ]
                        interval1 = int(params.get('cmd1_interval', '0'))
                        temp_commands_to_execute.append({'data': bytes(data1), 'interval': interval1, 'name': 'Motor 1 Config'})
                    except ValueError as e:
                        response_message += f"Error in Command 1 values: {e}. "
                        parsing_error = True

                # Command 2 (Motor 2 Config)
                if 'cmd2_enabled' in params and params['cmd2_enabled'] == 'on':
                    try:
                        data2 = [
                            int(params.get('cmd2_byte0', '0'), 0),
                            int(params.get('cmd2_byte1', '0'), 0),
                            int(params.get('cmd2_byte2', '0'), 0),
                            int(params.get('cmd2_byte3', '0'), 0),
                            int(params.get('cmd2_byte4', '0'), 0),
                            int(params.get('cmd2_byte5', '0'), 0),
                            int(params.get('cmd2_byte6', '0'), 0)
                        ]
                        interval2 = int(params.get('cmd2_interval', '0'))
                        temp_commands_to_execute.append({'data': bytes(data2), 'interval': interval2, 'name': 'Motor 2 Config'})
                    except ValueError as e:
                        response_message += f"Error in Command 2 values: {e}. "
                        parsing_error = True

                # Command 3 (Start Motors)
                if 'cmd3_enabled' in params and params['cmd3_enabled'] == 'on':
                    try:
                        data3 = [int(params.get('cmd3_byte0', '1'), 0)]
                        interval3 = int(params.get('cmd3_interval', '0'))
                        temp_commands_to_execute.append({'data': bytes(data3), 'interval': interval3, 'name': 'Start Motors'})
                    except ValueError as e:
                        response_message += f"Error in Start Command values: {e}. "
                        parsing_error = True

                # Command 4 (Stop Motors)
                if 'cmd4_enabled' in params and params['cmd4_enabled'] == 'on':
                    try:
                        data4 = [int(params.get('cmd4_byte0', '2'), 0)]
                        interval4 = int(params.get('cmd4_interval', '0'))
                        temp_commands_to_execute.append({'data': bytes(data4), 'interval': interval4, 'name': 'Stop Motors'})
                    except ValueError as e:
                        response_message += f"Error in Stop Command values: {e}. "
                        parsing_error = True

                if not parsing_error:
                    sequence_lock.acquire() # Ro'yxatni yangilashdan oldin lock
                    current_command_sequence = temp_commands_to_execute
                    sequence_lock.release() # Lockni bo'shatish

                    if current_command_sequence:
                        response_message = "New command sequence received and set for continuous execution!"
                    else:
                        response_message = "No commands selected. Sequence cleared."
                else:
                    response_message = "Errors found in command values. Sequence NOT updated."

                response_html = get_html_page(response_message)
                cl.send("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n".encode('utf-8'))
                cl.send(response_html.encode('utf-8'))
                cl.close()
                continue 

            elif "GET / " in request:
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + get_html_page()
            else:
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found"

            cl.send(response.encode('utf-8'))
            cl.close()

        except OSError as e:
            print("Socket Error:", e)
            cl.close()
        except Exception as e:
            print("An unexpected error occurred:", e)
            cl.close()

# --- Main funksiya ---
if __name__ == '__main__':
    ip_address = connect_wifi(SSID, PASSWORD)
    if ip_address:
        print(f"Web serverni boshlash uchun {ip_address} manziliga kiring")
        start_web_server()
    else:
        print("Wi-Fi ulanmadi, veb server ishga tushirilmaydi.")