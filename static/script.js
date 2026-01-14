const confirmBtn = document.getElementById("confirmSettings");
const latencyInput = document.getElementById("maxLatency");

const pingToggle = document.getElementById("pingEnabled");
const tracertToggle = document.getElementById("trEnabled");
const locDNSToggle = document.getElementById("locDNSEnabled");

const map = new maplibregl.Map({
    container: 'viewDiv',
    style: './maplibre/positron-no-labels.json',
    center: [0, 0],
    zoom: 1
});
let allCableIds = [];
let hoveredStateId = null;
let disabledCables = new Set();

function updateMapVisuals() {
    if (allCableIds.length === 0) return;

    allCableIds.forEach(id => {
        const isActive = !disabledCables.has(id);
        setCableState(id, isActive);
    })
}

window.onload = function () {
    fetch("http://127.0.0.1:13406/settings.json")
        .then(response => response.json())
        .then(data => {
            if (data.settings.ping) {
                pingToggle.checked = data.settings.ping.pingEnabled;
                if (data.settings.ping.maxLatency !== undefined) {
                    latencyInput.value = data.settings.ping.maxLatency;
                }
            }
            if (data.settings.traceroute) {
                tracertToggle.checked = data.settings.traceroute.trEnabled;
            }
            if (data.settings.localDNS) {
                locDNSToggle.checked = data.settings.localDNS.locDNSEnabled;
            }

            if (data.settings.disabledCables && Array.isArray(data.settings.disabledCables)) {
                data.settings.disabledCables.forEach(id => disabledCables.add(id));
            }
        })
        .catch(err => console.error("Error loading settings:", err));

    map.on('load', () => {
        fetch('./telegeography/cable-geo.json')
            .then(response => response.json())
            .then(geojsonData => {
                allCableIds = geojsonData.features.map(f => f.properties.id);

                map.addSource('cable-geo', {
                    'type': 'geojson',
                    'data': geojsonData,
                    'promoteId': 'id'
                });

                map.addLayer({
                    'id': 'cables-lines',
                    'type': 'line',
                    'source': 'cable-geo',
                    'filter': ['!=', ['get', 'color'], '#939597'],
                    'layout': {
                        'line-join': 'round',
                        'line-cap': 'round',
                    },
                    'paint': {
                        'line-color': [
                            'case',
                            ['boolean', ['feature-state', 'active'], true],
                            ['get', 'color'],
                            '#929496'
                        ],
                        'line-opacity': [
                            'case',
                            ['boolean', ['feature-state', 'active'], true],
                            1.0,
                            0.4
                        ],
                        'line-width': [
                            'case',
                            ['boolean', ['feature-state', 'hover'], false],
                            5,
                            2
                        ]
                    }
                });

                updateMapVisuals();
                //console.log(new Blob([JSON.stringify(allCableIds)]).size);
            })
            .catch(err => console.error("Error loading GeoJSON:", err));

        map.on('click', 'cables-lines', (e) => {
            const coordinates = e.lngLat;
            const properties = e.features[0].properties;

            const description = `<strong>${properties.name}</strong><br>ID: ${properties.id}`;

            new maplibregl.Popup()
                .setLngLat(coordinates)
                .setHTML(description)
                .addTo(map);

            const featureId = e.features[0].id;

            if (disabledCables.has(featureId)) {
                disabledCables.delete(featureId);
            } else {
                disabledCables.add(featureId)
            }

            toggleCableState(featureId, disabledCables.has(featureId));
        });

        map.on('mouseenter', 'cables-lines', (e) => {
            map.getCanvas().style.cursor = 'pointer';

            if (e.features.length > 0) {
                if (hoveredStateId !== null) {
                    map.setFeatureState(
                        { source: 'cable-geo', id: hoveredStateId },
                        { hover: false }
                    );
                }

                hoveredStateId = e.features[0].id;
                map.setFeatureState(
                    { source: 'cable-geo', id: hoveredStateId },
                    { hover: true }
                );
            }
        });

        map.on('mouseleave', 'cables-lines', () => {
            map.getCanvas().style.cursor = '';

            if (hoveredStateId !== null) {
                map.setFeatureState(
                    { source: 'cable-geo', id: hoveredStateId },
                    { hover: false }
                );
            }

            hoveredStateId = null;
        });
    });
}

confirmBtn.onclick = function () {
    const payload = {
        maxLatency: latencyInput.value ? parseFloat(latencyInput.value) : 80.0,
        pingEnabled: pingToggle.checked,
        trEnabled: tracertToggle.checked,
        locDNSEnabled: locDNSToggle.checked,
        disabledCables: Array.from(disabledCables),
    };

    const originalText = confirmBtn.innerText;
    confirmBtn.innerText = "Saving...";
    confirmBtn.disabled = true;

    fetch("http://127.0.0.1:13406/settings", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify(payload),
    })
        .then(response => {
            if (response.ok) {
                confirmBtn.innerText = "Saved!";
                confirmBtn.style.backgroundColor = "#155724";
            } else {
                throw new Error("Server responded with " + response.status);
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

function turnAllCablesOff() {
    allCableIds.forEach(id => disabledCables.add(id));
    updateMapVisuals();
}

function turnAllCablesOn() {
    disabledCables.clear();
    updateMapVisuals();
}

function setCableState(cableId, isActive) {
    map.setFeatureState(
        { source: 'cable-geo', id: cableId },
        { active: isActive }
    )
}

function toggleCableState(cableId, isCurrentlyActive) {
    map.setFeatureState(
        { source: 'cable-geo', id: cableId },
        { active: !isCurrentlyActive }
    );
}
