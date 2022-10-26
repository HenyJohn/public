file(GLOB_RECURSE source_list *.c)
idf_component_register(SRCS "${source_list}"
                        INCLUDE_DIRS
                         "."
                         "/home/dan/esp/esp-idf/components/nvs_flash/include"
                       #  "/home/dan/esp/esp-idf/examples/common_components/protocol_examples_common/include"
                        REQUIRES inv nvs_flash esp_http_server wpa_supplicant json common cloud)
