/*
 * File: traffic_config.h
 * Target: PIC18F452
 * Description: Hardware mapping, constants and State Definitions
 */

#ifndef TRAFFIC_CONFIG_H
#define TRAFFIC_CONFIG_H

#include <xc.h>

// --- Configuration Bits ---
#pragma config OSC = HS  // High Speed Oscillator
#pragma config WDT = OFF // Watchdog Timer Off
#pragma config LVP = OFF // Low Voltage Programming Off

#define _XTAL_FREQ 4000000 // 4MHz Clock

// --- Hardware Pin Mapping ---

// Outputs (Active High)
#define PED_GREEN LATDbits.LATD0
#define PED_RED LATDbits.LATD1
#define SIDE_GREEN LATDbits.LATD2
#define SIDE_AMBER LATDbits.LATD3
#define SIDE_RED LATDbits.LATD4
#define MAIN_GREEN LATDbits.LATD5
#define MAIN_AMBER LATDbits.LATD6
#define MAIN_RED LATDbits.LATD7

// Wait Lights (RB7 main, RB4-6 aux)
#define WAIT_LED_MAIN LATBbits.LATB7

// Inputs
// RB0: Pedestrian Button
// RA0: Main Sensor 1
// RA1: Side Sensor 1
// RA2: Main Sensor 2
// RA3: Side Sensor 2

// ADC Thresholds (10-bit)
// 1.4V / 5.0V * 1023 ~= 286
#define TRAFFIC_THRESHOLD 286

// --- State Definitions ---
typedef enum
{
    ST_MAIN_GREEN,
    ST_MAIN_AMBER,
    ST_ALL_RED_1,
    ST_PED_CROSS,
    ST_PED_CLEAR,
    ST_SIDE_PREP,
    ST_SIDE_GREEN,
    ST_SIDE_AMBER,
    ST_ALL_RED_2,
    ST_MAIN_PREP
} SystemState_t;

// --- Global Flags ---
extern volatile unsigned char ped_request_flag;

// --- Function Prototypes ---
void System_Init(void);
void FSM_Update(void);
unsigned int ADC_Read(unsigned char channel);
unsigned char Check_Side_Traffic(void);
unsigned char Check_Main_Traffic(void);

#endif