file(GLOB SOURCES "*.c*" "*/*.c*" "*/**/*.c*")
idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS ".")

