LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES :=  \
  main.c            \
  shlcdc_mmap.c

LOCAL_MODULE := nand_unlock

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_LIBRARIES := libdiagexploit

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
