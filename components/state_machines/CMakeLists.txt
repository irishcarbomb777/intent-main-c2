idf_component_register(SRCS "SensorStateMachineTask.c" "sleep_state.c" "sleep_state2.c" "active_state.c" "ready_state.c" "NetworkStateMachineTask.c" "network_active_state.c" "network_inactive_state.c" "intent_mqtt.c"
                    REQUIRES "mqtt"
                    INCLUDE_DIRS "."
                    INCLUDE_DIRS "../wifi"
                    INCLUDE_DIRS "../lsm6dsr"
                    INCLUDE_DIRS "../gpio")