#명령어 자동화해줌 터미널에 make 쓰면

all: server_fork server_thread

# 1. fork 버전 (매크로 없이 컴파일)
server_fork: server.c
	gcc server.c -o server_fork

# 2. 스레드 버전 (USE_THREAD 정의하고 컴파일)
server_thread: server.c
	gcc -D USE_THREAD -pthread server.c -o server_thread

# 청소용
clean:
	rm server_fork server_thread