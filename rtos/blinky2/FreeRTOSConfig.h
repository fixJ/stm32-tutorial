#define    configMAX_PRIORITIES    5
#define    configMINIMAL_STACK_SIZE    (unsigned short) 128
#define    configCHECK_FOR_STACK_OVERFLOW     1
#define    configUSE_16_BIT_TICKS    0
#define    configMAX_SYSCALL_INTERRUPT_PRIORITY    191
#define    configUSE_PREEMPTION    1
#define    configUSE_IDLE_HOOK    0
#define    configUSE_TICK_HOOK    0
#define    configTICK_RATE_HZ    ( TickType_t ) 250
#define configCPU_CLOCK_HZ              ( ( unsigned long ) 72000000 )
#define configSYSTICK_CLOCK_HZ          ( configCPU_CLOCK_HZ / 8 ) /* vTaskDelay() fix */
#define INCLUDE_vTaskDelay      1
#define configTOTAL_HEAP_SIZE		( ( size_t ) ( 17 * 1024 ) )
#define configIDLE_SHOULD_YIELD 1