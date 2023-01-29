Coolant sensor:  PANW103395-395 (Digikey 570-1458-ND) inserted into:
                 https://www.amazon.com/dp/B09V6Y4ST8
	         set R6 = 10k, R4 = 0R, R7 = open

Battery voltage: set R6 = open, R4 = 49.9k 1%, R7 = 10.0k 1%

Exhaust sensor: K Type thermocouple

Ignition sensor: diode-protected input with pullup.  When ignition is on, 
                 the diode is reverse biased, and thus the input is high.
		 when ignition is on, the diode is forward biased, and the
                 input is low (around 0.7V).

Outdoors temp sensor:  DS18B20 on a cable - Adafruit part 381, connected to 
                       DS2482S-100+ 1Wire-I2C interface.

