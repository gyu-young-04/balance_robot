#include <M5Unified.h>

int runMode =4; // 1: 기본 멀티태스킹, 2: 뮤텍스 실험 3: 철학자 문제 4: 화면출력

// 4.여러task 화면 동시출력

// 화면 출력을 보호하기 위한 뮤텍스 핸들
SemaphoreHandle_t xGuiMutex;

// 태스크 함수 선언
void TaskDisplay(void *pvParameters);

void Dis_setup() {
    // CoreS3 초기화
    auto cfg = M5.config();
    M5.begin(cfg);
    
    // 화면 설정
    M5.Display.setTextColor(WHITE);
    M5.Display.setTextSize(2);
    M5.Display.println("CoreS3 Mutex Display Test");
    M5.Display.println("-------------------------");

    // 1. 뮤텍스 생성
    xGuiMutex = xSemaphoreCreateMutex();

    if (xGuiMutex != NULL) {  // 생성 잘되면
        // 2. 3개의 서로 다른 태스크 생성 (출력 위치를 다르게 파라미터로 전달)
        // xTaskCreate(함수, 이름, 스택, 파라미터(y좌표), 우선순위, 핸들)
        xTaskCreate(TaskDisplay, "Task A", 4096, (void*)60,  1, NULL);
        xTaskCreate(TaskDisplay, "Task B", 4096, (void*)100, 1, NULL);
        xTaskCreate(TaskDisplay, "Task C", 4096, (void*)140, 1, NULL);
        // 서로 다른곳에 출력
    }
}

