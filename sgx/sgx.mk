SGX_DIR=$(BASE_DIR)/../sgx
KERNEL_DIR=$(BASE_DIR)/linux/
ANDROID_DIR=ignore_not_needed

sgx: kernel
	( cd $(SGX_DIR)/eurasiacon/build/linux/omap3430_android/kbuild ; \
		export ANDROID_ROOT=$(ANDROID_DIR); \
		export CROSS_COMPILE=arm-linux-; \
		export EURASIAROOT=${SGX_DIR}; \
		export KERNELDIR=${KERNEL_DIR}; \
	 	make package BUILD=release; \
	)

sgx-clean:
	( cd $(SGX_DIR)/eurasiacon/build/linux/omap3430_android/kbuild ; \
		export ANDROID_ROOT=$(ANDROID_DIR); \
		export CROSS_COMPILE=arm-linux-; \
		export EURASIAROOT=${SGX_DIR}; \
		export KERNELDIR=${KERNEL_DIR}; \
	 	make clean; \
		rm -f  $(SGX_DIR)/services4/srvkm/env/linux/kbuild/modules.order ;\
		rm -f  $(SGX_DIR)/services4/3rdparty/dc_omap3430_linux/kbuild/modules.order; \
		rm -rf $(SGX_DIR)/eurasiacon/binary_omap3430_android_release; \
	)

TARGETS	+= sgx
