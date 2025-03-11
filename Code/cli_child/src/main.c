/**
 * @file cli_leader.c
 * @brief Command Line Interface (CLI) application for Zephyr OS.
 * @author Harry Koutsourelakis
 * @email harrkout@gmail.com
 * @company Intelligent Systems and Computer Architecture (ISCA) Lab
 * @date March 2025
 * @version 1.0
 */
 
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#if defined(CONFIG_CLI_SAMPLE_MULTIPROTOCOL)
#include "ble.h"
#endif

#if defined(CONFIG_CLI_SAMPLE_LOW_POWER)
#include "low_power.h"
#endif

LOG_MODULE_REGISTER(cli_sample, CONFIG_OT_COMMAND_LINE_INTERFACE_LOG_LEVEL);

volatile bool float_computation_enabled = false;
volatile bool int_computation_enabled = false;
volatile bool fibonacci_calculation_enabled = false;

#define STACK_SIZE_FLOAT_COMP 1024
#define STACK_SIZE_INT_COMP 3072
#define STACK_SIZE_FIB_CALC 1024
#define STACK_SIZE_UDP_SEND 1024
#define THREAD_PRIORITY 7

static K_THREAD_STACK_DEFINE(float_comp_stack, STACK_SIZE_FLOAT_COMP);
static K_THREAD_STACK_DEFINE(int_comp_stack, STACK_SIZE_INT_COMP);
static K_THREAD_STACK_DEFINE(fib_calc_stack, STACK_SIZE_FIB_CALC);
static K_THREAD_STACK_DEFINE(udp_send_stack, STACK_SIZE_UDP_SEND);

static struct k_thread float_comp_thread, int_comp_thread, fib_calc_thread, udp_send_thread;

static void float_computation_thread(void *p1, void *p2, void *p3)
{
    volatile float counter = 0.0f;
    while (1) {
        if (float_computation_enabled) {
            LOG_INF("Starting floating-point computation...");
            float total = 0.0f;
            for (int i = 0; i < 50000; i++) {
                counter += (float)i * 2.0f;
                total = counter;
                if (i % 10000 == 0) {
                    LOG_INF("Floating-point computation progress: %d/50000", i);
                    k_yield();
                }
            }
            LOG_INF("Floating-point computation completed. Total accumulated value: %f", total);
            counter = 0.0f;
            k_msleep(1000);
        } else {
            k_msleep(100);
        }
    }
}

