/**
  ******************************************************************************
  * @file           : state_machine.c
  * @brief          : State machine for handling commands
  * @details        : Processes commands to update signal parameters and states
  * @date           : 2021
  * @date           : Nicholas Antoniades
  ******************************************************************************
  */

#include "state_machine.h"
#include "stm32_bsp.h"
#include <stdlib.h>


static void handle_command_1(uint8_t rx_buff[], signal_config* config);
static void handle_command_2(uint8_t rx_buff[], signal_config* config);
static void handle_command_3(uint8_t rx_buff[], signal_config* config);
static void handle_command_4(uint8_t rx_buff[], signal_config* config);

void handle_state(uint8_t command, uint8_t rx_buff[], signal_config* config)
{
    uint8_t command = rx_buff[2]; // Extract command from the message
    switch (command) 
    {
        case '1':
            handle_command_1(rx_buff, config);
            break;
        case '2':
            handle_command_2(rx_buff, config);
            break;
        case '3':
            handle_command_3(rx_buff, config);
            break;
        case '4':
            handle_command_4(rx_buff, config);
            break;
        default:
            BSP_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
            break;
    }
}

// Command 1: Update frequency for Signal 1
static void handle_command_1(uint8_t rx_buff[], signal_config* config) 
{
    BSP_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); // Busy
    char frequency_value[6] = {rx_buff[3], rx_buff[4], rx_buff[5], rx_buff[6], rx_buff[7], rx_buff[8]};
    int value_int = atoi(frequency_value);
    if (value_int < 500) value_int = 500;
    if (value_int > 20000) value_int = 20000;
    config->freq_signal_1 = value_int;
    set_clock_tim2(config);
}

// Command 2: Update frequency for Signal 2
static void handle_command_2(uint8_t rx_buff[], signal_config* config) 
{
    BSP_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); // Busy
    char frequency_value[6] = {rx_buff[3], rx_buff[4], rx_buff[5], rx_buff[6], rx_buff[7], rx_buff[8]};
    int value_int = atoi(frequency_value);
    if (value_int < 500) value_int = 500;
    if (value_int > 20000) value_int = 20000;
    config->freq_signal_2 = value_int;
    set_clock_tim4(config);
}

// Command 3: Update amplitude for Channel 1
static void handle_command_3(uint8_t rx_buff[], signal_config* config) 
{
    BSP_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); // Busy
    char amplitude_value[2] = {rx_buff[7], rx_buff[8]};
    int value_int = atoi(amplitude_value);
    if (value_int < 1) value_int = 1;
    if (value_int > 68) value_int = 68;
    config->channel_1_sine_scale = (double)value_int / 100;
    get_channel_1_sine(NULL, config); // Assuming NULL is replaced with actual data
}

// Command 4: Update amplitude for Channel 2
static void handle_command_4(uint8_t rx_buff[], signal_config* config)
{
    BSP_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); // Busy
    char amplitude_value[2] = {rx_buff[7], rx_buff[8]};
    int value_int = atoi(amplitude_value);
    if (value_int < 1) value_int = 1;
    if (value_int > 68) value_int = 68;
    config->channel_2_sine_scale = (double)value_int / 100;
    get_channel_2_sine(NULL, config); // Assuming NULL is replaced with actual data
}
