################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WebServer\src\ESP8266WebServer.cpp \
C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WebServer\src\Parsing.cpp 

LINK_OBJ += \
.\libraries\ESP8266WebServer\src\ESP8266WebServer.cpp.o \
.\libraries\ESP8266WebServer\src\Parsing.cpp.o 

CPP_DEPS += \
.\libraries\ESP8266WebServer\src\ESP8266WebServer.cpp.d \
.\libraries\ESP8266WebServer\src\Parsing.cpp.d 


# Each subdirectory must supply rules for building sources it contributes
libraries\ESP8266WebServer\src\ESP8266WebServer.cpp.o: C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WebServer\src\ESP8266WebServer.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\tools\xtensa-lx106-elf-gcc\1.20.0-26-gb404fb9-2/bin/xtensa-lx106-elf-g++" -D__ets__ -DICACHE_FLASH -U__STRICT_ANSI__ "-IC:\Dev\eclipse\/arduinoPlugin/packages/esp8266/hardware/esp8266/2.3.0/tools/sdk/include" "-IC:\Dev\eclipse\/arduinoPlugin/packages/esp8266/hardware/esp8266/2.3.0/tools/sdk/lwip/include" "-IC:/Users/Utilisateur/eclipse-workspace/Bedlight/Release/core" -c -Wall -Wextra -Os -g -mlongcalls -mtext-section-literals -fno-exceptions -fno-rtti -falign-functions=4 -std=c++11 -MMD -ffunction-sections -fdata-sections -DF_CPU=160000000L -DLWIP_OPEN_SRC   -DARDUINO=10802 -DARDUINO_ESP8266_WEMOS_D1MINI -DARDUINO_ARCH_ESP8266 -DARDUINO_BOARD="\"ESP8266_WEMOS_D1MINI\""  -DESP8266   -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\cores\esp8266" -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\variants\d1_mini" -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WiFi\src" -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WebServer\src" -I"\\NAS\home\Documents\Arduino\libraries\ArduinoJson-6.x\src" -I"C:\Dev\eclipse\arduinoPlugin\libraries\PubSubClient\2.6.0\src" -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266mDNS" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"
	@echo 'Finished building: $<'
	@echo ' '

libraries\ESP8266WebServer\src\Parsing.cpp.o: C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WebServer\src\Parsing.cpp
	@echo 'Building file: $<'
	@echo 'Starting C++ compile'
	"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\tools\xtensa-lx106-elf-gcc\1.20.0-26-gb404fb9-2/bin/xtensa-lx106-elf-g++" -D__ets__ -DICACHE_FLASH -U__STRICT_ANSI__ "-IC:\Dev\eclipse\/arduinoPlugin/packages/esp8266/hardware/esp8266/2.3.0/tools/sdk/include" "-IC:\Dev\eclipse\/arduinoPlugin/packages/esp8266/hardware/esp8266/2.3.0/tools/sdk/lwip/include" "-IC:/Users/Utilisateur/eclipse-workspace/Bedlight/Release/core" -c -Wall -Wextra -Os -g -mlongcalls -mtext-section-literals -fno-exceptions -fno-rtti -falign-functions=4 -std=c++11 -MMD -ffunction-sections -fdata-sections -DF_CPU=160000000L -DLWIP_OPEN_SRC   -DARDUINO=10802 -DARDUINO_ESP8266_WEMOS_D1MINI -DARDUINO_ARCH_ESP8266 -DARDUINO_BOARD="\"ESP8266_WEMOS_D1MINI\""  -DESP8266   -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\cores\esp8266" -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\variants\d1_mini" -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WiFi\src" -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266WebServer\src" -I"\\NAS\home\Documents\Arduino\libraries\ArduinoJson-6.x\src" -I"C:\Dev\eclipse\arduinoPlugin\libraries\PubSubClient\2.6.0\src" -I"C:\Dev\eclipse\arduinoPlugin\packages\esp8266\hardware\esp8266\2.3.0\libraries\ESP8266mDNS" -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -D__IN_ECLIPSE__=1 -x c++ "$<"  -o  "$@"
	@echo 'Finished building: $<'
	@echo ' '


