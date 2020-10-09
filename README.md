# LED_PWM_Controller

LED PWM Controller for camping van, PIC12F1572

For use with 12V lead battery, but 24v is also possible if voltages divisiors is adapted.

* 50W power, 15V max 
* UART controlled 
* No Off current (potentiometer Mechanical switch) 
* Low level battery monitornig ( Cutoff<=10.25V, 7%PWM Max<=11V, Normal>=11.5V) 
* Simple Connector 
* OverCurrent protection

## Hardware : 

Main parts are :
* PIC12F1572-E/MS Microcontroller (MSOP version) 
* TE 17PCSA103MC19P Potentiometer with switch 
* AP7383-33Y-13 Voltage Regulator 
* RUEF500 PPTC Current protector 
* NVD5C486NLT4G MOSFET N 
* TE 63951-1, 6.35 connector

STL 3D printed Housing provided in HW folder

## UART Command : 

UART HW is 2 pin 2mm JST connector with RX only and GND
3V3, 9600B, 8n1

### Command is :
| B1 | B2 | B3 | B4 | B5 | CHK |
1. B1 = 0xCA
2. B2 = 0xFE
3. B3 = ADDR or BrdCast
4. B4 = LedMSB
5. B5 = LedLSB 
6. CHK=B1+B2+B3+B4+B5 (8bit)


## Tools : 

Compiler : XC8 2.10
IDE : MPLAB X V5.35
Programmer : Microchip PICKIT3
