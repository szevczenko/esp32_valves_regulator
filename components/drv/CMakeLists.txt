idf_component_register(SRCS "battery.c" "but.c" "buzzer.c" "error_valve.c" "fast_add.c" 
                            "keepalive.c" "measure.c" "pcf8574.c" "ringBuff.c" "sleep.c"
                            "server_conroller.c" "ultrasonar.c" "power_on.c" "led.c"
                            "pwm_drv.c" "parameters.c" "water_flow_sensor.c"
                    INCLUDE_DIRS "." 
                    REQUIRES config backend menu main)
