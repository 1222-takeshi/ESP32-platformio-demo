#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Arduino.h"
#include "WiFi.h"
#include "esp_freertos_hooks.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#define UDP_PORT 9876

const char * WIFI_SSID = "TP_Link_Living";
const char * WIFI_PASS = "20210722";

long total_idle_count;
long core0_idle_count;
long core1_idle_count;
long total_tick_count;
long core0_tick_count;
long core1_tick_count;

typedef struct
{
  long total_idle_count;
  long core0_idle_count;
  long core1_idle_count;
  long total_tick_count;
  long core0_tick_count;
  long core1_tick_count;
} COUNTER_t;

QueueHandle_t xQueue;
SemaphoreHandle_t xSemaphore0;
SemaphoreHandle_t xSemaphore1;
SemaphoreHandle_t xSemaphore2;

// https://qiita.com/koara-local/items/585755faac70c8b37b5b
// [C++11 ~] variadic templates
template<typename ... Args>
void _printf(const char * format, Args const & ... args)
{
//    xSemaphoreTake(xSemaphore0, portMAX_DELAY);
  TickType_t _nowTick = xTaskGetTickCount();
  char * _taskName = pcTaskGetTaskName(NULL);
  printf("[%s:%d] ", _taskName, _nowTick);
  printf(format, args ...);
//    xSemaphoreGive(xSemaphore0);
}

//時間稼ぎ
void ConsumptionTick(int delay)
{
  TickType_t startTick;
  TickType_t endTick;
  TickType_t nowTick;
  startTick = xTaskGetTickCount();
  endTick = startTick + delay;
  while (1) {
    nowTick = xTaskGetTickCount();
    if (nowTick > endTick) {break;}
  }
}

void showNetworkInfo()
{
  IPAddress ip = WiFi.localIP();
  IPAddress mk = WiFi.subnetMask();
  IPAddress gw = WiFi.gatewayIP();
  _printf("IP address=%d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
  _printf("Netmask   =%d.%d.%d.%d\n", mk[0], mk[1], mk[2], mk[3]);
  _printf("Gateway   =%d.%d.%d.%d\n", gw[0], gw[1], gw[2], gw[3]);
}


bool ApplicationIdleHook(void)
{
  total_idle_count++;
  return true;
}

bool Core0IdleHook(void)
{
  core0_idle_count++;
  return true;
}

bool Core1IdleHook(void)
{
  core1_idle_count++;
  return true;
}

void ApplicationTickHook(void)
{
  total_tick_count++;
}

void Core0TickHook(void)
{
  core0_tick_count++;
}

void Core1TickHook(void)
{
  core1_tick_count++;
}


void clearHook()
{
  total_idle_count = 0;
  core0_idle_count = 0;
  core1_idle_count = 0;
  total_tick_count = 0;
  core0_tick_count = 0;
  core1_tick_count = 0;
}

void printHook(char * title)
{
  TickType_t nowTick;
  nowTick = xTaskGetTickCount();
  printf("[%d] %s\n", nowTick, title);
  printf(
    "[%d] total_idle_count=%ld total_tick_count=%ld\n", nowTick, total_idle_count,
    total_tick_count);
  float idleRate;
  idleRate = float(core0_idle_count) / float(core0_tick_count) * 100.0;
  printf(
    "[%d] core0_idle_count=%ld core0_tick_count=%ld idleRate=%.2f\n", nowTick,
    core0_idle_count, core0_tick_count, idleRate);
  idleRate = float(core1_idle_count) / float(core1_tick_count) * 100.0;
  printf(
    "[%d] core1_idle_count=%ld core1_tick_count=%ld idleRate=%.2f\n", nowTick,
    core1_idle_count, core1_tick_count, idleRate);
}


#if 1
// Monitor Task
static void monitorTask(void * pvParameters)
{
  COUNTER_t mess;
  _printf("start Priority=%d CoreID=%d\n", uxTaskPriorityGet(NULL), xPortGetCoreID());

  /* set up address to connect to */
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(UDP_PORT);
  addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);   /* send message to 255.255.255.255 */

  /* create the socket */
  int fd;
  int ret;
  fd = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    // Create a UDP socket.
  LWIP_ASSERT("fd >= 0", fd >= 0);

  float idleRate;
  char buffer[128];
  int buflen;

  while (1) {
    xQueueReceive(xQueue, &mess, portMAX_DELAY);
    sprintf(buffer, "Free memory: %d bytes\n", esp_get_free_heap_size());
    buflen = strlen(buffer);
    ret = lwip_sendto(fd, buffer, buflen, 0, (struct sockaddr *)&addr, sizeof(addr));
    LWIP_ASSERT("ret == buflen", ret == buflen);
    _printf("lwip_sendto ret=%d\n", ret);

    idleRate = (float)mess.total_idle_count / (float)mess.total_tick_count * 100.0;
    if (idleRate > 100.0) {idleRate = 100.0;}
    sprintf(buffer, "CPU Idle: %.2f %%\n", idleRate);
    buflen = strlen(buffer);
    ret = lwip_sendto(fd, buffer, buflen, 0, (struct sockaddr *)&addr, sizeof(addr));
    LWIP_ASSERT("ret == buflen", ret == buflen);

    idleRate = (float)mess.core0_idle_count / (float)mess.core0_tick_count * 100.0;
    if (idleRate > 100.0) {idleRate = 100.0;}
    sprintf(buffer, "CORE0 Idle: %.2f %%\n", idleRate);
    buflen = strlen(buffer);
    ret = lwip_sendto(fd, buffer, buflen, 0, (struct sockaddr *)&addr, sizeof(addr));
    LWIP_ASSERT("ret == buflen", ret == buflen);

    idleRate = (float)mess.core1_idle_count / (float)mess.core1_tick_count * 100.0;
    if (idleRate > 100.0) {idleRate = 100.0;}
    sprintf(buffer, "CORE1 Idle: %.2f %%\n\n", idleRate);
    buflen = strlen(buffer);
    ret = lwip_sendto(fd, buffer, buflen, 0, (struct sockaddr *)&addr, sizeof(addr));
    LWIP_ASSERT("ret == buflen", ret == buflen);

  }   // end while

  vTaskDelete(NULL);
}
#endif


