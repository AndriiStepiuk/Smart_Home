async function loadData() {
    const res = await fetch('/api/data');
    const data = await res.json();

    const labels = data.map(d => d.time);
    const temps = data.map(d => d.temp);

    const ctx = document.getElementById('tempChart').getContext('2d');

    new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels.reverse(),
            datasets: [{
                label: 'Temperature',
                data: temps.reverse(),
                borderWidth: 2
            }]
        }
    });
}

// gauge 
function createGauge(id, max) {
    return new Chart(document.getElementById(id), {
        type: 'doughnut',
        data: {
            datasets: [{
                data: [0, max],
                backgroundColor: ['green', '#444']
            }]
        },
        options: {
            rotation: -90,
            circumference: 180,
            cutout: '70%',
            plugins: { legend: { display: false } }
        }
    });
}

let tempChart = createGauge('tempChart', 50);
let humChart = createGauge('humChart', 100);
let presChart = createGauge('presChart', 1100);
let airChart = createGauge('airChart', 1000);

// status
function tempStatus(t){
    if(t<18) return ['blue','Холодно'];
    if(t<=25) return ['green','Норма'];
    if(t<=30) return ['orange','Тепло'];
    return ['red','Спекотно'];
}

function humStatus(h){
    if(h<30) return ['orange','Сухо'];
    if(h<=60) return ['green','Комфорт'];
    return ['blue','Вологість висока'];
}

function presStatus(p){
    if(p<990) return ['orange','Низький'];
    if(p<=1020) return ['green','Норма'];
    return ['red','Високий'];
}

function airStatus(v){
    if(v<300) return ['green','Добре'];
    if(v<600) return ['orange','Нормально'];
    return ['red','Погано'];
}

// оновлення
function update(chart, value, max, statusFunc, elementId, unit){
    let [color,text] = statusFunc(value);

    chart.data.datasets[0].data = [value, max-value];
    chart.data.datasets[0].backgroundColor = [color, '#444'];
    chart.update();

    document.getElementById(elementId).innerText =
        value + unit + " - " + text;
}

// якість повітря
function calculateAir(data){
    return (data.co2 + data.nh3 + data.smoke + data.alcohol + data.benzene)/5;
}

// меін оновлення
function updateAll(d){

    update(tempChart, d.temp, 50, tempStatus, "tempText", "°C");
    update(humChart, d.hum, 100, humStatus, "humText", "%");
    update(presChart, d.pres, 1100, presStatus, "presText", " hPa");

    let air = calculateAir(d);
    update(airChart, air, 1000, airStatus, "airText", "");

    document.getElementById("gasList").innerHTML = `
        CO2: ${d.co2} ppm<br>
        NH3: ${d.nh3} ppm<br>
        Smoke: ${d.smoke} ppm<br>
        Alcohol: ${d.alcohol} ppm<br>
        Benzene: ${d.benzene} ppm
    `;
}

setInterval(()=>{
    fetch('/api/data')
    .then(res=>res.json())
    .then(data=>{
        updateAll(data[0]);
    });
},5000);

// графік за тиждень
fetch('/api/history')
.then(res=>res.json())
.then(data=>{
    new Chart(document.getElementById('weekChart'), {
        type: 'line',
        data: {
            labels: data.dates,
            datasets: [
                {label:'Вологість', data:data.hum},
                {label:'Тиск', data:data.pres},
                {label:'Якість повітря', data:data.air}
            ]
        }
    });
});


function setAutoMode() {

    fetch('/led/auto', {
        method:'POST'
    });
}


loadData();