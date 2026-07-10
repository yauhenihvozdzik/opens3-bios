idf_component_register(SRCS "opens3_fs.c" "hal_flash.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES spi_flash esp_partition)
