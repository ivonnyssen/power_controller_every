#ifndef POWER_CONTROLLER_EVERY_PAGE_H
#define POWER_CONTROLLER_EVERY_PAGE_H

#define NUM_PORTS 4

char* items[NUM_PORTS] = {
        F(R"===(Imaging Computer 1)==="),
        F(R"===(Imaging Computer 2)==="),
        F(R"===(Network Switch)==="),
        F(R"===(Port 4)===")
};

char* pageTop[]= {
        F(R"===(<!DOCTYPE html>)==="),
        F(R"===(<html lang="en">)==="),
        F(R"===(<head>)==="),
        F(R"===(<meta charset="UTF-8">)==="),
        F(R"===(<meta name="viewport" content="width=device-width, initial-scale=1">)==="),
        F(R"===(<link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css" integrity="sha384-JcKb8q3iqJ61gNV9KGb8thSsNjpSL0n8PARn9HuZOnIxN0hoP+VmmDGMN5t9UJ0Z" crossorigin="anonymous">)==="),
        F(R"===(<script   src="https://code.jquery.com/jquery-3.5.1.min.js"   integrity="sha256-9/aliU8dGd2tb6OSsuzixeV4y/faTqgFtohetphbbj0="   crossorigin="anonymous"></script>)==="),
        F(R"===(<script src="https://stackpath.bootstrapcdn.com/bootstrap/4.5.2/js/bootstrap.min.js" integrity="sha384-B4gt1jrGC7Jh4AgTPSdUtOBvfO8shuf57BaghqFfPlYxofvL8/KUEfYiJOMMV+rV" crossorigin="anonymous"></script>)==="),
        F(R"===(<script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.3/Chart.bundle.min.js" integrity="sha512-vBmx0N/uQOXznm/Nbkp7h0P1RfLSj0HQrFSzV8m7rOGyj30fYAOKHYvCNez+yM8IrfnW0TCodDEjRqf6fodf/Q==" crossorigin="anonymous"></script>)==="),
        F(R"===(<title>Power Controller</title>)==="),
        F(R"===(</head>)==="),
        F(R"===(<body>)==="),
        F(R"===(<div class="container">)==="),
        F(R"===(<div class="row">)==="),
        F(R"===(<table class="table">)==="),
        F(R"===(<thead>)==="),
        F(R"===(<tr>)==="),
        F(R"===(<th scope="col"></th>)==="),
        F(R"===(<th scope="col">Status</th>)==="),
        F(R"===(<th colspan="2" scope="col">Actions</th>)==="),
        F(R"===(</tr>)==="),
        F(R"===(</thead>)==="),
        F(R"===(<tbody>)==="),
};

char* pageBottom[] = {
        F(R"===(</tbody>)==="),
        F(R"===(</table>)==="),
        F(R"===(</div>)==="),
        F(R"===(<div class="row">)==="),
        F(R"===(<div class="col"><canvas id="temperature"></canvas></div>)==="),
        F(R"===(</div>)==="),
        F(R"===(<script>)==="),
        F(R"===(let ctx1 = document.getElementById('temperature').getContext('2d');)==="),
        F(R"===(let labels = [];)==="),
        F(R"===(let pressureData = [];)==="),
        F(R"===(let temperatureData = [];)==="),
        F(R"===(let humidityData = [];)==="),
        F(R"===(window.fetch('sensors.jsn'))==="),
        F(R"===(.then(response => response.json()))==="),
        F(R"===(.then(data => data["values"].forEach((i) => {)==="),
        F(R"===(labels.push(i["time"]);)==="),
        F(R"===(pressureData.push(i["pressure"]);)==="),
        F(R"===(temperatureData.push(i["temp"]);)==="),
        F(R"===(humidityData.push(i["humidity"]);)==="),
        F(R"===(})))==="),
        F(R"===(.then()==="),
        F(R"===(() => {new Chart(ctx1, {)==="),
        F(R"===(type: 'line',)==="),
        F(R"===(data: {)==="),
        F(R"===(labels: labels,)==="),
        F(R"===(datasets: [{)==="),
        F(R"===(label: 'Temperature',)==="),
        F(R"===(fill: false,)==="),
        F(R"===(borderWidth: 1,)==="),
        F(R"===(backgroundColor: [)==="),
        F(R"===('rgba(255, 0, 0, 1)')==="),
        F(R"===(],)==="),
        F(R"===(borderColor: [)==="),
        F(R"===('rgba(255, 0, 0, 1)')==="),
        F(R"===(],)==="),
        F(R"===(yAxisID: 'y-axis-1',)==="),
        F(R"===(data: temperatureData,)==="),
        F(R"===(}, {)==="),
        F(R"===(label: 'Humidity',)==="),
        F(R"===(fill: false,)==="),
        F(R"===(borderWidth: 1,)==="),
        F(R"===(backgroundColor: [)==="),
        F(R"===('rgba(0, 0, 255, 1)')==="),
        F(R"===(],)==="),
        F(R"===(borderColor: [)==="),
        F(R"===('rgba(0, 0, 255, 1)')==="),
        F(R"===(],)==="),
        F(R"===(yAxisID: 'y-axis-1',)==="),
        F(R"===(data: humidityData,)==="),
        F(R"===(}, {)==="),
        F(R"===(label: 'Barometric Pressure',)==="),
        F(R"===(fill: false,)==="),
        F(R"===(borderWidth: 1,)==="),
        F(R"===(backgroundColor: [)==="),
        F(R"===('rgba(0, 255, 0, 1)')==="),
        F(R"===(],)==="),
        F(R"===(borderColor: [)==="),
        F(R"===('rgba(0, 255, 0, 1)')==="),
        F(R"===(],)==="),
        F(R"===(yAxisID: 'y-axis-2',)==="),
        F(R"===(data: pressureData,)==="),
        F(R"===(}])==="),
        F(R"===(},)==="),
        F(R"===(options: {)==="),
        F(R"===(scales: {)==="),
        F(R"===(yAxes: [{)==="),
        F(R"===(type: 'linear',)==="),
        F(R"===(display: true,)==="),
        F(R"===(position: 'left',)==="),
        F(R"===(id: 'y-axis-1',)==="),
        F(R"===(ticks: {)==="),
        F(R"===(beginAtZero: false)==="),
        F(R"===(})==="),
        F(R"===(}, {)==="),
        F(R"===(type: 'linear',)==="),
        F(R"===(display: true,)==="),
        F(R"===(position: 'right',)==="),
        F(R"===(id: 'y-axis-2',)==="),
        F(R"===(ticks: {)==="),
        F(R"===(beginAtZero: false)==="),
        F(R"===(})==="),
        F(R"===(}])==="),
        F(R"===(})==="),
        F(R"===(})==="),
        F(R"===(});})==="),
        F(R"===())==="),
        F(R"===(.catch((error) => {)==="),
        F(R"===(console.log('Error:', error);)==="),
        F(R"===(});)==="),
        F(R"===(</script>)==="),
        F(R"===(</div>)==="),
        F(R"===(</body>)==="),
        F(R"===(</html>)===")
};

#endif //POWER_CONTROLLER_EVERY_PAGE_H
