/*
 * File: main.c
 * Target: PIC18F452
 */

#include "traffic_config.h"

// --- Global Variables ---
volatile unsigned char ped_request_flag = 0;
SystemState_t current_state = ST_MAIN_GREEN;

// --- Interrupt Service Routine ---
void __interrupt() ISR(void)
{
    // Check INT0 (RB0) - Pedestrian Button
    if (INTCONbits.INT0IF)
    {
        // Button pressed (Active Low input, triggered on falling edge)
        ped_request_flag = 1;

        // Immediate UI feedback: Turn on Wait Lights
        WAIT_LED_MAIN = 1;
        LATBbits.LATB4 = 1; // Aux lights
        LATBbits.LATB5 = 1;
        LATBbits.LATB6 = 1;

        INTCONbits.INT0IF = 0; // Clear flag
    }
}

// Decodes the state into physical pin High/Low signals
void Set_Outputs(SystemState_t state)
{
    // 1. Reset all traffic lights to OFF (Safety)
    LATD = 0x00;

    // 2. Set specific lights based on state
    switch (state)
    {
    case ST_MAIN_GREEN:
        MAIN_GREEN = 1;
        SIDE_RED = 1;
        PED_RED = 1;
        break;
    case ST_MAIN_AMBER:
        MAIN_AMBER = 1;
        SIDE_RED = 1;
        PED_RED = 1;
        break;
    case ST_ALL_RED_1:
    case ST_ALL_RED_2:
    case ST_PED_CLEAR:
        MAIN_RED = 1;
        SIDE_RED = 1;
        PED_RED = 1;
        break;
    case ST_PED_CROSS:
        MAIN_RED = 1;
        SIDE_RED = 1;
        PED_GREEN = 1;
        break;
    case ST_SIDE_PREP:
        // UK Standard: Side goes Red + Amber
        MAIN_RED = 1;
        PED_RED = 1;
        SIDE_RED = 1;
        SIDE_AMBER = 1;
        break;
    case ST_SIDE_GREEN:
        MAIN_RED = 1;
        PED_RED = 1;
        SIDE_GREEN = 1;
        break;
    case ST_SIDE_AMBER:
        MAIN_RED = 1;
        PED_RED = 1;
        SIDE_AMBER = 1;
        break;
    case ST_MAIN_PREP:
        // UK Standard: Main goes Red + Amber
        SIDE_RED = 1;
        PED_RED = 1;
        MAIN_RED = 1;
        MAIN_AMBER = 1;
        break;
    }
}

// --- Main Loop ---
void main(void)
{
    System_Init();

    while (1)
    {
        Set_Outputs(current_state);
        FSM_Update();
    }
}

// --- State Machine Logic ---
void FSM_Update(void)
{
    switch (current_state)
    {
    case ST_MAIN_GREEN:
        // Minimum Green Time (Simulated)
        __delay_ms(5000);

        // Check for exit conditions
        // Priority: Pedestrian -> Side Road
        if (ped_request_flag || Check_Side_Traffic())
        {
            current_state = ST_MAIN_AMBER;
        }
        else
        {
            // Keep checking every 100ms if no demand
            __delay_ms(100);
        }
        break;

    case ST_MAIN_AMBER:
        __delay_ms(3000); // Standard 3s Amber
        current_state = ST_ALL_RED_1;
        break;

    case ST_ALL_RED_1:
        __delay_ms(2000); // Safety buffer

        if (ped_request_flag)
        {
            current_state = ST_PED_CROSS;
        }
        else
        {
            // If we are here, it must be side traffic (or default flow)
            current_state = ST_SIDE_PREP;
        }
        break;

    case ST_PED_CROSS:
        // Clear Request Flags/Lights
        ped_request_flag = 0;
        WAIT_LED_MAIN = 0;
        LATBbits.LATB4 = 0;
        LATBbits.LATB5 = 0;
        LATBbits.LATB6 = 0;

        __delay_ms(10000); // 10s Crossing time
        current_state = ST_PED_CLEAR;
        break;

    case ST_PED_CLEAR:
        __delay_ms(3000); // Clearance time
        // Decision: Go to Side if traffic exists, else return Main
        if (Check_Side_Traffic())
        {
            current_state = ST_SIDE_PREP;
        }
        else
        {
            current_state = ST_MAIN_PREP;
        }
        break;

    case ST_SIDE_PREP:
        __delay_ms(2000); // Red+Amber
        current_state = ST_SIDE_GREEN;
        break;

    case ST_SIDE_GREEN:
        __delay_ms(5000); // Min Green

        // Return to Main if Main Traffic exists or simply after a max time
        // Real-world: Check if Main road is piling up or Side is empty
        if (Check_Main_Traffic() || !Check_Side_Traffic())
        {
            current_state = ST_SIDE_AMBER;
        }
        else
        {
            // Simple timeout for demo prevents infinite blocking
            __delay_ms(5000);
            current_state = ST_SIDE_AMBER;
        }
        break;

    case ST_SIDE_AMBER:
        __delay_ms(3000);
        current_state = ST_ALL_RED_2;
        break;

    case ST_ALL_RED_2:
        __delay_ms(2000);
        current_state = ST_MAIN_PREP;
        break;

    case ST_MAIN_PREP:
        __delay_ms(2000); // Red+Amber
        current_state = ST_MAIN_GREEN;
        break;
    }
}

// --- Hardware Helper Functions ---

void System_Init(void)
{
    // 1. Port Directions (0=Output, 1=Input)
    TRISA = 0xFF; // RA0-RA3 are Sensors
    TRISB = 0x01; // RB0 Input, RB4-7 Output (Wait Lights)
    TRISD = 0x00; // Traffic Lights Output

    // 2. Clear Outputs
    LATB = 0x00;
    LATD = 0x00;

    // 3. ADC Configuration
    // PCFG = 0100 (AN0-AN2 Analog? No, we need AN0-AN3)
    // ADCON1: PCFG3:PCFG0
    // Setting 0x84 makes AN0, AN1, AN3 analog (Check Datasheet for 452 specifically)
    // Safe generic setting for 452: 0x80 (All Analog, Right Justified)
    ADCON1 = 0x80;
    ADCON0bits.ADON = 1; // Enable ADC Module

    // 4. Interrupts (RB0)
    INTCONbits.INT0IE = 1;   // Enable INT0
    INTCON2bits.INTEDG0 = 0; // Falling Edge (Active Low button)
    INTCONbits.GIE = 1;      // Global Interrupts
}

unsigned int ADC_Read(unsigned char channel)
{
    // Set channel (Bits 5-3 of ADCON0)
    ADCON0 &= 0xC7; // Clear channel bits
    ADCON0 |= (channel << 3);

    __delay_ms(2); // Acquisition time

    ADCON0bits.GO = 1; // Start conversion
    while (ADCON0bits.GO)
        ; // Wait for finish

    return ((ADRESH << 8) + ADRESL);
}

// Sensors: Main (RA0/RA2), Side (RA1/RA3)
unsigned char Check_Main_Traffic(void)
{
    // RA0 (Ch0) and RA2 (Ch2)
    if (ADC_Read(0) > TRAFFIC_THRESHOLD)
        return 1;
    if (ADC_Read(2) > TRAFFIC_THRESHOLD)
        return 1;
    return 0;
}

unsigned char Check_Side_Traffic(void)
{
    // RA1 (Ch1) and RA3 (Ch3)
    if (ADC_Read(1) > TRAFFIC_THRESHOLD)
        return 1;
    if (ADC_Read(3) > TRAFFIC_THRESHOLD)
        return 1;
    return 0;
}