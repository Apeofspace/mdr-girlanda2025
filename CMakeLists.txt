cmake_minimum_required(VERSION 3.7...3.28)
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

#-----PROJECT NAME-----
# this hack puts dir name as project name
# for serious project its better to set up actual name (also do it in Makefile)
# (also do it in JLink files)
cmake_path(GET CMAKE_CURRENT_SOURCE_DIR FILENAME ProjectName)   
string(REPLACE " " "_" ProjectId ${ProjectName})
project(${ProjectName})

#-----MCU-----
set(MCPU cortex-m3)
set(MCU_MODEL MDR1986VE9x)

#----COMPILER SETTINGS----
enable_language(C CXX ASM)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

#----SOURCES----
file(GLOB_RECURSE C_SRC CONFIGURE_DEPENDS "src/*.c" )

file(GLOB_RECURSE C_LIBS CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/lib/*.c
    # problems with bkp and eeprom and IRQ templates
    )

include_directories(
   ${CMAKE_SOURCE_DIR}/lib 
   ${CMAKE_SOURCE_DIR}/lib/Config 
   ${CMAKE_SOURCE_DIR}/lib/CMSIS/CM3/DeviceSupport/MDR32F9Qx/inc 
   ${CMAKE_SOURCE_DIR}/lib/MDR32F9Qx_StdPeriph_Driver/inc 
   ${CMAKE_SOURCE_DIR}/lib/MDR32F9Qx_StdPeriph_Driver/inc/USB_Library 
   ${CMAKE_SOURCE_DIR}/lib/CMSIS/CM3/CoreSupport 
   ${CMAKE_SOURCE_DIR}/lib/CMSIS/CM3/DeviceSupport/MDR32F9Qx/startup/gcc 
   # ${CMAKE_SOURCE_DIR}/lib/MDR32F9Qx_StdPeriph_Driver/inc/IRQ_Handler_Template
   )

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/lib/CMSIS/CM3/DeviceSupport/MDR32F9Qx/startup/gcc/MDR32F9Qx.ld)
set(STARTUP_SCRIPT ${CMAKE_SOURCE_DIR}/lib/CMSIS/CM3/DeviceSupport/MDR32F9Qx/startup/gcc/startup_MDR32F9Qx.s) 

#-----COMPILE OPTIONS-----
add_definitions(-DUSE_${MCU_MODEL} -D__STARTUP_CLEAR_BSS -D__START=main)

add_compile_options(
    -mcpu=${MCPU}
    -mthumb
    -mfloat-abi=soft  
    -ffunction-sections 
    -fdata-sections 
    -flto
    -fmessage-length=0
    -Wextra
    -Wshadow
    # -Wredundant-decls
    -Wno-missing-field-initializers 
    -Wno-cpp
    # -Wconversion
    -Wno-packed-bitfield-compat
    -pipe
    -MMD
    -MP)

add_link_options(
    -T ${LINKER_SCRIPT}
    -mcpu=${MCPU}
    -mthumb
    -specs=nano.specs 
    -lc -lm -lnosys 
    -Wl,-Map=${PROJECT_BINARY_DIR}/${PROJECT_NAME}.map,--cref -Wl,--gc-sections)

# Uncomment to mitigate c++17 absolute addresses warnings
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} 
# -Wno-register 
# -Wconversion # STM libraries!
# -Wno-volatile
# -Wold-style-cast
# -Wuseless-cast
# -Wsuggest-override")

#-----BUILD TYPE-----
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
    add_compile_options(-Ofast)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
    add_compile_options(-Ofast -g)
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
    add_compile_options(-Os)
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_options(-Og -g)
endif ()

#-----BUILD COMMANDS-----
set(OUTPUT_NAME output)
# set(OUTPUT_NAME ${PROJECT_NAME})
add_executable(${OUTPUT_NAME}.elf ${C_SRC} ${C_LIBS} ${STARTUP_SCRIPT} ${LINKER_SCRIPT})
set(HEX_FILE ${PROJECT_BINARY_DIR}/${OUTPUT_NAME}.hex)
set(BIN_FILE ${PROJECT_BINARY_DIR}/${OUTPUT_NAME}.bin)

# target_link_libraries(${OUTPUT_NAME} m) # link math.h 

add_custom_command(TARGET ${OUTPUT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${OUTPUT_NAME}.elf> ${HEX_FILE}
    COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${OUTPUT_NAME}.elf> ${BIN_FILE}
    COMMENT "Building ${HEX_FILE}
    Building ${BIN_FILE}")

