# Esp8266PCF8583WaterCounter

Project use:

PlatformIO  (http://platformio.org/)
Visual Studio Code (https://code.visualstudio.com/)

The device reads the readings from the water meter through the built-in reed switch, meter electricity through the output for verification.

Hardware needs ESP8266 ESP12-E, PCF8583 and 74HC00 to suppress the chatter of the contact.

This project implements ideas for my personal IOT from gitHub, ESP8266 libraries and much more. The device has been working for more than a month.

The source code can be considered as an example of the implementation of libraries:

OneWire: http://www.pjrc.com/teensy/td_libs_OneWire.html
DallasTemperature: https://github.com/milesburton/Arduino-Temperature-Control-Library
RemoteDebug: https://github.com/JoaoLopesF/RemoteDebug
SoftwareSerial, PubSubClient, ArduinoOTA with ESP8266

Also thanks to the forum http://www.homes-smart.ru/index.php/component/kunena/4-zhelezo/968-sbor-i-peredacha-pokazanij-schetchikov-vody-gaza-i-elektrichestv?q=/index.php/component/kunena/4-zhelezo/968-sbor-i-peredacha-pokazanij-schetchikov-vody-gaza-i-elektrichestv

