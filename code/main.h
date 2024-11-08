/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  * @details        : Defines the app_state structure and function prototypes.
  * @date           : 2023
  * @author         : Nicholas Antoniades
  ******************************************************************************
  */

#ifndef MAIN_H
#define MAIN_H

#include "signal_config.h"  // Assuming these headers define signal_config and uart_config
#include "uart_config.h"

typedef struct {
    signal_config signal_config;
    uart_config uart_config;
} app_state;

void init_config(signal_config *signal_config_ptr, uart_config *uart_config);
void main_loop(app_state *state);

#endif // MAIN_H 