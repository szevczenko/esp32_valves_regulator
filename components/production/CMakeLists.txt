idf_component_register(SRCS "cmd_nvs.c" "cmd_system.c" "cmd_wifi.c" "prod_app.c" "mdns_service.c" "ota.c"
                    INCLUDE_DIRS "." 
                    REQUIRES console nvs_flash backend spi_flash driver fatfs mdns)
