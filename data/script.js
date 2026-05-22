// WEBSOCKET 
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onLoad);

function onLoad(event) {
    initWebSocket();
    initChart();
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function initWebSocket() {
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function Send_Data(data) {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(data);
        console.log("📤 Gửi:", data);
    } else {
        alert("⚠️ WebSocket chưa kết nối!");
    }
}

// Process data received from the microcontroller
function onMessage(event) {
    try {
        var data = JSON.parse(event.data);
        
        // Update Clock & Chart (Temperature, Humidity)
        if(data.temperature !== undefined && data.humidity !== undefined) {
            gaugeTemp.refresh(data.temperature);
            gaugeHumi.refresh(data.humidity);
            updateChartData(data.temperature, data.humidity);
        }

        // 2. Update AI Status Card
        if(data.ai_state !== undefined) {
            updateAIStatus(data.ai_state);
        }

    } catch (e) {
        console.warn("Lỗi Parse JSON:", event.data);
    }
}

// UI NAVIGATION 
function showSection(id, event) {
    document.querySelectorAll('.section').forEach(sec => sec.style.display = 'none');
    document.getElementById(id).style.display = 'block';
    if(id === 'settings') document.getElementById(id).style.display = 'flex';
    document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
    event.currentTarget.classList.add('active');
}

// AI STATUS LOGIC 
function updateAIStatus(stateCode) {
    const card = document.getElementById('aiStatusCard');
    const text = document.getElementById('aiStateText');

    // Reset styles
    card.style.borderColor = "#444";
    text.style.color = "#ffffff";

    switch(stateCode) {
        case -1:
            text.innerText = "COLLECTING DATA!";
            card.style.borderColor = "var(--ai-warning)";
            text.style.color = "var(--ai-warning)";
            break;
        case 0:
            text.innerText = "STATE: NORMAL";
            card.style.borderColor = "var(--ai-normal)";
            text.style.color = "var(--ai-normal)";
            break;
        case 1:
            text.innerText = "STATE: FIRE RISK";
            card.style.borderColor = "var(--ai-danger)";
            text.style.color = "var(--ai-danger)";
            break;
        case 2:
            text.innerText = "STATE: MOLD RISK";
            card.style.borderColor = "var(--ai-info)";
            text.style.color = "var(--ai-info)";
            break;
        case 3:
            text.innerText = "STATE: ERROR!";
            card.style.borderColor = "var(--ai-danger)";
            text.style.color = "var(--ai-danger)";
            break;
        case 4:
            text.innerText = "STATE: AC ON";
            card.style.borderColor = "var(--accent-blue)";
            text.style.color = "var(--accent-blue)";
            break;
    }
}

// GAUGES & CHART 
var gaugeTemp, gaugeHumi;
var timeChart;

window.onload = function () {
    gaugeTemp = new JustGage({
        id: "gauge_temp", value: 0, min: -10, max: 80, donut: true, pointer: true,
        gaugeColor: "#e2e5eb", valueFontColor: "#1a1d23",
        levelColors: ["#00d2ff", "#2ed573", "#ffa502", "#ff4757"]
    });

    gaugeHumi = new JustGage({
        id: "gauge_humi", value: 0, min: 0, max: 100, donut: true, pointer: true,
        gaugeColor: "#e2e5eb", valueFontColor: "#1a1d23",
        levelColors: ["#ffa502", "#2ed573", "#00d2ff"]
    });

    initChart();
};

function initChart() {
    const ctx = document.getElementById('historyChart').getContext('2d');
    Chart.defaults.color = '#6b7280'; 

    timeChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: [], 
            datasets: [
                { label: 'Nhiệt độ (°C)', borderColor: '#ff4757', backgroundColor: 'rgba(255, 71, 87, 0.2)', data: [], tension: 0.4, fill: true },
                { label: 'Độ ẩm (%)', borderColor: '#00d2ff', backgroundColor: 'rgba(0, 210, 255, 0.2)', data: [], tension: 0.4, fill: true }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false, 
            scales: {
                x: { grid: { color: '#e2e5eb' } },
                y: { grid: { color: '#e2e5eb' } }
            }
        }
    });
}

function updateChartData(temp, hum) {
    const now = new Date();
    const timeString = now.getHours() + ':' + now.getMinutes() + ':' + now.getSeconds();

    if(timeChart.data.labels.length > 15) {
        timeChart.data.labels.shift();
        timeChart.data.datasets[0].data.shift();
        timeChart.data.datasets[1].data.shift();
    }

    timeChart.data.labels.push(timeString);
    timeChart.data.datasets[0].data.push(temp);
    timeChart.data.datasets[1].data.push(hum);
    timeChart.update();
}

// DEVICE CONTROLS 
let ledState = "AUTO"; 
let neoState = "AUTO";

function toggleLed() {
    // Loop state: AUTO -> ON -> OFF -> AUTO...
    if (ledState === "AUTO") {
        ledState = "ON";
    } else if (ledState === "ON") {
        ledState = "OFF";
    } else {
        ledState = "AUTO";
    }

    const btn = document.getElementById('btnLed');
    btn.innerText = "TRẠNG THÁI: " + ledState;
    
    if (ledState === "ON") btn.className = "toggle-btn on";
    else if (ledState === "OFF") btn.className = "toggle-btn off";
    else btn.className = "toggle-btn auto"; 
    
    updateLed();
}

function updateLed() {
    const freq = document.getElementById('sliderLedFreq').value;
    document.getElementById('valLedFreq').innerText = freq;
    
    const payload = JSON.stringify({
        device: "single_led",
        state: ledState, // Send "AUTO", "ON" or "OFF"
        delay: parseInt(freq)
    });
    Send_Data(payload);
}

function toggleNeo() {
    // Loop state: AUTO -> ON -> OFF -> AUTO...
    if (neoState === "AUTO") {
        neoState = "ON";
    } else if (neoState === "ON") {
        neoState = "OFF";
    } else {
        neoState = "AUTO";
    }

    const btn = document.getElementById('btnNeo');
    btn.innerText = "TRẠNG THÁI: " + neoState;
    
    if (neoState === "ON") btn.className = "toggle-btn on";
    else if (neoState === "OFF") btn.className = "toggle-btn off";
    else btn.className = "toggle-btn auto"; 

    updateNeo();
}

function updateNeo() {
    const freq = document.getElementById('sliderNeoFreq').value;
    const hexColor = document.getElementById('pickerNeoColor').value;
    document.getElementById('valNeoFreq').innerText = freq;
    
    const payload = JSON.stringify({
        device: "neopixel",
        state: neoState, // Send "AUTO", "ON" or "OFF"
        color: hexColor, 
        delay: parseInt(freq)
    });
    Send_Data(payload);
}

// SETTINGS FORM 
document.getElementById("settingsForm").addEventListener("submit", function (e) {
    e.preventDefault();
    const settingsJSON = JSON.stringify({
        page: "setting",
        value: {
            ssid: document.getElementById("ssid").value.trim(),
            password: document.getElementById("password").value.trim(),
            token: document.getElementById("token").value.trim(),
            server: document.getElementById("server").value.trim(),
            port: document.getElementById("port").value.trim()
        }
    });
    Send_Data(settingsJSON);
    alert("✅ Đã gửi lệnh khởi động cấu hình hệ thống!");
});
