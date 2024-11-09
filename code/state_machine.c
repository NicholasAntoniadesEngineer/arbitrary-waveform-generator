/**
  ******************************************************************************
  * @file           : state_machine.c
  * @brief          : State machine for handling commands
  * @details        : Processes commands to update signal parameters and states
  * @date           : 2020
  * @date           : Nicholas Antoniades
  ******************************************************************************
  */

#include <string.h>
#include <stdlib.h>

#include "state_machine.h"
#include "stm32_bsp.h"


static void state_1(uint8_t rx_buff[], signal_state* state);
static void state_2(uint8_t rx_buff[], signal_state* state);
static void state_3(uint8_t rx_buff[], signal_state* state);
static void state_4(uint8_t rx_buff[], signal_state* state);

void state_machine(uint8_t rx_buff[], signal_state* state)
{
    uint8_t command = rx_buff[2]; // Extract command from the message
    switch (command) 
    {
        case '1':
            state_1(rx_buff, state);
            break;
        case '2':
            state_2(rx_buff, state);
            break;
        case '3':
            state_3(rx_buff, state);
            break;
        case '4':
            state_4(rx_buff, state);
            break;
        default:
            BSP_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
            break;
    }
}

// State 1: Update frequency for Signal 1
static void state_1(uint8_t rx_buff[], signal_state* state) 
{
    BSP_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); // Busy
    char frequency_value[6] = {rx_buff[3], rx_buff[4], rx_buff[5], rx_buff[6], rx_buff[7], rx_buff[8]};
    int value_int = atoi(frequency_value);
    if (value_int < state->min_freq) value_int = state->min_freq;
    if (value_int > state->max_freq) value_int = state->max_freq;
    state->freq_signal_1 = value_int;
    set_clock_tim2(state);
}

// State 2: Update frequency for Signal 2
static void state_2(uint8_t rx_buff[], signal_state* state) 
{
    BSP_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); // Busy
    char frequency_value[6] = {rx_buff[3], rx_buff[4], rx_buff[5], rx_buff[6], rx_buff[7], rx_buff[8]};
    int value_int = atoi(frequency_value);
    if (value_int < state->min_freq) value_int = state->min_freq;
    if (value_int > state->max_freq) value_int = state->max_freq;
    state->freq_signal_2 = value_int;
    set_clock_tim4(state);
}

// State 3: Update amplitude for Channel 1
static void state_3(uint8_t rx_buff[], signal_state* state) 
{
    BSP_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); // Busy
    char amplitude_value[2] = {rx_buff[7], rx_buff[8]};
    int value_int = atoi(amplitude_value);
    if (value_int < state->min_amplitude) value_int = state->min_amplitude;
    if (value_int > state->max_amplitude) value_int = state->max_amplitude;
    state->channel_1_sine_scale = (double)value_int / 100;
    get_channel_1_sine(NULL, state); // Assuming NULL is replaced with actual data
}

// State 4: Update amplitude for Channel 2
static void state_4(uint8_t rx_buff[], signal_state* state)
{
    BSP_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); // Busy
    char amplitude_value[2] = {rx_buff[7], rx_buff[8]};
    int value_int = atoi(amplitude_value);
    if (value_int < state->min_amplitude) value_int = state->min_amplitude;
    if (value_int > state->max_amplitude) value_int = state->max_amplitude;
    state->channel_2_sine_scale = (double)value_int / 100;
    get_channel_2_sine(NULL, state); // Assuming NULL is replaced with actual data
}
