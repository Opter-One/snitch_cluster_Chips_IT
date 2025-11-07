APP              := test_SSR
$(APP)_BUILD_DIR := $(SN_ROOT)/sw/benchmarks/test_SSR/build
SRCS             := $(SN_ROOT)/sw/benchmarks/test_SSR/src/$(APP).c
$(APP)_INCDIRS   := $(SN_ROOT)/sw/benchmarks/test_SSR/data

include $(SN_ROOT)/sw/kernels/common.mk