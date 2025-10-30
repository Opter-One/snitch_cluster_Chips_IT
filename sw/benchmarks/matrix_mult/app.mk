APP              := bench_matr
$(APP)_BUILD_DIR := $(SN_ROOT)/sw/benchmarks/matrix_mult/build
SRCS             := $(SN_ROOT)/sw/benchmarks/matrix_mult/src/$(APP).c
$(APP)_INCDIRS   := $(SN_ROOT)/sw/benchmarks/matrix_mult/data

include $(SN_ROOT)/sw/kernels/common.mk