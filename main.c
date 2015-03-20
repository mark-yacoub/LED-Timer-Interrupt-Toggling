#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"



int8_t m_uartReceived[8]; // assuming the number sent will not more than 8 digits
int8_t m_LedStatus = 1;
int32_t m_charCount = 0;


//*****************************************************************************
// The error routine that is called if the driver library encounters an error.
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif


//*****************************************************************************
// Timer interrupt that toggles the LED at a preset frequency
//*****************************************************************************
void TimerIntHandler(void)
{
	ROM_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	ROM_IntMasterDisable();

	// toggle the LED
	m_LedStatus ^= 1;
	if (m_LedStatus == 0)
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
	else
		GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

	ROM_IntMasterEnable();
}


//**************************************************************************************
// The UART interrupt handler, whenever anything is received from Putty, or serial input
// it modifies the frequency at which the timer interrupts the processer
// at the timer interrupt, the LED is toggled
//**************************************************************************************
void UARTIntHandler(void)
{
    uint32_t ui32Status;
    int8_t c;
    int8_t i;

    // Get the interrrupt status.
    ui32Status = ROM_UARTIntStatus(UART0_BASE, true);

    // Clear the asserted interrupts.
    ROM_UARTIntClear(UART0_BASE, ui32Status);
    // Loop while there are characters in the receive FIFO.
    while(ROM_UARTCharsAvail(UART0_BASE))
    {
        // Read the next character from the UART and write it back to the UART.
        c = ROM_UARTCharGetNonBlocking(UART0_BASE);

        // check if 'Enter' has been pressed
        if (c != 13)
		{
        	// rewrite back to the Serial screen for the usual to see what he's typing
			ROM_UARTCharPutNonBlocking(UART0_BASE, c);
			// added the char in the array of chars
			m_uartReceived[m_charCount++] = c;
		}

        else
        {
        	// skip to the next line to see all inputs stacked on top of each other
			ROM_UARTCharPutNonBlocking(UART0_BASE,'\n');
			ROM_UARTCharPutNonBlocking(UART0_BASE,'\r');

			// set a null char to the last index to indicate the end of the string
        	m_uartReceived[m_charCount] = '\0';

        	// convert the array of characters to a number
        	int32_t delayValue = atoi(m_uartReceived);
        	// reset the array of chars
        	m_charCount = 0;
        	for (i=0; i<8; ++i)
        	{
        		m_uartReceived[i] = '0';
        	}

        	// check if delay is a second or less
        	float freqPerSec;
        	if (delayValue <= 1000)
        		freqPerSec = 1000/delayValue;
        	else
        		freqPerSec = 1/(delayValue/1000);

        	// Set the frequency of the timer
        	ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, ROM_SysCtlClockGet()/freqPerSec);
        }

        // turn on the LED light
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);
    }
}


//*****************************************************************************
// Enable the Interrupt for the timer
//*****************************************************************************
void EnableTimerInterrupt(void)
{
	// Enable the peripherals used by this example.
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	// Configure the two 32-bit periodic timers.
	ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, ROM_SysCtlClockGet());
	ROM_IntEnable(INT_TIMER0A);
	ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerEnable(TIMER0_BASE, TIMER_A);
}

//*****************************************************************************
// The Main Function
//*****************************************************************************
int main(void)
{
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    ROM_FPUEnable();
    ROM_FPULazyStackingEnable();

    // Set the clocking to run directly from the crystal.
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    // Enable the GPIO port that is used for the on-board LED.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

    // Enable the GPIO pins for the LED (PF2).
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2);

    // Enable the peripherals used by this example.
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Enable processor interrupts.
    ROM_IntMasterEnable();

    // Set GPIO A0 and A1 as UART pins.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    // Configure the UART for 115,200, 8-N-1 operation.
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                             UART_CONFIG_PAR_NONE));

    // Enable the UART interrupt.
    ROM_IntEnable(INT_UART0);
    ROM_UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT);

	int8_t i;
	for (i=0; i<8; ++i)
		m_uartReceived[i] = '0';

    EnableTimerInterrupt();

    while(1) {;}
}
