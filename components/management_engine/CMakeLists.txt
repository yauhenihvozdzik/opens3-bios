# ESP32-S3 does not have LP-Core RISC-V coprocessor.
# Management Engine runs as a FreeRTOS task on the HP core instead.
idf_component_register(SRCS "management_engine.c" "management_engine_s3.c"
                       INCLUDE_DIRS "include"
                       REQUIRES freertos esp_hw_support esp_system esp_driver_gpio esp_timer)