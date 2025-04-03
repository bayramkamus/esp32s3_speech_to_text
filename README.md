# 🔐 ESP32-S3 Voice Password Recognition Project

A complete voice-activated password recognition system using the ESP32-S3 microcontroller, capable of detecting voice commands and verifying them as passwords to unlock access.

---

### What it does:

- Captures voice input through ESP32-S3.
- Recognizes voice and checks if it matches a predefined password.
- Communicates with a Flask server for authentication and UI feedback.
- Displays access status via HTML interface.

---

## 🧩 Components

- ESP32-S3 microcontroller
- inmp441 microphone
- Wi-Fi network
- Flask server (Python)
- HTML frontend for user feedback

---

## 📁 Project Structure

```
esp32s3_voice_password/
├── main/
│   ├── main.c
│
├── templates/
│   ├── index.html
├── app.py
├── sdkconfig
├── README.md
```

---

## ⚙️ Setup

### ESP32-S3 Firmware

1. Clone the project:

   ```bash
   git clone https://github.com/bayramkamus/esp32s3_speech_to_text.git
   cd esp32s3_speech_to_text
   ```

2. Configure Wi-Fi:

   ```c
   #define WIFI_SSID "YOUR_WIFI_SSID"
   #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
   ```

3. Configure server IP:

   ```c
   #define SERVER_IP "YOUR_SERVER_IP"
   #define SERVER_PORT 5000
   ```

4. Build and flash:
   ```bash
   idf.py build
   idf.py -p <YOUR_SERIAL_PORT> flash monitor
   ```

---

### Flask Server (Python)

1. Navigate to `esp32s3_speech_to_text` folder:

   ```bash
   cd esp32s3_speech_to_text
   ```

2. Install dependencies:

   ```bash
   pip install flask
   ```

3. Run the server:
   ```bash
   python app.py
   ```

---

### HTML

The HTML file is served from the Flask backend and displays recognition status and access results.

---

## 🔐 How It Works

1. User speaks a passphrase into ESP32-S3.
2. Speech is converted to text using a recognition model.
3. Recognized text is sent to the Flask server.
4. Server checks if the phrase matches the expected password.
5. Result is displayed on the HTML page (Access Granted/Denied).

---

## 🛠️ Example Output

```
Recognized text: "open sesame"
Access Status: ✅ Access Granted
```

Or

```
Recognized text: "hello world"
Access Status: ❌ Access Denied
```

---

📌 **Notes**

- Ensure ESP32-S3 has stable power supply and working microphone.
- Both ESP32 and Flask server must be on the same network.
- Modify password logic in `app.py` as needed.
