# LED-Timer-Interrupt-Toggling
Toggle the LED based on the timer interrupt. The frequency is set through UART (such as PuTTY)

Microcontroller: TM4C123GH6PM by TI.

Architecture: ARM Cortex-M4

IDE: Code Composer Studio 6.

This sample code utilizes the board LED, the timer and the UART.
The user (through PuTTY or other UART senders) sends a value in milliseconds through UART.
This value is saved to the board, and it indicates the frequency at which the timer interrupts the processer. 
At each timer interrupt, the LED is toggled on and off.
