import socket
import threading
import traceback
import sys
import subprocess
import time

from dnslookup import dns_resolve, DNS_TAG
from iocolour import *
from traceroute import traceroute

HOST = '127.0.0.1'
PORT = 8080

def host_port_from_url(url):
    http_pos = url.find(b"http://")
    if http_pos == -1:
        http_pos = url.find(b"https://")
    if http_pos != -1:
        temp = url[http_pos + 7:]
        host_end_pos = temp.find(b'/')
        if host_end_pos == -1:
            host_end_pos = len(temp)
        host = temp[:host_end_pos].decode('utf-8')
        port = 80 if b"https://" not in url else 443
    else:
        url_end_pos = url.find(b':')
        if url_end_pos == -1:
            host = url.decode('utf-8')
            port = 80 # default to http
        else:
            host = url[:url_end_pos].decode('utf-8')
            port = int(url[url_end_pos + 1:].decode('utf-8'))
    return host, port

def handle_client(client_socket):
    try:
        request = client_socket.recv(4096)
        if not request:
            return

        first_line = request.split(b'\n')[0]
        url = first_line.split(b' ')[1]

        host, port = host_port_from_url(url)

        destination_ip = dns_resolve("j", host)
        if destination_ip == None:
            print(f"{DNS_TAG} {ERR_TAG} Failed to resolve hostname {host}")
            return
        print(f"{DNS_TAG} {host} resolved to {destination_ip}")

        tr_output = traceroute(destination_ip)

        remote_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        remote_socket.connect((destination_ip, port))

        print(f"{PROXY_TAG} Accepted request for: {host}:{port}")

        if b"CONNECT" in first_line:
            connection_established_response = b"HTTP/1.1 200 Connection Established\r\n\r\n"
            client_socket.sendall(connection_established_response)
            print(f"{PROXY_TAG} Client established connection!")
        else:
            remote_socket.sendall(request)
            print(f"{PROXY_TAG} Remote socket sent request!")

        def tunnel_data(source, destination):
            source.settimeout(15)
            while True:
                try:
                    data = source.recv(4096)
                    if not data:
                        break
                    destination.sendall(data)
                except socket.error:
                        break

        client_to_remote_thread = threading.Thread(target=tunnel_data, args=(client_socket, remote_socket))
        remote_to_client_thread = threading.Thread(target=tunnel_data, args=(remote_socket, client_socket))

        client_to_remote_thread.daemon = True;
        remote_to_client_thread.daemon = True;

        client_to_remote_thread.start()
        remote_to_client_thread.start()

        client_to_remote_thread.join()
        remote_to_client_thread.join()

    except Exception as e:
        print(f"{PROXY_TAG} {ERR_TAG} Error handling client: {e}")
        print(f"{PROXY_TAG} {ERR_TAG} URL: {url}")
        exc_type, exc_value, exc_traceback = sys.exc_info()
        tb_info = traceback.extract_tb(exc_traceback)
        filename, line_number, function_name, line_text = tb_info[-1]

        print(f"{PROXY_TAG} {ERR_TAG} Exception Type: {exc_type.__name__}")
        print(f"{PROXY_TAG} {ERR_TAG} Exception Value: {exc_value}")
        print(f"{PROXY_TAG} {ERR_TAG} File: {filename}")
        print(f"{PROXY_TAG} {ERR_TAG} Line Number: {line_number}")
        print(f"{PROXY_TAG} {ERR_TAG} Function: {function_name}")
        print(f"{PROXY_TAG} {ERR_TAG} Line of code: {line_text}")
    finally:
        client_socket.close()
        if 'remote_socket' in locals():
            remote_socket.close()
    return

def start_proxy_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((HOST, PORT))
    server_socket.listen(8)

    print(f"{PROXY_TAG} Proxy server listening on {HOST}:{PORT}")

    while True:
        client_socket, addr = server_socket.accept()
        print(f"{PROXY_TAG} Accepted connection from {addr[0]}:{addr[1]}")

        client_handler = threading.Thread(target=handle_client, args=(client_socket,))
        client_handler.daemon = True
        client_handler.start()
        print(f"{bcolours.WARNING}[Info]{bcolours.ENDC} ThreadCount: {threading.active_count()}")

if __name__ == "__main__":
    start_proxy_server()
