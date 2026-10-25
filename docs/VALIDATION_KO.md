# 검증 기록

## 실행 결과

- 기준 날짜: 2026-10-25 (Asia/Seoul)
- ARM GNU Toolchain 15.2.1, Cortex-M4 hard-float
- PC GCC 16.1.0
- C99, -O2, -Wall -Wextra -Werror
- 정적 라이브러리 libmotor_framework.a 생성 성공
- STM32 예제 전체 컴파일·링크·BIN 생성 성공
- ELF 미해결 심볼 없음
- ELF 시작점: 0x0800019d (__start의 Thumb 주소)
- text 3416 bytes / data 0 bytes / bss 152 bytes
- 코어 8개 그룹: 20,324개 assertion 통과
- 프레임워크 API 5개 그룹: 286개 assertion 통과
- 합계 13개 그룹: 20,610개 assertion 통과

## 프레임워크 API 통합 테스트

tests/test_framework.c는 시간·버튼·출력·critical section을 가상 보드로 연결하고
모듈을 따로 흉내 내는 대신 실제 MF_Init/MF_Process 등의 공개 API를 실행합니다.

1. 유효성: NULL·누락 콜백·시간 순서 오류·잘못된 기본 듀티를 거부
2. API: 비동기 접수, 초기 정지, 변경이 없을 때 출력 콜백 생략, 듀티, 설정된 250ms 반전 대기, 정지 취소
3. ISR 입력: 정지 우선, 후속 구동 폐기, UART 오류·큐 포화 시 정지, main 명령 큐 포화
4. 설정된 버튼 시간: 5ms 디바운싱·100ms 계속 누름 정지 및 정지 후 해제 억제
5. 인스턴스 분리: 출력 상태 독립과 기존 critical section 상태 복원

기존 코어 테스트에는 기본 1000ms 방향 전환, 버튼 경계·반복 입력,
uint32_t 시간 순환과 10,000단계 명령 조합이 포함됩니다.

## 범위

PC 테스트는 하드웨어 레지스터 및 실제 CPU 선점 시점까지 검증하지 않습니다.
ARM 빌드 성공은 실제 모터 구동 성공을 뜻하지 않습니다.
보드 다운로드·PWM 파형·버튼 접점·마스킹 시간·물리 정지 지연은 측정하지 않았습니다.

원본 및 optimized 폴더 무변경 여부는 별도의 SHA-256 확인 기록을 참고하세요.
