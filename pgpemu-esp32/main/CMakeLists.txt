FILE(GLOB CSources *.c)

idf_component_register(
    SRCS ${CSources}
    INCLUDE_DIRS "."
    REQUIRES
        esp_lcd
        esp_driver_i2c
        esp_adc
        driver
        freertos
        bt
        nvs_flash
        esp_system
        log
        mbedtls
)

nvs_create_partition_image(nvs ../secrets.csv FLASH_IN_PROJECT)
