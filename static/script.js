const confirmBtn = document.getElementById("confirmSettings");
const latencyInput = document.getElementById("maxLatency");
const pingToggle = document.getElementById("pingEnabled");
const tracertToggle = document.getElementById("trEnabled");

window.onload = function () {
    fetch("http://127.0.0.1:13406/settings.json")
        .then(response => response.json())
        .then(data => {
            if (data.settings.ping && data.settings.ping.maxLatency !== undefined && data.settings.ping.pingEnabled !== undefined) {
                pingToggle.checked = data.settings.ping.pingEnabled;
                latencyInput.value = data.settings.ping.maxLatency;
            }
            if (data.settings.traceroute && data.settings.traceroute.trEnabled !== undefined) {
                tracertToggle.checked = data.settings.traceroute.trEnabled;
            }
        })
        .catch(err => console.error("Error loading settings:", err));
}

confirmBtn.onclick = function () {
    let maxLatency = latencyInput.value;
    let pingEnabled = pingToggle.checked ? 1 : 0;
    let trEnabled = tracertToggle.checked ? 1 : 0;

    if (maxLatency == 0 || maxLatency === '') {
        maxLatency = 80;
        return;
    }

    const originalText = confirmBtn.innerText;
    confirmBtn.innerText = "Saving...";
    confirmBtn.disabled = true;

    fetch("http://127.0.0.1:13406/settings?rtt=" + maxLatency + "&pingEnabled=" + pingEnabled + "&trEnabled=" + trEnabled)
        .then(response => {
            if (response.ok) {
                confirmBtn.innerText = "Saved!";
                confirmBtn.style.backgroundColor = "#155724";
            } else {
                confirmBtn.innerText = "Error";
                confirmBtn.style.backgroundColor = "#dc3545";
            }
        })
        .catch(error => {
            console.error("Error:", error);
            confirmBtn.innerText = "Failed";
            confirmBtn.style.backgroundColor = "#dc3545";
        })
        .finally(() => {
            setTimeout(() => {
                confirmBtn.innerText = originalText;
                confirmBtn.disabled = false;
                confirmBtn.style.backgroundColor = "";
            }, 2000);
        });
};
