#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>

#if defined(CONFIG_CLI_SAMPLE_MULTIPROTOCOL)
#include "ble.h"
#endif

#if defined(CONFIG_CLI_SAMPLE_LOW_POWER)
#include "low_power.h"
#endif

LOG_MODULE_REGISTER(cli_sample, CONFIG_OT_COMMAND_LINE_INTERFACE_LOG_LEVEL);

// Function control flags
volatile bool func_1_enabled = false;  // Add volatile for flag access
volatile bool func_2_enabled = false;
volatile bool func_3_enabled = false;

// Adjusted thread configurations
#define STACK_SIZE_FUNC1 1024
#define STACK_SIZE_FUNC2 3072  // Larger stack for sorting
#define STACK_SIZE_FUNC3 1024
#define THREAD_PRIORITY 7      // Lower priority than shell (which typically uses 4)

static K_THREAD_STACK_DEFINE(stack1, STACK_SIZE_FUNC1);
static K_THREAD_STACK_DEFINE(stack2, STACK_SIZE_FUNC2);
static K_THREAD_STACK_DEFINE(stack3, STACK_SIZE_FUNC3);

static struct k_thread thread1, thread2, thread3;

/* CPU-intensive functions with yields */
static void function_1_thread(void *p1, void *p2, void *p3)
{
    volatile uint32_t counter = 0;
    while (1) {
        if (func_1_enabled) {
            for (int i = 0; i < 50000; i++) {  // Reduced iterations
                counter += i * 2;
                if (i % 1000 == 0) k_yield();  // Yield periodically
            }
            counter = 0;
            k_msleep(10);
        } else {
            k_msleep(100);
        }
    }
}

static void function_2_thread(void *p1, void *p2, void *p3)
{
    int data[200];  // Reduced array size
    while (1) {
        if (func_2_enabled) {
            // Generate random data
            for (int i = 0; i < ARRAY_SIZE(data); i++) {
                data[i] = k_cycle_get_32() % 1000;
            }
            
            // Bubble sort with yield
            for (int i = 0; i < ARRAY_SIZE(data)-1; i++) {
                for (int j = 0; j < ARRAY_SIZE(data)-i-1; j++) {
                    if (data[j] > data[j+1]) {
                        int temp = data[j];
                        data[j] = data[j+1];
                        data[j+1] = temp;
                    }
                }
                k_yield();  // Yield after each outer loop iteration
            }
            k_msleep(10);
        } else {
            k_msleep(100);
        }
    }
}

static int fibonacci(int n) {
    int a = 0, b = 1, c;
    if (n == 0) return a;
    for (int i = 2; i <= n; i++) {
        c = a + b;
        a = b;
        b = c;
        if (i % 100 == 0) k_yield();  // Yield periodically
    }
    return b;
}

static void function_3_thread(void *p1, void *p2, void *p3)
{
    while (1) {
        if (func_3_enabled) {
            volatile int result = fibonacci(500);  // Reduced from 1000
            (void)result;
            k_msleep(10);
        } else {
            k_msleep(100);
        }
    }
}

/* Shell command handlers */
static void enable_function_1(const struct shell *shell, size_t argc, char **argv) {
    func_1_enabled = true;
    shell_print(shell, "Function 1 (Sensor Simulation) Enabled");
}

static void disable_function_1(const struct shell *shell, size_t argc, char **argv) {
    func_1_enabled = false;
    shell_print(shell, "Function 1 (Sensor Simulation) Disabled");
}

static void enable_function_2(const struct shell *shell, size_t argc, char **argv) {
    func_2_enabled = true;
    shell_print(shell, "Function 2 (Data Processing) Enabled");
}

static void disable_function_2(const struct shell *shell, size_t argc, char **argv) {
    func_2_enabled = false;
    shell_print(shell, "Function 2 (Data Processing) Disabled");
}

static void enable_function_3(const struct shell *shell, size_t argc, char **argv) {
    func_3_enabled = true;
    shell_print(shell, "Function 3 (CPU Usage Report) Enabled");
}

static void disable_function_3(const struct shell *shell, size_t argc, char **argv) {
    func_3_enabled = false;
    shell_print(shell, "Function 3 (CPU Usage Report) Disabled");
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_function,
    SHELL_CMD(enable_func1, NULL, "Enable Function 1 (Sensor Simulation)", enable_function_1),
    SHELL_CMD(disable_func1, NULL, "Disable Function 1 (Sensor Simulation)", disable_function_1),
    SHELL_CMD(enable_func2, NULL, "Enable Function 2 (Data Processing)", enable_function_2),
    SHELL_CMD(disable_func2, NULL, "Disable Function 2 (Data Processing)", disable_function_2),
    SHELL_CMD(enable_func3, NULL, "Enable Function 3 (CPU Usage Report)", enable_function_3),
    SHELL_CMD(disable_func3, NULL, "Disable Function 3 (CPU Usage Report)", disable_function_3),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(functions, &sub_function, "Functions submenu", NULL);

int main(void) {
    /* ... (keep existing main() initialization code unchanged) ... */

    // Create threads with adjusted priorities
    k_thread_create(&thread1, stack1, K_THREAD_STACK_SIZEOF(stack1),
                    function_1_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&thread2, stack2, K_THREAD_STACK_SIZEOF(stack2),
                    function_2_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&thread3, stack3, K_THREAD_STACK_SIZEOF(stack3),
                    function_3_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    while (1) {
        k_msleep(1000);
    }

    return 0;
}