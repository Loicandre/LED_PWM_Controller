# LED_PWM_Controller

LED PWM Controller for camping van, PIC12F1572

For use with 12V lead battery, but 24v is also possible if voltages divisiors is adapted.

* 50W power, 15V max (or more if you change voltage divisor) 
* UART controlled 
* High frequency PWM (26khz), no audible noise and no flickering
* No Off current (potentiometer Mechanical switch) 
* Low level battery monitornig ( Cutoff<=10.25V, 7%PWM Max<=11V, Normal>=11.5V) 
* Simple Connector, 6.35 terminals
* OverCurrent protection

<img src="https://github.com/Loicandre/LED_PWM_Controller/blob/main/DOC/IMG_20200929_100800.jpg" width="575"> 

<p float="left"> 
  <img src="https://github.com/Loicandre/LED_PWM_Controller/blob/main/DOC/IMG_20200929_101517.jpg" width="220">
  <img src="https://github.com/Loicandre/LED_PWM_Controller/blob/main/DOC/IMG_20200929_100811.jpg" width="350"> 
</p>

## Hardware : 

Schematics [here...](https://github.com/Loicandre/LED_PWM_Controller/blob/main/HW/Schematic%20Prints.PDF)

Main parts are :
* PIC12F1572-E/MS Microcontroller (MSOP version) 
* TE 17PCSA103MC19P Potentiometer with switch 
* AP7383-33Y-13 Voltage Regulator 
* RUEF500 PPTC Current protector 
* NVD5C486NLT4G MOSFET N 
* TE 63951-1, 6.35 connector

STL 3D printed Housing provided in HW folder

## UART Command : 

UART has priority. To return to the potentiometer control, stop sending UARTs frames and turn the potentiometer a bit.

UART HW is 2 pin 2mm JST connector with RX only and GND, 
3V3, 9600B, 8n1

### Command is :
| B1 | B2 | B3 | B4 | B5 | CHK |
|:----|:----|:----|:----|:----|:----:|
1. B1 = 0xCA
2. B2 = 0xFE
3. B3 = ADDR (currently define by preprocessor) or BrdCast (0xFF)
4. B4 = LedMSB
5. B5 = LedLSB 
6. CHK=B1+B2+B3+B4+B5 (8bit)


## Tools : 

Compiler : XC8 2.10
IDE : MPLAB X V5.35
Programmer : Microchip PICKIT3