static void int_computation_thread(void *p1, void *p2, void *p3)
{
    volatile uint32_t counter = 0;
    while (1) {
        if (int_computation_enabled) {
            LOG_INF("Starting integer computation...");
            uint32_t total = 0;
            for (int i = 0; i < 50000; i++) {
                counter += i * 2;
                total = counter;
                if (i % 10000 == 0) {
                    LOG_INF("Integer computation progress: %d/50000", i);
                    k_yield();
                }
            }
            LOG_INF("Integer computation completed. Total accumulated value: %u", total);
            counter = 0;
            k_msleep(1000);
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
        if (i % 100 == 0) {
            LOG_INF("Calculating Fibonacci(%d)...", i);
            k_yield();
        }
    }
    return b;
}

static void fibonacci_calculation_thread(void *p1, void *p2, void *p3)
{
    while (1) {
        if (fibonacci_calculation_enabled) {
            LOG_INF("Starting Fibonacci(500) calculation...");
            int result = fibonacci(500);
            LOG_INF("Fibonacci(500) result: %d", result);
            k_msleep(1000);
        } else {
            k_msleep(100);
        }
    }
}

static void handle_udp_command(const char *command)
{
    if (strncmp(command, "enable_float", 12) == 0) {
        float_computation_enabled = true;
        LOG_INF("Floating-point computation enabled via UDP");
    }
    else if (strncmp(command, "disable_float", 13) == 0) {
        float_computation_enabled = false;
        LOG_INF("Floating-point computation disabled via UDP");
    }
    else if (strncmp(command, "enable_int", 10) == 0) {
        int_computation_enabled = true;
        LOG_INF("Integer computation enabled via UDP");
    }
    else if (strncmp(command, "disable_int", 11) == 0) {
        int_computation_enabled = false;
        LOG_INF("Integer computation disabled via UDP");
    }
    else if (strncmp(command, "enable_fibonacci", 16) == 0) {
        fibonacci_calculation_enabled = true;
        LOG_INF("Fibonacci calculation enabled via UDP");
    }
    else if (strncmp(command, "disable_fibonacci", 17) == 0) {
        fibonacci_calculation_enabled = false;
        LOG_INF("Fibonacci calculation disabled via UDP");
    }
    else {
        LOG_WRN("Unknown UDP command: %s", command);
    }
}

static void enable_float_computation(const struct shell *shell, size_t argc, char **argv) {
    float_computation_enabled = true;
    shell_print(shell, "Floating-point computation enabled");
}

static void disable_float_computation(const struct shell *shell, size_t argc, char **argv) {
    float_computation_enabled = false;
    shell_print(shell, "Floating-point computation disabled");
}

static void enable_int_computation(const struct shell *shell, size_t argc, char **argv) {
    int_computation_enabled = true;
    shell_print(shell, "Integer computation enabled");
}

static void disable_int_computation(const struct shell *shell, size_t argc, char **argv) {
    int_computation_enabled = false;
    shell_print(shell, "Integer computation disabled");
}

static void enable_fibonacci_calculation(const struct shell *shell, size_t argc, char **argv) {
    fibonacci_calculation_enabled = true;
    shell_print(shell, "Fibonacci calculation enabled");
}

static void disable_fibonacci_calculation(const struct shell *shell, size_t argc, char **argv) {
    fibonacci_calculation_enabled = false;
    shell_print(shell, "Fibonacci calculation disabled");
}

static int cmd_status(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "Current Status:");
    shell_print(shell, "- Floating-point Computation: %s", 
               float_computation_enabled ? "ACTIVE" : "INACTIVE");
    shell_print(shell, "- Integer Computation: %s", 
               int_computation_enabled ? "ACTIVE" : "INACTIVE");
    shell_print(shell, "- Fibonacci Calculation: %s", 
               fibonacci_calculation_enabled ? "ACTIVE" : "INACTIVE");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_function,
    SHELL_CMD(enable_float, NULL, "Enable floating-point computation", enable_float_computation),
    SHELL_CMD(disable_float, NULL, "Disable floating-point computation", disable_float_computation),
    SHELL_CMD(enable_int, NULL, "Enable integer computation", enable_int_computation),
    SHELL_CMD(disable_int, NULL, "Disable integer computation", disable_int_computation),
    SHELL_CMD(enable_fibonacci, NULL, "Enable Fibonacci calculation", enable_fibonacci_calculation),
    SHELL_CMD(disable_fibonacci, NULL, "Disable Fibonacci calculation", disable_fibonacci_calculation),
    SHELL_CMD(status, NULL, "Show current command status", cmd_status),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(commands, &sub_function, "CPU workload management commands", NULL);


void send_ot_commands(const struct shell *shell) {
    const char *commands[] = {
        "ot dataset channel 15",
        "ot dataset panid 0x1234",
        "ot dataset networkname \"OpenThreadDemo\"",
        "ot dataset extpanid 1111111122222222",
        "ot dataset networkkey 00112233445566778899aabbccddeeff",
        "ot dataset commit active",
        "ot ifconfig up",
        "ot thread start",
        "ot state",
        "ot udp open",
        "ot udp bind :: 1234"
    };

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        shell_fprintf(shell, SHELL_NORMAL, "Executing: %s\n", commands[i]);
        shell_execute_cmd(shell, commands[i]);
        k_sleep(K_SECONDS(1));
    }

    LOG_INF("OpenThread stack initialized and ready.");
}

void main(void) {
    const struct shell *shell = shell_backend_uart_get_ptr();
    srand(time(NULL));
    send_ot_commands(shell);

    k_thread_create(&float_comp_thread, float_comp_stack, 
                    K_THREAD_STACK_SIZEOF(float_comp_stack),
                    float_computation_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&int_comp_thread, int_comp_stack, 
                    K_THREAD_STACK_SIZEOF(int_comp_stack),
                    int_computation_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_create(&fib_calc_thread, fib_calc_stack, 
                    K_THREAD_STACK_SIZEOF(fib_calc_stack),
                    fibonacci_calculation_thread, NULL, NULL, NULL,
                    THREAD_PRIORITY, 0, K_NO_WAIT);

    while (1) {
        k_msleep(1000);
    }
}