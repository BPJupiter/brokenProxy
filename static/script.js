const confirmBtn = document.getElementById("confirmSettings");
const latencyInput = document.getElementById("maxLatency");

window.onload = function () {
    fetch("http://127.0.0.1:13406/settings.json")
        .then(response => response.json())
        .then(data => {
            if (data.settings && data.settings.maxLatency !== undefined) {
                latencyInput.value = data.settings.maxLatency;
            }
        })
        .catch(err => console.error("Error loading settings:", err));
}

confirmBtn.onclick = function () {
    let maxLatency = latencyInput.value;

    if (maxLatency == 0 || maxLatency === '') {
        maxLatency = 80;
        return;
    }

    const originalText = confirmBtn.innerText;
    confirmBtn.innerText = "Saving...";
    confirmBtn.disabled = true;

    fetch("http://127.0.0.1:13406/settings?rtt=" + maxLatency)
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