void Dis_loop() {
    // 메인 루프는 비워두거나 다른 작업 수행
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// 화면에 출력하는 공통 태스크 함수
void TaskDisplay(void *pvParameters) {
    int yPos = (int)pvParameters; // 파라미터로 받은 y좌표
    int counter = 0;
    char taskName[16];
    strncpy(taskName, pcTaskGetName(NULL), sizeof(taskName)-1); // 현재 태스크 이름 가져오기
    // TaskDisplay

    for (;;) {
        // [핵심] 뮤텍스를 획득하여 화면 제어권 독점
        if (xSemaphoreTake(xGuiMutex, portMAX_DELAY) == pdTRUE) {
            
            // 화면의 특정 위치를 지우고 새로 쓰기
            M5.Display.setCursor(10, yPos);
            M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
            // 배경색 지정하여 이전 잔상 지우기
            M5.Display.printf("%s Running: %d    ", taskName, counter++);
            
            // 출력 후 잠시 대기해도 다른 태스크가 화면을 건드리지 못함
            vTaskDelay(pdMS_TO_TICKS(100)); 

            // [핵심] 출력 완료 후 뮤텍스 반납
            xSemaphoreGive(xGuiMutex);
        }

        // 다음 출력을 위해 1초 대기 (이때는 뮤텍스가 없으므로 다른 태스크가 출력 가능)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}




// 3. 철학자 문제(deadlock)


SemaphoreHandle_t forks[5]; // 포크저장

void TaskPhilosopher(void *pvParameters) {
  int id = (int)pvParameters; // 포크 뮤텍스 인덱스 번호
  int leftFork = id;
  int rightFork = (id + 1) % 5;

  for (;;) {
    Serial.printf("[철학자 %d] 생각 중...\n", id);
    vTaskDelay(pdMS_TO_TICKS(1000)); // 생각하는 시간

    // 1. 왼쪽 포크 집기
    if (xSemaphoreTake(forks[leftFork], portMAX_DELAY) == pdTRUE) {
      Serial.printf("  -> [철학자 %d] 왼쪽 포크(%d) 획득\n", id, leftFork);
      
      // 데드락 상황을 확실히 유도하기 위해 잠시 대기 (모든 철학자가 왼쪽을 들 때까지)
      vTaskDelay(pdMS_TO_TICKS(100)); 

      // 2. 오른쪽 포크 집기 시도
      Serial.printf("  ... [철학자 %d] 오른쪽 포크(%d) 기다리는 중...\n", id, rightFork);
      if (xSemaphoreTake(forks[rightFork], portMAX_DELAY) == pdTRUE) {
        Serial.printf("***** [철학자 %d] 식사 중! *****\n", id);
        vTaskDelay(pdMS_TO_TICKS(1000));

        xSemaphoreGive(forks[rightFork]);
        xSemaphoreGive(forks[leftFork]);
      }
    }
  }
}

// TaskPhilosopher 함수 내부의 포크 집기 로직만 수정
void TaskPhilosopherFixed(void *pvParameters) {
  int id = (int)pvParameters;
  int leftFork = id;
  int rightFork = (id + 1) % 5;

  for (;;) {
    // 4번 철학자(마지막 사람)만 오른쪽 포크를 먼저 집게 함
    if (id == 4) {
        xSemaphoreTake(forks[rightFork], portMAX_DELAY);
        xSemaphoreTake(forks[leftFork], portMAX_DELAY);
    } else {
        xSemaphoreTake(forks[leftFork], portMAX_DELAY);
        xSemaphoreTake(forks[rightFork], portMAX_DELAY);
    }

    Serial.printf("***** [철학자 %d] 식사 중! *****\n", id);
    vTaskDelay(pdMS_TO_TICKS(1000));

    xSemaphoreGive(forks[leftFork]);
    xSemaphoreGive(forks[rightFork]);
    
    vTaskDelay(pdMS_TO_TICKS(10)); // 다른 철학자에게 기회 양보
  }
}

void Philosopher_setup() {
  
  // 5개의 포크(뮤텍스) 생성
  for (int i = 0; i < 5; i++) {
    forks[i] = xSemaphoreCreateMutex();
  }

  // 5명의 철학자 태스크 생성
  for (int i = 0; i < 5; i++) {
    xTaskCreate(TaskPhilosopherFixed, "Philo", 2048, (void *)i, 1, NULL);
  }
}

void Philosopher_loop() {
  // 루프는 비워둡니다.
}





// 2. 뮤텍스 사용으로 전역변수 변화 살펴보기

volatile int sharedValue = 0; // 공유 전역 변수
// 단순히 선언한 경우와 달리 언제든 바뀔수있기에 매번 메모리로 확인하게 하는 방식
SemaphoreHandle_t xMutex;      // 뮤텍스 핸들
// 뮤텍스나 세마포어의 상태를 저장한 구조체에 접근할 필요없이
// xMutex 에는 뮤텍스나 세마포어 상태를 저장한 주소를 저장

//우선순위가 같을 때 FreeRTOS는 생성된 순서대로 리스트에 등록하고 실행 기회를 준다.
// vTaskDelay 선언시 같은 우선순위 task 제일 뒤로감
// -3
// 1 1 1 1 1 1 1 1 1 1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1   
// 2 2 2 2 2 2 2 2 2 2 -2 -2 -2 -2 -2 -2 -2 -2 -2 -2
// 3 3 3 3 3 3 3 3 3 3 -3 -3 -3 -3 -3 -3 -3 -3 -3 -3   ...                             

void mutex_setup() {
  // 1. 뮤텍스 생성 (해결책을 위해 미리 선언만 함)
  xMutex = xSemaphoreCreateMutex();

  Serial.println("--- 실시간 시스템 경쟁 상태 시작 ---");

  // 더하기 태스크 10개 생성
  for (int i = 0; i < 10; i++) {
    xTaskCreate(TaskAdd, "Add", 2048, NULL, 1, NULL);
  }

  // 빼기 태스크 10개 생성
  for (int i = 0; i < 10; i++) {
    xTaskCreate(TaskSub, "Sub", 2048, NULL, 1, NULL);
  }
  // 우선순위가 모두 동일한 경우 round robin 사용
}

void mutex_loop() { // loop 도 우선순위가 1이기에 덧셈뺄셈 task와 같이 round robin
  // 현재 전역 변수 값 확인을 위해 1초마다 출력
  Serial.printf("현재 전역 변수 값: %d\n", sharedValue);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

// 1을 더하는 태스크
void TaskAdd(void *pvParameters) {
  for (int i = 0; i < 1000; i++) {
    /* [해결 방법] 아래 주석을 풀면 정상 동작합니다. */
     if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      // 획득 (열쇠ID,열쇠가 생길때까지 기다릴거임) == 열쇠를 얻으면 true 반환
      
      // 비정상 상황 유도를 위해 값을 읽고 연산하는 사이에 아주 짧은 지연을 줍니다.
      int temp = sharedValue;
      temp++;
      vTaskDelay(0); // 다른 태스크에게 CPU를 넘겨 경쟁 상태 극대화
      // blocked 되지는 않고 그냥 넘겨주기만 할때
      sharedValue = temp;

    xSemaphoreGive(xMutex); // 열쇠 반납
    }
  }
  vTaskDelete(NULL); // 작업 완료 후 태스크 삭제
}

// 1을 빼는 태스크
void TaskSub(void *pvParameters) {
  for (int i = 0; i < 1000; i++) {
    /* [해결 방법] 아래 주석을 풀면 정상 동작합니다. */
    if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
      
      int temp = sharedValue;
      temp--;
      vTaskDelay(0);
      sharedValue = temp;

    xSemaphoreGive(xMutex);
    }
  }
  vTaskDelete(NULL);
}


// 1. task 여러개 만들어서 점유시간 조절하기

// 3 개의 함수 선언
void Task1(void *pvParameters);
void Task2(void *pvParameters);
void Task3(void *pvParameters);
// task 를 만들때 어떤 파라미터를 사용자가 넘길지 모르기에
// 일단 주소값으로 받고 알아서 사용자가 형변환해서 사용하게하는것이다
// 일단 지금 멀티테스크에서는 매개변수를 지정하지는않았다.


void multi_setup() {

  // xTaskCreate(함수명, "태스크이름", 스택크기, 파라미터, 우선순위, 핸들)
  xTaskCreate(Task1, "Task 1", 2048, NULL, 1, NULL); 
  xTaskCreate(Task2, "Task 2", 2048, NULL, 2, NULL); 
  xTaskCreate(Task3, "Task 3", 2048, NULL, 3, NULL);
  // 숫자가 크면 먼저하는거임
  // 스택크기: 2048 * 4바이트 = 8192바이트
  // 핸들: 생성된 task를 나중에 조종할때 사용함
  Serial.println("--- FreeRTOS 스케줄러 시작 ---");
}

void multi_loop() {
  // 모드 1일 때는 특별히 할 일이 없으므로 5초씩 쉽니다.
  vTaskDelay(5000 / portTICK_PERIOD_MS);

}

void Task1(void *pvParameters) {
  const char* taskName = "Task 1";
  UBaseType_t priority = uxTaskPriorityGet(NULL); // 우선순위 불러오기

  while (1) {
    Serial.printf("[%s] Priority: %d - Running\n", taskName, (int)priority);
    
    // 테스트: delay(100)와 vTaskDelay 비교
    // delay(100); 
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    // 현재 task를 blocked 상태로 바꾸고 2초간 할일이 없으니 다른 task 시켜주라는뜻

    // delay(100)의 경우 함수내부에서 루프를 돌며 시간때우는 busy-waiting 임
  }
}

void Task2(void *pvParameters) {
  const char* taskName = "Task 2";
  UBaseType_t priority = uxTaskPriorityGet(NULL);
  while (1) {
    Serial.printf("[%s] Priority: %d - Running\n", taskName, (int)priority);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void Task3(void *pvParameters) {
  const char* taskName = "Task 3";
  UBaseType_t priority = uxTaskPriorityGet(NULL);
  while (1) {
    Serial.printf("[%s] Priority: %d - Running\n", taskName, (int)priority);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void setup(){
  Serial.begin(115200); // 컴퓨터와 아두이노 사이의 통신속도 설정
  while (!Serial);
  if (runMode == 1) {
    multi_setup();
  } 
  else if (runMode == 2) {
    mutex_setup();
  }
  else if (runMode == 3) {
    Philosopher_setup();
  }
  else if (runMode == 4) {
    Dis_setup();
  }
}

void loop() {
  if (runMode == 1) {
    multi_loop();
  } 
  else if (runMode == 2) {
    mutex_loop();
  }
  else if (runMode == 3) {
    Philosopher_loop();
  }
  else if (runMode == 4) {
    Dis_loop();
  }
}
