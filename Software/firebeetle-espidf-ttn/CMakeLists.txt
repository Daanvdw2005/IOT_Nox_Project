cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(firebeetle-espidf-ttn)

set( CMAKE_C_STANDARD 11 )

idf_component_register( 
    SRCS "main/main.c"
    INCLUDE_DIRS ""
    REQUIRES lora_radio ttn
)