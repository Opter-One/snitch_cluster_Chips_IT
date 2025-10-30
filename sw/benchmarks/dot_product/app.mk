APP              := bench_dot
$(APP)_BUILD_DIR := $(SN_ROOT)/sw/benchmarks/dot_product/build
SRCS             := $(SN_ROOT)/sw/benchmarks/dot_product/src/$(APP).c
$(APP)_INCDIRS   := $(SN_ROOT)/sw/benchmarks/dot_product/data

include $(SN_ROOT)/sw/kernels/common.mk