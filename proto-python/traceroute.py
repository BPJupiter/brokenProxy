import subprocess

from iocolour import *

def traceroute(address: str):
    try:
        traceroute = subprocess.run(
            ["traceroute", "-n", "-q", "1", "-w", "1", address],
            capture_output=True,
            text=True,
            check=True,
            timeout=15
        )
        print(f"{TRACERT_TAG} Traceroute on {address}")
    except subprocess.CalledProcessError as e:
        print(f"{TRACERT_TAG} {ERR_TAG} Traceroute command failed: {e}")
    except subprocess.TimeoutExpired as e:
        print(f"{TRACERT_TAG} {TIMEOUT_TAG} Traceroute timed out.")
    return traceroute.stdout.strip()
