document.addEventListener('DOMContentLoaded', async () => {
    console.log("Ready!");

    // Register buttons
    document.getElementById("btnCalibrate").addEventListener('click', async () => {
        let response = await fetch("/calibrateco2", { method: "POST" });
        console.log(await response.json());
    }, false);

    document.getElementById("btnReset").addEventListener('click', async () => {
        let response = await fetch("/reset", { method: "POST" });
        console.log(await response.json());
    }, false);

    setupSensorDataRefresh();
    setupSettingsForm();
});

async function setupSettingsForm() {
    let settingsFormElement = document.querySelector('#settingsForm');
    if (!settingsFormElement) {
        return;
    }

    document.getElementById("btnRescan").addEventListener('click', async () => {
        await scanForNetworks();
    }, false);

    document.getElementById("btnSave").addEventListener('click', async () => {
        postPreferences();
    }, false);

    document.getElementById("btnOta").addEventListener('click', async () => {
        postOTA();
    }, false);

    document.getElementById("btnResetOta").addEventListener('click', async () => {
        let response = await fetch("/ota/reset", { method: "POST" });
        console.log(await response.json());
    }, false);
    

    setFormAvaliability(false);
    await scanForNetworks();
    await getCurrentSettings();
    setFormAvaliability(true);
}

async function postOTA() {
    let progressBar = document.getElementById("otaProgress");
    progressBar.style.display = "block";

    let file = document.getElementById("otaFile").files[0];
    let xhr = new XMLHttpRequest();
    xhr.open("POST", "/ota", true);
    xhr.setRequestHeader('X-Requested-With', 'XMLHttpRequest');

    xhr.upload.addEventListener("progress", (event) => {
        if (event.lengthComputable) {
            let percentage = Math.round((event.loaded / event.total) * 100) + "%";
            document.getElementById("otaProgressBar").style.width = Math.roundpercentage;
            document.getElementById("otaProgressBar").innerHTML = percentage;
        }
    });

    xhr.onreadystatechange = () => {
        if (xhr.readyState === XMLHttpRequest.DONE) {
            let status = xhr.status;
            if (status >= 200 && status < 400) {
                alert("Upload done, AirQuality will reboot in 5 seconds.")
            } else {
                alert("Upload failed");
            }
        }
        progressBar.style.display = "none";
    };

    xhr.send(file);
}

async function setupSensorDataRefresh() {
    let sensorDataElement = document.querySelector('#sensorData');
    console.log(sensorDataElement);
    if (!sensorDataElement) {
        return;
    }

    setInterval(async () => {
        await refreshSensorValues();
    }, 5000);
    await refreshSensorValues();

    setInterval(async () => {
        await refreshStatus();
    }, 10000);
    await refreshStatus();
}

async function refreshSensorValues() {
    let response = await fetch("/sensorvalues");
    let sensorValues = await response.json();
    document.getElementById("sensorData").innerText = JSON.stringify(sensorValues, null, 4);
}

async function refreshStatus() {
    let response = await fetch("/status");
    let statusJson = await response.json();
    document.getElementById("status").innerText = JSON.stringify(statusJson, null, 4);
}

async function scanForNetworks() {
    setFormAvaliability(false);
    let response = await fetch("/networks");
    let networks = await response.json();

    let networkList = document.getElementById("networkList");

    // Remove all networks
    while (networkList.firstChild) {
        networkList.removeChild(networkList.firstChild);
    }

    // Deduplicate WiFi networks
    let seenNetworks = [];

    // Populate network select
    networks.forEach(network => {
        if (seenNetworks.includes(network.ssid)) {
            return;
        }

        var newLI = document.createElement('li');
        newLI.classList.add('list-unstyled');

        let newOption = document.createElement('input');
        newOption.setAttribute('type', 'radio');
        newOption.classList.add('form-check-input');
        newOption.setAttribute('id', network.ssid);
        newOption.setAttribute('value', network.ssid);
        newOption.setAttribute('name', "wifiSsid");

        newLI.appendChild(newOption);
        newLI.appendChild(document.createTextNode(` ${network.ssid} - ${network.channel} - ${network.rssi} - ${network.authmode}`));

        networkList.appendChild(newLI);

        seenNetworks.push(network.ssid);
    });
    setFormAvaliability(true);
}

async function getCurrentSettings() {
    setFormAvaliability(false);
    let response = await fetch("/preferences");
    let preferences = await response.json();


    document.getElementById("wifiPassword").value = preferences["wifiPassword"];
    document.getElementById("deviceName").value = preferences["deviceName"];
    document.getElementById("deviceVersion").value = preferences["deviceVersion"];
    document.getElementById("mqttHost").value = preferences["mqttHost"];
    document.getElementById("mqttUsername").value = preferences["mqttUsername"];
    document.getElementById("mqttPassword").value = preferences["mqttPassword"];
    document.getElementById("logHost").value = preferences["logHost"];
    document.getElementById("logUsername").value = preferences["logUsername"];
    document.getElementById("logPassword").value = preferences["logPassword"];
    document.getElementById("ledIntensity").value = preferences["ledIntensity"];
    document.getElementById("logValues").checked = preferences["logValues"];



    if (!preferences["provisioningMode"]) {
        document.getElementById("deviceVersion").disabled = true;
    }
    setFormAvaliability(true);

    let radiobtn = document.getElementById(preferences["wifiSsid"]);
    radiobtn.checked = true;
}

async function postPreferences() {
    // Create JSON payload
    var data = {
        deviceName: document.getElementById("deviceName").value,
        wifiSsid: document.querySelector("input[type='radio'][name='wifiSsid']:checked").value,
        wifiPassword: document.getElementById("wifiPassword").value,
        mqttHost: document.getElementById("mqttHost").value,
        mqttUsername: document.getElementById("mqttUsername").value,
        mqttPassword: document.getElementById("mqttPassword").value,
        logHost: document.getElementById("logHost").value,
        logUsername: document.getElementById("logUsername").value,
        logPassword: document.getElementById("logPassword").value,
        ledIntensity: document.getElementById("ledIntensity").value,
        logValues: document.getElementById("logValues").checked
    };

    if (!document.getElementById('deviceVersion').disabled) {
        data["deviceVersion"] = document.getElementById("deviceVersion").value;
    }

    const response = await fetch("/preferences", {
        method: "POST", // or 'PUT'
        headers: {
            "Content-Type": "application/json",
        },
        body: JSON.stringify(data),
    });

    console.log(response);

    getCurrentSettings();
}

function setFormAvaliability(available) {
    // var form = document.getElementById("settingsForm");
    // var elements = form.elements;

    // for (var i = 0; i < elements.length; i++) {
    //     elements[i].disabled = !available;
    // }
}