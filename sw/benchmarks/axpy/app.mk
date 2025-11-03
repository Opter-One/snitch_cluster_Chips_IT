APP              := axpy2
$(APP)_BUILD_DIR := $(SN_ROOT)/sw/benchmarks/axpy/build
SRCS             := $(SN_ROOT)/sw/benchmarks/axpy/src/$(APP).c
$(APP)_INCDIRS   := $(SN_ROOT)/sw/benchmarks/axpy/data

include $(SN_ROOT)/sw/kernels/common.mk