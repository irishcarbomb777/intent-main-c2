idf_component_register(SRCS "rtos_tasks.c"
                    REQUIRES "mqtt"
                    INCLUDE_DIRS "."
                    INCLUDE_DIRS "../wifi"
                    INCLUDE_DIRS "../lsm6dsr"
                    INCLUDE_DIRS "../gpio")
                    

                    