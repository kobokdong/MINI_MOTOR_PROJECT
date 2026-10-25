# MINI MOTOR FRAMEWORK

**기존 MINI_MOTOR_PROJECT_optimized를 재사용 가능한 C 라이브러리, 보드 포트, 응용 예제로 분리한 버전**입니다.

- 원본 MINI_MOTOR_PROJECT와 기존 optimized 폴더는 수정하지 않았습니다.
- RTOS 없이 메인 반복문에서 동작하는 정적 할당 기반 프레임워크입니다.
- STM32F411용 포트와 실제 빌드 가능한 예제를 포함합니다.
- 제어 코어에는 STM32 레지스터·CMSIS 헤더가 없습니다.
- 라이브러리 함수와 타입에는 MF 접두사를 적용했습니다.
- 버전: 1.0.0

## 디렉터리 구조

```text
MINI_MOTOR_FRAMEWORK/
├── framework/
│   ├── include/
│   │   └── motor_framework.h       # 응용에서 사용하는 공개 API
│   └── src/
│       ├── motor_framework.c       # 초기화·입력·제어 흐름 통합
│       ├── controller.c            # 모터 상태 머신
│       ├── button.c                # 버튼 시간·디바운싱
│       └── input.c                 # 명령 큐·우선 정지
├── ports/stm32f411/
│   ├── board.c, board.h            # 프레임워크와 STM32의 연결부
│   ├── motor.c                    # TIM5 및 PA0/PA1
│   ├── uart.c                     # USART2 설정
│   ├── key.c                      # PC13 원시 입력
│   ├── systick.c                  # 1ms 시계
│   ├── clock.c                    # 96MHz 클록
│   ├── crt0.s                     # 시작 코드
│   └── rom_0x08000000.lds          # 메모리 배치
├── examples/stm32f411_motor/
│   └── main.c                     # 프레임워크 사용 예제
├── tests/                         # 코어 및 공개 API 테스트
├── docs/                          # API·포팅·설계·검증 설명
├── build/                         # .a / ELF / BIN / 테스트 결과물
└── build.ps1
```

framework/include의 controller.h, button.h, input.h는 컨텍스트 정의에 필요한 내부 모듈 헤더입니다.
응용에서는 motor_framework.h와 MF_Init/Process 등의 상위 API를 사용하고,
인스턴스 내부 필드를 직접 변경하지 않습니다.

## 기본 사용

실행 가능한 예제는 examples/stm32f411_motor/main.c입니다.

```c
#include "board.h"

static MF_Instance motor;

void Main(void)
{
    MF_Config config = MF_DefaultConfig();
    MF_Port port;

    Board_Init();
    port = Board_MotorPort();

    config.reverse_delay_ms = 1000u;
    config.default_level = 1u;

    if (!MF_Init(&motor, &config, &port)) {
        for (;;) { }
    }

    Board_StartReception(&motor);

    for (;;) {
        MF_Process(&motor);
    }
}
```

이 예제는 UART 또는 버튼 입력을 기다리며, 부팅 후 자동으로 회전하지 않습니다.

## 응용 코드에서 명령 보내기

```c
MF_Command(&motor, MF_COMMAND_CW, 0u);
MF_Command(&motor, MF_COMMAND_LEVEL, 5u); /* 70% */
MF_Stop(&motor);
```

MF_Command/MF_Stop은 **비동기 요청**입니다.
실제 출력은 다음 MF_Process에서 바뀝니다.
위 세 함수를 연속으로 호출한 뒤 Process하면 정지가 우선하므로 회전하지 않습니다.
각 동작을 확인하려면 응용의 이벤트 처리 시점마다 명령을 요청하고,
메인 반복문은 계속 MF_Process를 실행해야 합니다.

MF_Stop은 보드와 무관한 프로그램 명령이며, 물리 비상 정지 기능이 아닙니다.

## API

| API | 호출 위치 | 역할 |
| --- | --- | --- |
| MF_DefaultConfig | 초기화 코드 | 기본 설정 생성 |
| MF_Init | IRQ 수신 활성화 전 | 설정 검증, 상태 초기화, 정지 출력 |
| MF_Process | 메인 반복문 | 입력 처리, 상태 전환, 필요할 때 출력 콜백 |
| MF_Command | 메인 | 방향·정지·듀티 명령 요청 |
| MF_Stop | 메인 | 정지 요청 |
| MF_GetStatus | 메인 | 상태·듀티·입력 오류의 일관된 스냅샷 |
| MF_OnByteISR | 직렬화된 수신 ISR | F/R/S/0/1~9 명령 바이트 전달 |
| MF_OnRxErrorISR | 수신 ISR | 수신 오류에 따른 정지 요청 |

