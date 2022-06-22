#! /bin/bash

PROJECT_ROOT=$(pwd)
BUILD_DIR=${PROJECT_ROOT}/build/marco_build

LIBECC_ROOT=${PROJECT_ROOT}/libecc

cmake -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build

mkdir -p build
mkdir -p ${BUILD_DIR}
cd build

# libquestdb_client lib

cp ${LIBECC_ROOT}/build/libsign.a .

# Build C example
/usr/bin/cc  -I${PROJECT_ROOT}/include \
-std=c11 \
-DWITH_STDLIB \
-MD \
-MT ${BUILD_DIR}/line_sender_c_example.c.o \
-MF ${BUILD_DIR}/line_sender_c_example.c.o.d \
-o ${BUILD_DIR}/line_sender_c_example.c.o \
-c ${PROJECT_ROOT}/examples/line_sender_c_example.c -g

/usr/bin/cc \
-DWITH_STDLIB \
-o ${BUILD_DIR}/authentication.c.o \
-c ${PROJECT_ROOT}/src/authentication.c \
-g

/usr/bin/cc \
-DWITH_STDLIB \
-o ${BUILD_DIR}/base64.c.o \
-c ${PROJECT_ROOT}/src/base64.c \
-g

/usr/bin/cc  -I/home/makko/github/libecc/src/external_deps \
-DWITH_STDLIB \
-o ${BUILD_DIR}/print.c.o \
-c /home/makko/github/libecc/src/external_deps/print.c

/usr/bin/cc  -I/home/makko/github/libecc/src/external_deps \
-DWITH_STDLIB \
-o ${BUILD_DIR}/rand.c.o \
-c /home/makko/github/libecc/src/external_deps/rand.c

# Link C example
/usr/bin/cc \
${BUILD_DIR}/line_sender_c_example.c.o \
${BUILD_DIR}/authentication.c.o \
${BUILD_DIR}/base64.c.o \
${BUILD_DIR}/print.c.o \
${BUILD_DIR}/rand.c.o \
-o line_sender_c_example \
libc_questdb_client_static.a \
libsign.a \
-ldl -lrt -lpthread -lgcc_s -lc -lm -lutil -g
