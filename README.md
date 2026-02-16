# socket-programming-lab
Beej의 가이드를 기반으로 한 리눅스 C 소켓 프로그래밍 학습 및 채팅 서버 구현

# Multi-Model Network Server: 프로세스와 스레드의 병행 처리 비교 분석

C언어 POSIX 소켓 API를 사용하여 TCP 서버-클라이언트를 구현하고, 다중 접속 처리를 위한 두 가지 핵심 모델(Multi-Process vs Multi-Thread)의 효율성을 비교.

## 주요 구현 기능

* Flexible Network: IPv4/IPv6 주소 체계에 구애받지 않는 getaddrinfo 기반의 독립적 주소.
* Multi-Process (Fork): fork() 시스템 콜을 사용하여 각 요청을 독립된 프로세스로 처리하고, SIGCHLD 핸들러를 통해 좀비 프로세스를 방지.
* Multi-Thread (Pthread): pthread_create를 사용하여 요청당 스레드를 생성하며, 힙 메모리 할당을 통해 스레드 간 데이터 무결성을 보장.
* Automated Build: Makefile의 매크로 정의(-D USE_THREAD)를 활용해 코드 수정 없이 두 가지 모델을 각각 컴파일 가능하게 함.

## 성능 테스트 결과 (Benchmark)

Python 기반 부하 테스트 스크립트(benchmark.py)를 사용하여 3,000명의 동시 접속 상황을 시뮬레이션한 결과.

| 모델 | 동시 접속자 수 | 실행 완료 시간 |
| --- | --- | --- |
| Multi-Process (Fork) | 3,000명 | 5.8634s |
| Multi-Thread (Thread) | 3,000명 | 5.8039s |

### 결과 분석

* 생성 속도: 두 모델 간의 시간 차이는 약 0.05초로 미미함. 현대 리눅스 커널의 프로세스 생성 최적화(Copy-On-Write) 기술로 인해 소규모 환경에서는 생성 비용 차이가 극명하지 않음.
* 자원 점유: 시간 차이는 적으나 htop 모니터링 결과, 스레드 모델이 프로세스 모델 대비 메모리 점유 및 관리 측면에서 압도적인 효율성을 보임을 확인.

##  실험 및 트러블슈팅 (Troubleshooting)

### 1. 시각적 구조 차이 분석

htop 모니터링을 통해 모델별 OS 자원 할당 방식을 시각적으로 검증함.

* Fork 모델: 각 연결이 고유한 PID를 할당받아 독립적으로 나열되는 것을 확인(List View).
![alt text](<스크린샷 2026-02-16 044108-1.png>)

* Thread 모델: 하나의 부모 PID 아래 수천 개의 스레드가 트리 구조로 종속되어 실행되는 것을 확인(Tree View).

![alt text](<스크린샷 2026-02-16 044018-1.png>)


### 2. 시스템 한계와 확장성 (Scalability Error)

동시 접속자를 8,000명으로 증설했을 때 RuntimeError: can't start new thread 발생을 확인하며 다음의 기술적 한계를 분석함.


* 원인: 리눅스 커널의 ulimit(max user processes 및 open files) 제한에 의한 시스템 자원 할당 거부 현상.
* 결론: 단순한 Thread-per-connection 모델로는 수만 명 이상의 대규모 접속 처리에 한계가 있음을 체감함. 이를 해결하기 위한 I/O Multiplexing 및 Event-driven 모델의 필요성을 학습하는 계기가 됨.

## 향후 목표 
I/O Multiplexing 도입: 본 실험을 통해 확인한 Thread-per-connection 모델의 자원 한계를 극복하기 위해, select, poll 또는 리눅스 특화 기술인 epoll을 적용하여 단일 프로세스 내에서 수만 개의 연결을 처리하는 구조로 고도화할 계획.