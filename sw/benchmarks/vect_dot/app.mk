APP              := vect_dot
$(APP)_BUILD_DIR := $(SN_ROOT)/sw/benchmarks/$(APP)/build
SRCS             := $(SN_ROOT)/sw/benchmarks/$(APP)/src/$(APP).c
$(APP)_INCDIRS   := $(SN_ROOT)/sw/benchmarks/$(APP)/data

include $(SN_ROOT)/sw/kernels/common.mk