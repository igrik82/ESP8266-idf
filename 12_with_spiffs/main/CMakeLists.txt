set(SOURCES main.cpp)
idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . ../include)
# spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