#if 0
// Monitor Task
static void monitorTask(void * pvParameters)
{
  COUNTER_t mess;
  _printf("start Priority=%d CoreID=%d\n", uxTaskPriorityGet(NULL), xPortGetCoreID());

  float idleRate;
  while (1) {
    xQueueReceive(xQueue, &mess, portMAX_DELAY);
    _printf("Free memory: %d bytes\n", esp_get_free_heap_size());

    idleRate = (float)mess.total_idle_count / (float)mess.total_tick_count * 100.0;
    if (idleRate > 100.0) {idleRate = 100.0;}
    _printf("CPU Idle: %.2f %%\n", idleRate);

    idleRate = (float)mess.core0_idle_count / (float)mess.core0_tick_count * 100.0;
    if (idleRate > 100.0) {idleRate = 100.0;}
    _printf("CORE0 Idle: %.2f %%\n", idleRate);

    idleRate = (float)mess.core1_idle_count / (float)mess.core1_tick_count * 100.0;
    if (idleRate > 100.0) {idleRate = 100.0;}
    _printf("CORE1 Idle: %.2f %%\n\n", idleRate);
  }   // end while

  vTaskDelete(NULL);
}
#endif

// Performance Task
void perfTask(void * pvParameters)
{
  COUNTER_t mess;
  _printf("start Priority=%d CoreID=%d\n", uxTaskPriorityGet(NULL), xPortGetCoreID());

  while (1) {
    clearHook();
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    //printHook("perf");
    mess.total_idle_count = total_idle_count;
    mess.core0_idle_count = core0_idle_count;
    mess.core1_idle_count = core1_idle_count;
    mess.total_tick_count = total_tick_count;
    mess.core0_tick_count = core0_tick_count;
    mess.core1_tick_count = core1_tick_count;
    xQueueOverwrite(xQueue, &mess);
  }
}

// Dummy Task
// void dummyTask(void * pvParameters)
// {
//   SemaphoreHandle_t * _Semaphore = (SemaphoreHandle_t *)pvParameters;
//   _printf("start Priority=%d CoreID=%d\n", uxTaskPriorityGet(NULL), xPortGetCoreID());

//   while (1) {
//     xSemaphoreTake(*_Semaphore, portMAX_DELAY);
//     _printf("Take Semaphore\n");
//     for (int i = 0; i < 100; i++) {
//       ConsumptionTick(99);
//       vTaskDelay(1);
//     }
//   }
// }

void setup()
{
  delay(2000);
  _printf("start Priority=%d CoreID=%d\n", uxTaskPriorityGet(NULL), xPortGetCoreID());

  /* Connect WiFi */
  _printf("Connecting to %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    _printf("Wait for connect....\n");
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

  _printf("WiFi connected\n");
  showNetworkInfo();

  /* Create queue */
  xQueue = xQueueCreate(1, sizeof(COUNTER_t) );
  configASSERT(xQueue);

  /* Create Semaphore */
  // xSemaphore0 = xSemaphoreCreateBinary();
  // xSemaphore1 = xSemaphoreCreateBinary();
  // xSemaphore2 = xSemaphoreCreateBinary();
  // configASSERT(xSemaphore0);
  // configASSERT(xSemaphore1);
  // configASSERT(xSemaphore2);

  esp_register_freertos_idle_hook(&ApplicationIdleHook);
  esp_register_freertos_tick_hook(&ApplicationTickHook);
  esp_register_freertos_idle_hook_for_cpu(&Core0IdleHook, 0);
  esp_register_freertos_idle_hook_for_cpu(&Core1IdleHook, 1);
  esp_register_freertos_tick_hook_for_cpu(&Core0TickHook, 0);
  esp_register_freertos_tick_hook_for_cpu(&Core1TickHook, 1);

  xTaskCreate(monitorTask, "monitor", 4096, NULL, 2, NULL);
  xTaskCreate(perfTask, "perf", 4096, NULL, 2, NULL);

  // xTaskCreatePinnedToCore(dummyTask, "dummy0", 4096, &xSemaphore1, 1, NULL, 0);
  // xTaskCreatePinnedToCore(dummyTask, "dummy1", 4096, &xSemaphore2, 1, NULL, 1);

  for (int i = 0; i < 100; i++) {
    // if ( (i % 4) == 0) {xSemaphoreGive(xSemaphore1);}
    // if ( (i % 4) == 1) {xSemaphoreGive(xSemaphore2);}
    // if ( (i % 4) == 2) {
    //   xSemaphoreGive(xSemaphore1);
    //   xSemaphoreGive(xSemaphore2);
    // }
    vTaskDelay(10000 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void loop()   // Never run
{
  _printf("loop\n");
}
