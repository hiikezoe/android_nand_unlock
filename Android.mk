LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  main.c           \
  diag.c

LOCAL_MODULE := nand_unlock

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
