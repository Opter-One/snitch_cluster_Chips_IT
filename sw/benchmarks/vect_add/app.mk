APP              := vect_add
$(APP)_BUILD_DIR := $(SN_ROOT)/sw/benchmarks/vect_add/build
SRCS             := $(SN_ROOT)/sw/benchmarks/vect_add/src/$(APP).c
$(APP)_INCDIRS   := $(SN_ROOT)/sw/benchmarks/vect_add/data

include $(SN_ROOT)/sw/kernels/common.mk