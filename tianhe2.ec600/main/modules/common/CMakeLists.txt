
file(GLOB_RECURSE source_list *.c)
idf_component_register(SRCS "${source_list}"
                    INCLUDE_DIRS
                     "."
                     "/home/dan/esp/esp-idf/components/nvs_flash/include"
                     "/home/dan/esp/esp-idf/components/spi_flash/include"
                     "/home/dan/esp/esp-idf/components/fatfs/vfs"
                     "/home/dan/esp/esp-idf/examples/common_components/protocol_examples_common/include"
                     "/home/dan/esp/esp-idf/components/app_update/include"
# REQUIRES inv esp-tls esp_http_client app_update fatfs esp_https_ota driver json)
REQUIRES inv app_update fatfs driver json)


