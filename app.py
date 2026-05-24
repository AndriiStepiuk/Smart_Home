from flask import Flask, request, jsonify, render_template
from flask_mysqldb import MySQL
import requests

app = Flask(__name__)


BOT_TOKEN = " "
CHAT_ID = " "

def send_telegram(message):

    url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"

    data = {
        "chat_id": CHAT_ID,
        "text": message
        
    }

    requests.post(url, data=data)

app.config['MYSQL_HOST'] = 'localhost'
app.config['MYSQL_USER'] = 'root'
app.config['MYSQL_PASSWORD'] = ''  
app.config['MYSQL_DB'] = 'smart_home'

mysql = MySQL(app)

# головна сторінка
@app.route('/')
def index():
    return render_template('index.html')

# прийом даних з єсп
@app.route('/data', methods=['POST'])
def receive_data():
    data = request.json

    smoke = data['smoke']
    co2 = data['co2']
    temperature = data['temperature']
    humidity = data['humidity']

    # телеграм сповіщення
    if smoke > 50:
        send_telegram("🚨 УВАГА! Високий рівень диму!")

    if co2 > 1000:
        send_telegram("⚠️ Високий рівень CO2!")

    if temperature > 30:
        send_telegram("🔥 Можлива пожежа! Висока температура!")

    if humidity < 30:
        send_telegram("💦 Низька вологість! Увімкніть пристрій зволоження!")    

    temp = data.get('temperature')
    hum = data.get('humidity')
    pres = data.get('pressure')
    co2 = data.get('co2')
    nh3 = data.get('nh3')
    smoke = data.get('smoke')
    alcohol = data.get('alcohol')
    benzene = data.get('benzene')
    light = data.get('light')

    cur = mysql.connection.cursor()
    cur.execute("""
        INSERT INTO sensor_data (temperature, humidity, pressure, co2, nh3, smoke, alcohol, benzene, light)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
    """, (temp, hum, pres, co2, nh3, smoke, alcohol, benzene, light))

    mysql.connection.commit()
    cur.close()

    return jsonify({"status": "ok"})

# отримання даних
@app.route('/api/data')
def get_data():
    cur = mysql.connection.cursor()
    cur.execute("SELECT * FROM sensor_data ORDER BY created_at DESC LIMIT 20")
    rows = cur.fetchall()
    cur.close()

    result = []
    for row in rows:
        result.append({
            "temp": row[1],
            "hum": row[2],
            "pres": row[3],
            "co2": row[4],
            "nh3": row[5],
            "smoke": row[6],
            "alcohol": row[7],
            "benzene": row[8],
            "light": row[9],
            "time": str(row[10])
        })

    return jsonify(result)

#  дані за тиждень
@app.route('/api/history')
def weekly_data():
    cur = mysql.connection.cursor()

    cur.execute("""
        SELECT 
            DATE(created_at),
            AVG(humidity),
            AVG(pressure),
            AVG(co2),
            AVG(nh3),
            AVG(smoke),
            AVG(alcohol),
            AVG(benzene)
        FROM sensor_data
        GROUP BY DATE(created_at)
        ORDER BY DATE(created_at) DESC
        LIMIT 7
    """)

    rows = cur.fetchall()
    cur.close()

    dates = []
    hum_values = []
    pres_values = []
    air_values = []

    for row in rows:
        date = str(row[0])
        hum = float(row[1])
        pres = float(row[2])

        # якість повітря
        air = (row[3] + row[4] + row[5] + row[6] + row[7]) / 5

        dates.append(date)
        hum_values.append(hum)
        pres_values.append(pres)
        air_values.append(float(air))

    dates.reverse()
    hum_values.reverse()
    pres_values.reverse()
    air_values.reverse()

    return jsonify({
        "dates": dates,
        "hum": hum_values,
        "pres": pres_values,
        "air": air_values
    })


@app.route('/led', methods=['POST'])
def led_control():

    data = request.json

    mode = data.get("mode")
    r = data.get("r")
    g = data.get("g")
    b = data.get("b")

    esp_ip = "192.168.3.81"

    try:

        if mode:
            requests.get(f"http://{esp_ip}/mode?name={mode}")

        requests.get(
            f"http://{esp_ip}/color?r={r}&g={g}&b={b}"
        )

        return jsonify({"status":"ok"})

    except:
        return jsonify({"status":"error"})
    

@app.route('/led/auto', methods=['POST'])
def led_auto():

    esp_ip = "192.168.3.81"

    requests.get(f"http://{esp_ip}/auto")

    return jsonify({"status":"ok"})



if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)