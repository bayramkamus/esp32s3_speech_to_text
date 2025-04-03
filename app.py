from flask import Flask, request, send_from_directory
from flask import Flask, request, send_from_directory, render_template
import os
import uuid
import speech_recognition as sr

UPLOAD_FOLDER = "saved_audios"
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# 🔐 Gerekli olan doğru sesli şifre
EXPECTED_PASSWORD = "mermer merve"

app = Flask(__name__)

@app.route("/upload", methods=["POST"])
def upload():
    try:
        wav_data = request.get_data()
        filename = f"received_{uuid.uuid4()}.wav"
        file_path = os.path.join(UPLOAD_FOLDER, filename)

        with open(file_path, "wb") as f:
            f.write(wav_data)

        print(f"📁 Saved: {file_path} | Size: {os.path.getsize(file_path)} bayt")

        recognizer = sr.Recognizer()
        with sr.AudioFile(file_path) as source:
            audio = recognizer.record(source)
            text = recognizer.recognize_google(audio, language="tr-TR").strip().lower()
            print("🧠 Recognized:", text)

        # 🔐 Şifre kontrolü
        if text == EXPECTED_PASSWORD:
            print("✔️ Correct password! Access granted.")
            return "OK"
        else:
            print("❌ Incorrect password!")
            return "DENIED"

    except Exception as e:
        print("❌ Recognition error:", repr(e))
        return "ERROR"

@app.route("/files/<filename>")
def serve_audio_file(filename):
    return send_from_directory(UPLOAD_FOLDER, filename)

@app.route("/")
def index():
    return render_template("index.html")


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
