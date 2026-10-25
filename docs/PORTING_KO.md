# 다른 보드로 옮기는 방법

## 필요한 작업

framework/include와 framework/src는 그대로 두고 새 ports/<board>를 만듭니다.
STM32 파일을 다른 MCU에서도 함께 빌드하지 않습니다.

다음 MF_Port 콜백을 구현합니다.

| 콜백 | 구현 계약 |
| --- | --- |
| enter_critical(user) | 입력 생산자 IRQ를 막고 이전 보호 상태 반환 |
| exit_critical(user, saved) | 저장한 상태로 복원 |
| now_ms(user) | uint32_t 밀리초 단조 증가 시계 |
| button_pressed(user) | 눌림이면 1, 해제면 0 |
| apply_output(user, direction, percent) | 지정한 방향/PWM 적용, NONE이면 출력 정지 |

버튼이 없는 보드는 button_pressed에서 항상 0을 반환할 수 있습니다.
함수 포인터 자체를 NULL로 두는 것은 허용하지 않습니다.
시계는 프레임워크가 소유하지 않으므로 SysTick, 다른 주기 타이머, 플랫폼 시계 중 선택할 수 있습니다.

## 보드 초기화

1. 클록·핀·타이머를 설정하고 모터 출력은 정지로 둡니다.
2. 버튼 원시 입력, 시계, UART를 준비합니다.
3. MF_Port에 보드 함수와 user 컨텍스트를 연결합니다.
4. MF_Init 성공을 확인합니다.
5. 입력 ISR을 인스턴스에 연결하고 활성화합니다.
6. 메인에서 MF_Process를 반복 실행합니다.

출력 핀과 UART는 보드 포트가 담당하므로 코어는 핀 번호를 모릅니다.
다른 드라이버의 EN/DIR/PWM 입력 방식도 apply_output에서 변환할 수 있습니다.
전압과 실제 정지 논리는 해당 드라이버에 맞게 검증해야 합니다.

## ISR 연결

UART 수신 ISR은 보드별 수신 레지스터를 한 번 읽고 MF_OnByteISR로 넘깁니다.
오버런 등 수신 오류는 MF_OnRxErrorISR로 넘깁니다.
UART 상태/오류 플래그를 해제하는 방법은 해당 MCU 포트에서 구현합니다.

STM32F411 예제는 SR을 읽은 뒤 DR을 읽는 경로를 사용합니다.
다른 MCU에도 이 순서를 그대로 복사하면 안 됩니다.

## 여러 인스턴스

MF_Instance를 별도로 만들고 각각 다른 MF_Port.user를 제공할 수 있습니다.
PC 테스트에는 두 인스턴스를 다른 가상 출력에 연결한 실제 예제가 있습니다.
tests/test_framework.c의 test_instances_and_critical_restore를 참고하세요.

제공 STM32 포트의 Board_MotorPort는 물리 출력 하나와 버튼 하나에 고정되어 있습니다.
단순히 인스턴스 두 개를 이 포트에 연결하면 같은 핀을 서로 덮어씁니다.
실제 다중 모터는 출력 채널과 입력 분배, 버튼 지정, 콜백 user 구조를 새로 작성해야 합니다.

## 빌드

코어 파일 네 개를 타깃 ABI로 컴파일해 정적 라이브러리로 묶고,
응용과 포트 오브젝트 뒤에 라이브러리를 링크합니다.
공개 헤더 검색 경로에 framework/include를 추가합니다.

기본 .a는 Cortex-M4 / Thumb / fpv4-sp-d16 / hard-float입니다.
다른 CPU·컴파일러·ABI에는 다시 빌드합니다.
링커 스크립트와 시작 코드도 새 보드에 맞게 준비합니다.

프레임워크 자체는 힙·printf·OS API를 요구하지 않습니다.

## 포팅 테스트

- PC의 fake port 테스트는 하드웨어 없이 실제 MF API를 실행하는 참고 구현입니다.
- 새 포트의 정지 출력과 방향 극성부터 모터 전원 없이 확인합니다.
- 시계 1ms, PWM, 입력 오류, 큐 포화와 정지 응답을 계측합니다.
- critical section의 최악 실행 시간이 UART 수신과 시계 정확도에 영향을 주지 않는지 확인합니다.
- 현재 예제는 bare-metal 싱글코어 구조이며 RTOS 포트는 별도 설계가 필요합니다.
