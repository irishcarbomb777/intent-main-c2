idf_component_register(SRCS "SensorStateMachineTask.c" "NetworkStateMachineTask.c"
                    REQUIRES "mqtt"
                    INCLUDE_DIRS "."
                    INCLUDE_DIRS "../wifi"
                    INCLUDE_DIRS "../lsm6dsr"
                    INCLUDE_DIRS "../gpio")