Along with the libraries included under this directory, the following
must be added in the Arduino IDE:

Boards:
- Add the STM32 boards to index:  https://github.com/stm32duino/BoardManagerFiles/raw/main/package\_stmicroelectronics\_index.json
- choose Generic STM32F0 Series
- in Tools -> Board part number:  Generic F042F6Px

Libraries:
- Queue by SMFSW <xgarmanbozlax@gmail.com> - v1.11.0
- ArduinoLog by Thijs Elenbaas - v1.1.1

also, to populate this directory, you need:
- git submodule init
- get submodule update
