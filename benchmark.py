#스레드 100개를 동시에 접속 시킴

import socket
import threading
import time

SERVER_IP = '127.0.0.1'
SERVER_PORT = 3490
NUM_CLIENTS = 3000  # 접속할 좀비 숫자 (조절 가능)

def attack_server():
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(10)

        sock.connect((SERVER_IP, SERVER_PORT))
        
        # 데이터 주고 받기
        msg = sock.recv(1024)
        # print(f"받은 메시지: {msg.decode()}") # 출력 많으면 느려지니 주석
        
        # 소켓 닫기 전에 5초만 멍 때리면서 버티기
        # 이러면 서버쪽에서도 연결을 못 끊고 프로세스/스레드를 유지함
        time.sleep(5)

        #여기서 끊어짐
        sock.close()
    except Exception as e:
        print(f"에러 발생: {e}")

print(f"--- {NUM_CLIENTS}명 동시 접속 테스트 시작 ---")
start_time = time.time()

threads = []
for _ in range(NUM_CLIENTS):
    t = threading.Thread(target=attack_server)
    t.start()
    threads.append(t)

for t in threads:
    t.join()

end_time = time.time()
print(f"--- 테스트 종료. 걸린 시간: {end_time - start_time:.4f}초 ---")