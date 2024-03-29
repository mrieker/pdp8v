EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:aoigate-cache
EELAYER 25 0
EELAYER END
$Descr USLetter 11000 8500
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L 74HC04 U1
U 1 1 64734C30
P 5400 3000
F 0 "U1" H 5550 3100 50  0000 C CNN
F 1 "1/2-6J6" H 5600 2900 50  0000 C CNN
F 2 "" H 5400 3000 50  0001 C CNN
F 3 "" H 5400 3000 50  0001 C CNN
	1    5400 3000
	1    0    0    -1  
$EndComp
$Comp
L R R1
U 1 1 64734D1C
P 4300 2700
F 0 "R1" V 4380 2700 50  0000 C CNN
F 1 "47K" V 4300 2700 50  0000 C CNN
F 2 "" V 4230 2700 50  0001 C CNN
F 3 "" H 4300 2700 50  0001 C CNN
	1    4300 2700
	0    1    1    0   
$EndComp
$Comp
L D_ALT D1
U 1 1 64734DFD
P 4300 3000
F 0 "D1" H 4300 3100 50  0000 C CNN
F 1 "1N4148" H 4300 2900 50  0000 C CNN
F 2 "" H 4300 3000 50  0001 C CNN
F 3 "" H 4300 3000 50  0001 C CNN
	1    4300 3000
	1    0    0    -1  
$EndComp
$Comp
L D_ALT D2
U 1 1 64734E25
P 4300 3300
F 0 "D2" H 4300 3400 50  0000 C CNN
F 1 "1N4148" H 4300 3200 50  0000 C CNN
F 2 "" H 4300 3300 50  0001 C CNN
F 3 "" H 4300 3300 50  0001 C CNN
	1    4300 3300
	1    0    0    -1  
$EndComp
$Comp
L D_ALT D6
U 1 1 64734EA5
P 4700 3000
F 0 "D6" H 4700 3100 50  0000 C CNN
F 1 "1N4148" H 4700 2900 50  0000 C CNN
F 2 "" H 4700 3000 50  0001 C CNN
F 3 "" H 4700 3000 50  0001 C CNN
	1    4700 3000
	-1   0    0    -1  
$EndComp
$Comp
L R R2
U 1 1 6473508F
P 4300 3600
F 0 "R2" V 4380 3600 50  0000 C CNN
F 1 "47K" V 4300 3600 50  0000 C CNN
F 2 "" V 4230 3600 50  0001 C CNN
F 3 "" H 4300 3600 50  0001 C CNN
	1    4300 3600
	0    1    1    0   
$EndComp
$Comp
L D_ALT D3
U 1 1 64735095
P 4300 3900
F 0 "D3" H 4300 4000 50  0000 C CNN
F 1 "1N4148" H 4300 3800 50  0000 C CNN
F 2 "" H 4300 3900 50  0001 C CNN
F 3 "" H 4300 3900 50  0001 C CNN
	1    4300 3900
	1    0    0    -1  
$EndComp
$Comp
L D_ALT D4
U 1 1 6473509B
P 4300 4200
F 0 "D4" H 4300 4300 50  0000 C CNN
F 1 "1N4148" H 4300 4100 50  0000 C CNN
F 2 "" H 4300 4200 50  0001 C CNN
F 3 "" H 4300 4200 50  0001 C CNN
	1    4300 4200
	1    0    0    -1  
$EndComp
$Comp
L D_ALT D5
U 1 1 647350A1
P 4300 4500
F 0 "D5" H 4300 4600 50  0000 C CNN
F 1 "1N4148" H 4300 4400 50  0000 C CNN
F 2 "" H 4300 4500 50  0001 C CNN
F 3 "" H 4300 4500 50  0001 C CNN
	1    4300 4500
	1    0    0    -1  
$EndComp
$Comp
L D_ALT D7
U 1 1 647352A5
P 4700 3900
F 0 "D7" H 4700 4000 50  0000 C CNN
F 1 "1N4148" H 4700 3800 50  0000 C CNN
F 2 "" H 4700 3900 50  0001 C CNN
F 3 "" H 4700 3900 50  0001 C CNN
	1    4700 3900
	-1   0    0    -1  
$EndComp
Wire Wire Line
	4450 2700 4500 2700
Wire Wire Line
	4500 2700 4500 3300
Wire Wire Line
	4500 3300 4450 3300
Wire Wire Line
	4450 3000 4550 3000
Connection ~ 4500 3000
Wire Wire Line
	4450 3600 4500 3600
Wire Wire Line
	4500 3600 4500 4500
Wire Wire Line
	4500 4500 4450 4500
Wire Wire Line
	4450 4200 4500 4200
Connection ~ 4500 4200
Wire Wire Line
	4450 3900 4550 3900
Connection ~ 4500 3900
Wire Wire Line
	4950 3000 4850 3000
Wire Wire Line
	4850 3900 4900 3900
Wire Wire Line
	4900 3900 4900 3000
Connection ~ 4900 3000
Wire Wire Line
	4150 2700 3950 2700
Wire Wire Line
	4150 3000 3950 3000
Wire Wire Line
	4150 3300 3950 3300
Wire Wire Line
	4150 3600 3950 3600
Wire Wire Line
	4150 3900 3950 3900
Wire Wire Line
	4150 4200 3950 4200
Wire Wire Line
	4150 4500 3950 4500
Text Label 3950 2700 0    60   ~ 0
+5V
Text Label 3950 3600 0    60   ~ 0
+5V
$EndSCHEMATC
