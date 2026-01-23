#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#define PORT "3490"     // 서버의 어느 구멍(포트)으로 들어갈지 정함.클라이언트의 포트는 (connect 호출할 때) os가 남는거 아무거나 자동으로 배정해줌
#define MAXDATASIZE 100 // 서버가 보내는 말을 담을 그릇의 크기 100바이트

// 주소 변환 함수, IPv4인지 IPv6인지 판단 후 주소 위치를 찾아냄
void *get_in_addr(struct sockaddr *sa)
{
    // IPv4일때
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    // IPv6일때
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{

    int sockfd, numbytes;
    char buf[MAXDATASIZE]; // 데이터를 받아서 저장할 빈 공책(크기 100)
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    // 실행할 때 서버 주소(IP) 안 적었으면 에러메시지
    // 예:  ./client (x)->./client 127.0.1 (o)
    if (argc != 2)
    {
        fprintf(stderr, "usage:clint hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints); // 힌트 쪽지 초기화
    hints.ai_family = AF_UNSPEC;     // IPv4 IPv6 상관없음
    hints.ai_socktype = SOCK_STREAM; // TCP

    //[중요] argv[1]에는 사용자가 입력한 서버 IP가 들어잇음
    // 서버 때는 AI_PASSIVE(내꺼 써)를 썼지만 클리이언트는 내가 가고 싶은 목적지(argv[1])를 정확히 줌
    // 클라이언트가 연결을 원하는 서버에 대한 정보 가져옴, 결과가 0이 아니면 에러난 것
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // servinfo 리스트에 있는 모든 IP주소를 순회해서 소켓 만들고 연결 시도
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        // 1.전화기(소켓) 구입
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("client:socket");
            continue; // 소켓 못만들면 다음 주소로감
        }

        // (옵션)이 주소로 연결 시도 중이라는 메시지 띄움(네트워크->컴퓨터 방식으로 표기 변환)
        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        printf("client:attempting connection to %s\n", s);

        // 2. 전화 걸기(connect):sockfd를 저 주소(p->ai_addr)에 꽂아라
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            perror("client:connect");
            close(sockfd); // 실패하면 전화기 부수고
            continue;      // 다음 주소 시도
        }

        break; // 연결 성공! 루프 탈출
    }

    // p==NULL이면 연결 가능한 IP주소 후보가 없다는것 또는 다 순회했는데도 연결이 안됐다는것
    if (p == NULL)
    {
        fprintf(stderr, "client:failed to connect\n");
        return 2;
    }

    // 연결 됐다는 메시지 띄움(숫자 변환)
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client:connected to %s\n", s);

    freeaddrinfo(servinfo); // 주소 목록 메모리 해제

    // 대화하기!!
    //  recv():서버가 보낸 데이기
    //  받아서 buf에 저장, 받은 바이트 수 리턴.
    // MAXDATASIZE-1만큼 읽는 이유:맨 뒤에 널문자 넣을 자리 남겨두려고
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1)
    {
        perror("recv");
        exit(1);
    }

    // buf 끝에 0 넣음(끝 표시)
    // 문자열의 끝에는 무조건 '\0'(널문자)가 있어야 컴퓨터가 여기가 끝인 걸 앎
    // recv는 데이터만 받아오고 '\0'는 안붙여주므로 수동으로 붙여야함
    buf[numbytes] = '\0';

    // 받은 메시지 화면에 출력
    printf("client:received '%s'\n", buf);

    // 소켓 메모리 해제. 전화 끊기
    close(sockfd);

    return 0;
}
