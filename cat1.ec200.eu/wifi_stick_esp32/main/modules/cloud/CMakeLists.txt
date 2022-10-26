
file(GLOB_RECURSE source_list *.c)
idf_component_register(SRCS "${source_list}"
                    INCLUDE_DIRS "."
                    REQUIRES inv wifi common update)

# REQUIRES inv mqtt wifi common update)