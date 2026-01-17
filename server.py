import os
from datetime import datetime
from flask import Flask, request, jsonify
import numpy as np
import cv2
import easyocr

app = Flask(__name__)

print("Loading AI Model...")
reader = easyocr.Reader(['en'])
print("✅ AI Model Loaded!")

UPLOAD_FOLDER = 'captured_images'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

@app.route('/upload', methods=['POST'])
def upload_image():
    try:
        # Get load number from ESP32 (1 or 2)
        load_number = request.args.get("load", "UNKNOWN")

        img_bytes = request.get_data()
        if not img_bytes:
            return jsonify({"error": "No image data"}), 400

        nparr = np.frombuffer(img_bytes, np.uint8)
        img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

        if img is None:
            return jsonify({"error": "Invalid image"}), 400

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"LOAD{load_number}_{timestamp}.jpg"
        save_path = os.path.join(UPLOAD_FOLDER, filename)
        cv2.imwrite(save_path, img)

        # OCR
        results = reader.readtext(img, detail=0)
        plate_text = " ".join(results).upper() if results else "UNKNOWN"

        print(f"📸 Saved: {filename}")
        print(f"🔠 Plate: {plate_text}")

        return jsonify({
            "plate": plate_text,
            "load": load_number,
            "time": timestamp
        })

    except Exception as e:
        print(f"❌ Error: {e}")
        return jsonify({"error": "Server error"}), 500


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
