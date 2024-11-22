.PHONY: all build cmake clean flash

BUILD_DIR := build
# BUILD_TYPE ?= Debug
BUILD_TYPE ?= RelWithDebInfo 
# BUILD_TYPE ?= Release 
all: build

OUTPUT_BIN_NAME := output
# OUTPUT_BIN_NAME := $(notdir $(CURDIR))

cmake:
	cmake -B${BUILD_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DOUTPUT_BIN_NAME=${OUTPUT_BIN_NAME}

build: cmake
	$(MAKE) -C ${BUILD_DIR} --no-print-directory

clean:
	rm -rf $(BUILD_DIR)

flash: build
	JLinkExe -Device K1986BE92QI -If JTAG -Speed 1000 -JTAGConf -1,-1 JLink/FlashMCU.jlink

