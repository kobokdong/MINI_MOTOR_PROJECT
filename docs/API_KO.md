# 공개 API 사용 규칙

## 초기화

MF_DefaultConfig()로 설정을 만든 뒤 보드 포트를 준비하고 MF_Init()을 호출합니다.
MF_Init 성공 전에 UART 생산자 인터럽트를 활성화하지 않습니다.
인스턴스는 static 또는 동등하게 충분히 오래 살아 있는 메모리에 할당합니다.
malloc은 사용하지 않습니다.

MF_Init의 반환값:

- 1: 성공, 정지 출력 콜백 적용 완료
- 0: 인스턴스·설정·콜백이 유효하지 않음

초기화 실패 시 코어가 보드 출력을 보장할 수 없으므로
보드 초기화 단계에서 출력을 정지로 만들어 두어야 합니다.
제공 예제는 Board_Init이 먼저 PA0/PA1을 LOW로 만듭니다.

이미 실행 중인 인스턴스를 MF_Init으로 덮어쓰는 것은 지원하지 않습니다.
재설정하려면 정지 완료를 확인하고 해당 인스턴스의 수신 IRQ 연결을 해제한 뒤 초기화하세요.
헤더에 공개된 인스턴스 필드는 정적 할당을 위한 내부 저장공간이며 직접 변경하는 설정 API가 아닙니다.

## 설정 검증

- 0 < reverse_delay_ms <= 0x7fffffff
- 0 < debounce_ms <= short_press_ms
- short_press_ms < long_release_ms <= hold_stop_ms <= 0x7fffffff
- default_level: 1~9
- MF_Port의 다섯 함수 포인터가 모두 있어야 함
- user는 NULL이어도 됨

값은 MF_Init 때 복사되므로 이후 원래 config 변수를 바꾸어도 인스턴스에는 적용되지 않습니다.
시간 단위는 밀리초이며 now_ms는 uint32_t 단조 증가 카운터를 제공합니다.
카운터 순환은 unsigned 차이로 처리하며, 대기 구간을 카운터 한 바퀴 이상 방치하지 않습니다.

## 메인에서 호출하는 API

### MF_Process(instance)

자주 반복 호출합니다. 최소한 선택한 디바운싱 시간보다 충분히 짧은 간격을 권합니다.
제공된 예제는 blocking 작업 없이 계속 호출합니다.
한 번 호출할 때 정지 이벤트를 먼저 처리하거나 일반 UART 명령을 최대 한 개 소비합니다.

버튼의 긴 누름과 UART 정지 중 하나라도 있으면 정지가 우선입니다.
일반 버튼 이벤트와 UART 명령이 같은 반복에 있으면 UART를 나중에 적용합니다.
정지 후 버튼 해제를 확인하기 전까지 이전 버튼 이벤트는 억제됩니다.

### MF_Command(instance, command, level)

| command | level | 의미 |
| --- | --- | --- |
| MF_COMMAND_CW | 사용하지 않음, 0 권장 | CW 요청 |
| MF_COMMAND_CCW | 사용하지 않음, 0 권장 | CCW 요청 |
| MF_COMMAND_STOP | 사용하지 않음, 0 권장 | 정지 요청 |
| MF_COMMAND_LEVEL | 1~9 | 듀티 단계 요청 |

1은 접수를 의미하며 모터에 이미 적용됐다는 뜻이 아닙니다.
이후 정지가 들어오면 접수된 일반 명령도 폐기됩니다.

0은 초기화되지 않음·잘못된 명령/단계·정지 처리 대기·큐 포화를 의미합니다.
큐 포화는 정지 래치와 overflow 카운터를 설정합니다.

### MF_Stop(instance)

MF_Command(instance, MF_COMMAND_STOP, 0)의 편의 함수입니다.
반환값은 없으며 정지 요청을 래치합니다.
실제 정지는 다음 Process에서 적용됩니다.
Process가 실행되지 않으면 정지도 적용되지 않으므로 블로킹 응용 루프를 만들지 마세요.

### MF_GetStatus(instance, status)

성공하면 1, 유효하지 않은 인스턴스·status이면 0을 반환합니다.
다음 정보를 복사합니다.

- state: MF_STATE_STOPPED / MF_STATE_CW / MF_STATE_CCW / MF_STATE_REVERSING
- output: MF_DIR_NONE / MF_DIR_CW / MF_DIR_CCW
- target: 요청 목표 방향
- level, duty_percent: 단계와 계산상 PWM 퍼센트
- rx_errors, rx_overflows: 누적 입력 오류·큐 포화 횟수
- stop_pending: 정지가 아직 처리되지 않았는지

정지 중 duty_percent는 다음 구동에 사용할 설정값이지 실제 PWM이 출력된다는 뜻이 아닙니다.

## ISR에서 호출하는 API

### MF_OnByteISR(instance, byte)

F/f, R/r, S/s, 0, 1~9를 처리합니다.
정지 문자는 일반 큐와 분리된 우선 래치에 저장합니다.
다른 문자는 무시합니다. 큐 저장 용량은 31개입니다.

### MF_OnRxErrorISR(instance)

수신 오류 카운터를 증가시키고 정지를 요청합니다.
보드 포트가 UART 오류 플래그를 해제한 뒤 또는 해제하는 과정에서 호출합니다.

하나의 인스턴스에는 직렬화된 ISR 생산자 하나를 전제로 합니다.
여러 ISR이 서로 선점하면서 같은 큐에 쓰거나,
여러 CPU 코어가 동시에 접근하는 사용은 지원하지 않습니다.
UART 외의 입력을 연결할 때도 이 규칙을 지켜야 합니다.

## 콜백 규칙

모든 콜백은 재진입하지 않고 짧게 끝나야 합니다.
특히 apply_output에서 MF_GetStatus, MF_Stop, MF_Process 등을 다시 호출하지 않습니다.
임계 구역 안의 콜백에서 잠금·대기 API를 호출하면 교착 또는 지연이 생길 수 있습니다.

enter_critical은 기존 마스킹 상태를 반환하고,
exit_critical은 무조건 IRQ를 켜는 대신 해당 상태로 복원해야 합니다.
이 프레임워크의 태스크/IRQ 모델과 맞지 않는 단순 뮤텍스로 대체하지 마세요.