상세 계약은 [API_KO.md](docs/API_KO.md)에 있습니다.

## 변경 가능한 설정

설정은 인스턴스 초기화 시 복사되며 이후 직접 변경하지 않습니다.

| 설정 | 기본값 | 의미 |
| --- | --- | --- |
| reverse_delay_ms | 1000 | 반대 방향 전환 전 정지 시간 |
| debounce_ms | 20 | 버튼 안정 상태 확인 시간 |
| short_press_ms | 900 | 짧게 누름의 최대 시간 |
| long_release_ms | 2100 | 해제 시 길게 누름 판정 시간 |
| hold_stop_ms | 3000 | 해제하지 않아도 정지하는 시간 |
| default_level | 1 | 기본 듀티 단계, 50% |

PWM 주파수·핀·UART 속도는 보드 의존 설정이므로 ports/stm32f411에서 변경합니다.
현재 설정은 PA0/PA1 TIM5 10kHz, PC13 active LOW, USART2 PA2/PA3 115200 8N1입니다.

## 기본 동작 정책

- F/f: CW / R/r: CCW / S/s/0: 정지 / 1~9: 듀티 50~90%
- 버튼 시간 판정과 정지·방향 전환 정책은 optimized 버전을 유지합니다.
- 반대 방향 요청은 출력을 정지하고 설정된 시간 대기 후 적용합니다.
- 정지는 목표 방향과 기존 명령 큐를 취소합니다.
- 정지 요청을 처리하기 전에 들어온 다른 구동 명령은 실행하지 않습니다.
- 버퍼 포화·수신 오류는 정지 요청으로 처리합니다.
- 대기 중 추가 버튼 짧게 누름은 무시하며, UART 목표 방향·듀티 갱신은 허용합니다.
- 정지 직후 반대 방향 요청도 남은 대기 시간을 지킵니다.
- 인스턴스마다 컨텍스트가 독립적입니다. 제공된 STM32 보드 포트는 물리 모터 1개용입니다.

## 빌드와 테스트

PowerShell에서 이 폴더를 기준으로 실행합니다.

```powershell
.\build.ps1
.\tests\run_tests.ps1
```

기본 도구 경로:

- ARM: C:\arm-gnu-toolchain-15.2.rel1-mingw-w64-i686-arm-none-eabi
- PC 테스트: C:\msys64\ucrt64\bin\gcc.exe

다른 설치 경로는 각각 -ToolDir / -Compiler 옵션으로 지정합니다.

생성되는 주요 결과물:

- build/libmotor_framework.a — Cortex-M4 hard-float용 제어 정적 라이브러리
- build/rom_0x08000000.elf — STM32F411 예제 실행 파일
- build/rom_0x08000000.bin — 다운로드용 바이너리

보드 다운로드는 스크립트에 포함하지 않았습니다.
다른 MCU나 ABI로 옮길 때는 기존 .a를 그대로 쓰지 말고 framework/src를 해당 도구로 다시 컴파일해야 합니다.

## 검증 결과와 한계

- ARM 컴파일·정적 라이브러리 생성·ELF 링크·BIN 생성 성공
- -Wall -Wextra -Werror 통과
- 기존 로직: 20,324개 검사 / 공개 API 통합: 286개 검사 통과
- 기본 예제: text 3416 bytes, data 0 bytes, bss 152 bytes
- 실제 보드에서 PWM·버튼·마스킹 시간·정지 지연은 검증하지 않았습니다.
- 재사용성을 위한 콜백·컨텍스트가 추가되어 optimized 단일 응용보다 코드와 정적 메모리가 늘었습니다.
- Cortex-M4 싱글코어 ISR/메인 모델을 기준으로 하며, 멀티코어·RTOS 다중 태스크 동시 접근을 그대로 지원하는 구조는 아닙니다.

## 문서

- [왜 이렇게 나누었는가](docs/DESIGN_KO.md)
- [API와 호출 규칙](docs/API_KO.md)
- [다른 보드로 옮기는 방법](docs/PORTING_KO.md)
- [테스트 결과와 보드 검증](docs/VALIDATION_KO.md)
