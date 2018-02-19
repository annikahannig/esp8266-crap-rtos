
PROGRAM = esp_crap_rtos

ESPPORT=/dev/ttyUSB0

PROGRAM_SRC_DIR=./src
PROGRAM_INC_DIR=./src ./include

EXTRA_COMPONENTS = extras/i2s_dma extras/ws2812_i2s

include ../esp-open-rtos/common.mk

