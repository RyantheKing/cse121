from flask import Flask, request, jsonify
import socket

app = Flask(__name__)

posted_data = []

location = "Santa+Cruz"

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(('10.254.254.254', 1))
        local_ip = s.getsockname()[0]
    except Exception:
        local_ip = '127.0.0.1'
    finally:
        s.close()
    return local_ip

@app.route('/')
def home():
    local_ip = get_local_ip()
    # convert posted data to string
    data_display = '\n'.join(str(data) for data in posted_data[-50:])
    return f"""
    <html>
        <body>
            <h1>Local IP: {local_ip}</h1>
            <h2>Posted Data:</h2>
            <pre>{data_display}</pre>
        </body>
    </html>
    """

@app.route('/post', methods=['POST'])
def posit_data():
    global posted_data
    data = request.get_json() or request.form or request.data or {}
    posted_data.append(data)
    return jsonify(data)

@app.route('/location', methods=['GET'])
def get_location():
    return location, 200, {'Content-Type': 'text/plain'}

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=1234, debug=True)
