
#define _POSIX_C_SOURCE 200112L // 옛날 방식 말고 POSIX(유닉스 표준) 2001년 버전 이상 기능 사용 선언
#define _XOPEN_SOURCE 700       // 최신 유닉스 기능 사용 선언

#include <pthread.h> //thread 라이브러리!!->이건 표준 c 라이브러리에 없어서 컴파일 명령 시 따로 링크를 걸어줘야함 gcc server.c -o server -pthread

// 기본 c언어 도구
#include <stdio.h>  //standard input/output:printf()->출력,fprintf()->에러출력,perror()->에러 메시지 자동 출력
#include <stdlib.h> //standard library. 프로그램을 강제 종료하거나 메모리 관련 일 할 때 씀. exit()->프로그램 즉시 종료.에러 시 사용
#include <string.h> //구조체를 깨끗이 지우거나 문자열 길이를 잴 때 씀. memset(),strlen()

// os 시스템 도구(리눅스에만 있는 기능)
#include <unistd.h>    //unix standard. fork(),close(),read(),write()
#include <errno.h>     //시스템 함수가 실패했을 때 왜 실패했는지 이유(번호)를 알아내려고 씀. errno->전역 에러 변수
#include <sys/types.h> //pid_t(프로세스 id),size_t(크기)같은 데이터 타입(자료형)들이 정의되어있음. 다른 헤더들의 기반이 되는 헤더

// 네트워크 핵심 도구. 네트워크 통신 필수템
#include <sys/socket.h> //소켓을 만들고 연결하는 모든 핵심함수가 있음. socket(),bind(),listen(),accept(),recv()
#include <netinet/in.h> //주소 양식(구조체)와 바이트 순서 변환. struct sockaddr_in(IPv4 주소판),htons()->'포트' 번호 뒤집기(리틀 엔디안,빅엔디안)
#include <arpa/inet.h>  //IP주소를 문자열<->숫자로 변환할 때 씀. 사람 눈에 보기 좋은 'IP주소' 형식에서 컴퓨터를 위한 이진수로 바꾸기 같은것. inet_pton(),inet_ntop()

// 도우미&좀비 청소부
#include <netdb.h> //복잡한 주소 설정을 한방에 해주는 getaddrinfo(),freeaddrinfo(),struct addrinfo
// 자식 프로세스가 죽어서 좀비가 됐을 때 자동으로 치워주는 청소부(sigchld_handler)를 만들기 위해 필요.wiatpid()->시체 치우기, sigaction()->청소부 등록하기
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490" // 연결할 포트 번호
#define BACKLOG 10  // 대기 인원 10

// 새로 만든 스레드가 할 일을 정의하는 함수
void *handle_client(void *arg)
{
    // 1.인자(소켓) 받아오기&메모리 해제
    // arg는 힙 메모리의 주소. 거기로 가서 소켓 번호를 꺼내와야함(*붙이기)
    int new_fd = *((int *)arg);

    free(arg); // [중요] 소켓 번호 꺼냈으면 포장지(메모리)는 회수

    // 2.알아서 죽은 스레드 사라짐(os가 자동으로 완전히 회수)
    pthread_detach(pthread_self());

    printf(">>>알바생:손님 받은! 10초동안 처리하는 척 함...\n");
    sleep(10); // 10초 멈춤

    // 3.손님한테 인사 메시지 전송
    if (send(new_fd, "Hello,world", 13, 0) == -1)
    {
        perror("send");
    }

    // 4.전화 끊기
    close(new_fd);

    return NULL;
}
/*
// 종료된 자식을 부모가 처리(좀비 방지)
void sigchld_handler(int s)
{
    (void)s; // 1.경고 끄기 용

    int saved_errno = errno; // 2.에러 번호 백업(중요!)

    // 3.죽은 자식 모두 청소
    //-1:아무 자식이나 죽은 놈 와라
    // WNOHANG:기다리지 않음(wait no hang). 죽은 자식이 없으면 그냥 바로 루프 끝내고 나감. 이거 안 쓰면 자식 죽을 때까지 부모 멈춤
    // while:동시에 여러 자식 죽을 수도 있으니까 죽은 애들은 한 번에 싹 다 처리하려고
    while (waitpid(-1, NULL, WNOHANG > 0))
        ;

    errno = saved_errno; // 4.에러 번호 복구
}
*/

