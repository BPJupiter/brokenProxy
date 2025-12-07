class bcolours:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

TRACERT_TAG = f"{bcolours.OKGREEN}[Traceroute]{bcolours.ENDC}"
DNS_TAG = f"{bcolours.OKCYAN}[DNS Lookup]{bcolours.ENDC}"
ERR_TAG = f"{bcolours.FAIL}[Error]{bcolours.ENDC}"
TIMEOUT_TAG = f"{bcolours.WARNING}[Timeout]{bcolours.ENDC}"
PROXY_TAG = f"{bcolours.OKBLUE}[Proxy]{bcolours.ENDC}"
