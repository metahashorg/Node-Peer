FIND_PATH(SECP256K1_INCLUDE_DIR secp256k1.h /usr/local/include /usr/include)

FIND_LIBRARY(SECP256K1_LIBRARY NAMES libsecp256k1.a PATH /usr/local/lib /usr/lib)

IF (SECP256K1_INCLUDE_DIR AND SECP256K1_LIBRARY)
    SET(SECP256K1_FOUND TRUE)
ENDIF ()

IF (SECP256K1_FOUND)
    IF (NOT SECP256K1_FIND_QUIETLY)
        MESSAGE(STATUS "Found libsecp256k1: ${SECP256K1_LIBRARY}")
    ENDIF ()
ELSE ()
    IF (SECP256K1_FIND_REQUIRED)
        IF (NOT SECP256K1_INCLUDE_DIR)
            MESSAGE(FATAL_ERROR "Could not find libsecp256k1 header file!")
        ENDIF ()

        IF (NOT SECP256K1_LIBRARY)
            MESSAGE(FATAL_ERROR "Could not find libsecp256k1 library file!")
        ENDIF ()
    ENDIF ()
ENDIF ()