// 주소 변환 함수:ipv4인지 ipv6인지 확인해서 알맞은 ip 주소 위치를 뽑아줌
// struct sockaddr은 겉표지일 뿐, 알맹이는 IPv4용 sockaddr_in과 IPv6용 sockaddr_in6구조체가 다름
// 그걸 구분해서 진짜 IP주소가 적힌 위치만 뽑아주는 수
void *get_in_addr(struct sockaddr *sa)
{
    // IPv4라면
    if (sa->sa_family == AF_INET)
    {
        // sockaddr_in으로 형변환해서 sin_addr(IP주소) 리턴
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    // IPv6라면 sockaddr_in6로 형변환해서 sin6_addr 리턴
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(void)
{

    int sockfd, new_fd; // sockfd:문지기 소켓, new_fd:손님 담당 소켓
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // 접속한 클라이언트 정보 담을 넉넉한 공간
    socklen_t sin_size;
    struct sigaction sa; // 좀비 청소부 설정용 구조체
    int yes = 1;
    char s[INET6_ADDRSTRLEN]; // IP주소 문자열 담을 그릇
    int rv;

    //[핵심] hints에 원하는 조건(TCP,내 IP 사용)을 적고 getaddrinfo 부르면 servinfo라는 리스트에 "사용 가능한 주소 정보들"이 김

    memset(&hints, 0, sizeof hints); // 구조체 초기화
    hints.ai_family = AF_INET;       // IPv4만 쓴다(AF_UNSPEC)이면 둘다
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // server IP는 자동으로 채워짐

    // getaddrinfo에게 서버 정보 얻어오기
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // 소켓 생성 및 바딩
    //  가져온 모든 ip 주소를 순회해서 socket()으로 sockfd 가져오고 bind() 시도, 될때까지 반복
    for (p = servinfo; p != NULL; p = p->ai_next)
    {

        // 1. 전화기(소켓) 구입 socket()
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server:socket: ");
            continue;
        }

        // 2.이미 사용중인 주소(address already in use)라도 그냥 재사용 가능
        //->서버를 껐다(ctrl+c) 다시 바로 켜면 os가 방금 그 포트 연결의 잔재가 남아있어서 못 쓴다고 에러냄(안전을 위해)
        // 개발할 때는 껐다 켰다 반복하니까 이 옵션 없으면 매번 1~2분 기다려야함
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        // 3. 전화번호(포트) 확정 위해 bind() 시도
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            // bind() 실패 시 만들었던 sockfd도 닫고 다음 ip 주소로 감
            close(sockfd);
            perror("server:bind");
            continue;
        }

        break; // 성공했으면 bind() 시도 끝!
    }

    // 연결 끝났으면 servinfo 메모리 해제
    freeaddrinfo(servinfo);

    // 아무런 ip주소도 안 갖고 온 경우는 연결 실패
    if (p == NULL)
    {
        fprintf(stderr, "server:failed to bind\n");
        exit(1);
    }

    // listen(). 대기열(BACKLOG)만큼 손님 받을 준비. 실패시 에러 메시지 띄우고 탈출
    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    while (1)
    {
        printf("server:waiting for connections...\n");

        sin_size = sizeof their_addr;

        // 1.대기타다가 손님 오면 'new_fd' 생성
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        // 생성 못하고 에러날 때
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        // 2. [중요] 소켓 번호 안전하게 복사
        //->그냥 넘기면 worker_thread에서 그 주소값 읽기도 전에 메인 스레드가 while 루프 돌아서 다음 손님 번호를 new_fd에 덮어 쓸 수 있기 때문
        int *client_socket = malloc(sizeof(int));
        *client_socket = new_fd; // 힙메모리에 따로 공간 파서 복사해두고 그 포인터를 넘김

        // 3.접속한 손님 IP 주소 찍어보기
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server:got connection from %s\n", s);

        // 4.스레드 id 변수 선언(os가 스레드 생성되면 고유 id 줌)
        //->detach 쓸거면 이 id를 나중에 쓸 일은 없지만 pthread_create 함수가 무조건 id 적을 공간을 요구해서 형식 상 만듦
        pthread_t worker_thread;

        // 5.pthread_create 호출(handle_client 실행)해서 스레드 생성
        //&worker_thread: 새로 만든 스레드 id 적을 변수의 주소
        // NULL:스레드 우선순위/스택 크기는 일단 기본 설정으로
        // handle_client:스레드 탄생하자마자 실행할 함수
        // client_socket:힙메모리에 복사해둔 new_fd를 void *arg로 전달
        if (pthread_create(&worker_thread, NULL, handle_client, client_socket))
        {
            perror("pthread_create");
            // 스레드 생성 실패 시 메모리 해제하고 소켓 닫기
            free(client_socket);
            close(new_fd);
        }

        //[중요] 메인 스레드는 new_fd를 close하지 않음!! 스레드가 쓸거니까
        // 메인 스레드는 다시 손님을 기다리러 while 처음으로 감
    }

    /*


    //[청소부 등록 과정] 자식 죽었다고 신호(SIGCHLD)가 오면 자동으로 청소부 함수 실행
    sa.sa_handler = sigchld_handler; // 청소부는 아까 만든 걔로 등록
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // 핵심! 자식 죽은 신호 와도 부모 프로세스(서버)는 하던거(accept 함수 안에서 손님 대기) 계속해라
                              //->이거 없으면 자식 하나 죽을때마다 서버 꺼질수도 음
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

     // 무한루프:손님 받기 accept()&fork()->여기장
    while (1)
    {
        printf("server:waiting for connections...\n");

        sin_size = sizeof their_addr;

        // 1.대기타다가 손님 오면 'new_fd' 생성
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        // 생성 못하고 에러날 때
        if (new_fd == -1)
        {
            perror("accept");
            continue;
        }

        // 2.접속한 손님 IP 주소 찍어보기
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        printf("server:got connection form %s\n", s);

        // 3.분신술:fork()한 다음 자식 프로세스(부모는 0받으니까)일 경우 if문 안 실행
        if (!fork())
        {
            //=====자식 프로세스 구역======

            close(sockfd); // 자식 프로세스니까 부모 프로세스의 sockfd(문지기 소켓) 필요없음

            // 손님한테 인사 메시송
            if (send(new_fd, "Hello,world", 13, 0) == -1)
            {
                perror("send");
            }

            close(new_fd); // 자식:할 말 끝났으니 손님 보냄
            exit(0);       // 자식:퇴근함(프로세스 종료->좀비됨)
        }

        //======부모 프로세스 구역=====

        // 부모 프로세스는 자식 프로세스에게만 필요한 new_fd(하나의 연결만 담당하는 소켓) 필요없음
        // 이거 안 닫으면 부모한테 파일 스크립터가 쌓여서 서버 터짐계속
        close(new_fd);
    } // 다시 while문 처음으로 돌아가서 accpet 대기
    */

    return 0;
}
