idf_component_register(SRCS "sps30_i2c.c" "main.c"
                    INCLUDE_DIRS ".")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error=unused-value)