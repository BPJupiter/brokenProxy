document.getElementById("confirmSettings").onclick = function () {
  var maxLatency = document.getElementById("maxLatency").value;
  if (maxLatency === '') {
    maxLatency = -1;
  }
  fetch("http://127.0.0.1:8080/settings?rtt=" + maxLatency);
};
