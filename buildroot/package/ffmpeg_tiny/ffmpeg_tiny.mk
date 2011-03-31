ifeq ($(ARCH),arm)
#############################################################
#
# ffmpeg_tiny
#
#############################################################
FFMPEG_TINY_DIR:=$(BUILD_DIR)/ffmpeg_tiny

FFMPEG_TINY_SRC=ffmpeg_r18839.tgz

FFMPEG_TINY_LIBAVCODEC_REV=52
FFMPEG_TINY_LIBAVFORMAT_REV=52
FFMPEG_TINY_LIBAVUTIL_REV=50

#$(DL_DIR)/ffmpeg_tiny:
#	(cd $(DL_DIR) ;\
#	svn checkout svn://svn.mplayerhq.hu/ffmpeg/trunk ffmpeg )

$(FFMPEG_TINY_DIR)/.unpacked:
	(cd $(BUILD_DIR) ; \
	tar -xvzf $(DL_DIR)/$(FFMPEG_TINY_SRC) ; \
	mv ffmpeg $(FFMPEG_TINY_DIR) )
	toolchain/patch-kernel.sh $(FFMPEG_TINY_DIR) package/ffmpeg_tiny/ \*.patch
	touch $@

$(FFMPEG_TINY_DIR)/.configured: $(FFMPEG_TINY_DIR)/.unpacked
	(cd $(FFMPEG_TINY_DIR); \
	$(TARGET_CONFIGURE_OPTS) \
	$(TARGET_CONFIGURE_ARGS) \
	./configure \
	--arch=$(ARCH) \
	--cc=$(TARGET_CC) \
	--enable-cross-compile \
	--extra-cflags="-fPIC -DPIC" \
	--prefix=/usr  \
	--libdir=/usr/lib \
	--enable-shared \
	--disable-bzlib \
	--disable-sse \
	--disable-ffmpeg --disable-ffplay --disable-ffserver \
	--disable-libfaad --disable-libfaac \
	--disable-muxers \
	--disable-demuxers \
	--disable-parsers \
	--disable-bsfs \
	--disable-protocols \
	--disable-devices \
	--disable-filters \
	--disable-encoders \
	--disable-decoders \
 	--enable-decoder=cook \
 	--enable-decoder=flac \
 	--enable-demuxer=avi \
 	--enable-demuxer=matroska \
 	--enable-protocol=file \
 	--enable-protocol=http \
 	--disable-static \
	--disable-mmx \
	--disable-stripping \
	--enable-memalign-hack );
	touch $@

$(FFMPEG_TINY_DIR)/.compiled: $(FFMPEG_TINY_DIR)/.configured
	$(MAKE) -C $(FFMPEG_TINY_DIR)
	touch $@

$(TARGET_DIR)/usr/lib/libavcodec.so.$(FFMPEG_TINY_LIBAVCODEC_REV): $(FFMPEG_TINY_DIR)/.compiled
	cp $(FFMPEG_TINY_DIR)/libavcodec/libavcodec.so.$(FFMPEG_TINY_LIBAVCODEC_REV) $@
	rm -f $(TARGET_DIR)/usr/lib/libavcodec.so && ln -s $(@F) $(TARGET_DIR)/usr/lib/libavcodec.so

$(TARGET_DIR)/usr/lib/libavformat.so.$(FFMPEG_TINY_LIBAVFORMAT_REV): $(FFMPEG_TINY_DIR)/.compiled
	cp $(FFMPEG_TINY_DIR)/libavformat/libavformat.so.$(FFMPEG_TINY_LIBAVFORMAT_REV) $@
	rm -f $(TARGET_DIR)/usr/lib/libavformat.so && ln -s $(@F) $(TARGET_DIR)/usr/lib/libavformat.so

$(TARGET_DIR)/usr/lib/libavutil.so.$(FFMPEG_TINY_LIBAVUTIL_REV): $(FFMPEG_TINY_DIR)/.compiled
	cp $(FFMPEG_TINY_DIR)/libavutil/libavutil.so.$(FFMPEG_TINY_LIBAVUTIL_REV) $@
	rm -f $(TARGET_DIR)/usr/lib/libavutil.so && ln -s $(@F) $(TARGET_DIR)/usr/lib/libavutil.so

$(FFMPEG_TINY_DIR)/.installed: $(TARGET_DIR)/usr/lib/libavformat.so.$(FFMPEG_TINY_LIBAVFORMAT_REV) $(TARGET_DIR)/usr/lib/libavcodec.so.$(FFMPEG_TINY_LIBAVCODEC_REV) $(TARGET_DIR)/usr/lib/libavutil.so.$(FFMPEG_TINY_LIBAVUTIL_REV)
	DESTDIR=$(STAGING_DIR) $(MAKE) -C $(FFMPEG_TINY_DIR) install
	touch $@

ffmpeg_tiny: uclibc $(FFMPEG_TINY_DIR)/.installed 

ffmpeg_tiny-clean:
	rm -f $(STAGING_DIR)/usr/liblibav*
	rm -f $(TARGET_DIR)/usr/lib/libav*
	rm -rf $(STAGING_DIR)/usr/include/ffmpeg
	-$(MAKE) -C $(FFMPEG_TINY_DIR) clean

ffmpeg_tiny-dirclean:
	rm -rf $(FFMPEG_TINY_DIR)

#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(strip $(BR2_PACKAGE_FFMPEG_TINY)),y)
TARGETS+=ffmpeg_tiny
endif
endif
