ifeq ($(ARCH),i586)
FFMPEG_VERSION=
FFMPEG_SOURCE=ffmpeg_r18839.tgz
FFMPEG_CAT:=$(ZCAT)
FFMPEG_SITE=http://$(BR2_SOURCEFORGE_MIRROR).dl.sourceforge.net/sourceforge/netcat
FFMPEG_DIR:=$(BUILD_DIR)/ffmpeg
FFMPEG_BINARY=ffmpeg
FFMPEG_TARGET_BINARY=ffmpeg

FFMPEG_LIBAVCODEC_REV=52
FFMPEG_LIBAVFORMAT_REV=52
FFMPEG_LIBAVUTIL_REV=50

$(DL_DIR)/$(FFMPEG_SOURCE):
	$(WGET) -P $(DL_DIR) $(FFMPEG_SITE)/$(FFMPEG_SOURCE)

ffmpeg-source: $(DL_DIR)/$(FFMPEG_SOURCE)

$(FFMPEG_DIR)/.unpacked: $(DL_DIR)/$(FFMPEG_SOURCE)
	$(FFMPEG_CAT) $(DL_DIR)/$(FFMPEG_SOURCE) | tar -C $(BUILD_DIR) $(TAR_OPTIONS) -
	toolchain/patch-kernel.sh $(FFMPEG_DIR) package/ffmpeg/ \*.patch
	touch $@

$(FFMPEG_DIR)/.configured: $(FFMPEG_DIR)/.unpacked
	(cd $(FFMPEG_DIR); rm -rf config.cache; \
		$(TARGET_CONFIGURE_OPTS) \
		$(TARGET_CONFIGURE_ARGS) \
		./configure \
		--arch=$(ARCH) \
		--cc=$(TARGET_CC) \
		--enable-cross-compile \
		--prefix=/usr  \
		--mandir=/usr/share/man \
		--libdir=/usr/lib \
		--enable-gpl  \
		--enable-shared \
		--disable-bzlib \
		--disable-sse \
		--enable-nonfree \
		--disable-ffmpeg --disable-ffplay --disable-ffserver \
		--enable-libfaad --enable-libfaac \
	)
	touch $@

$(FFMPEG_DIR)/.compiled: $(FFMPEG_DIR)/.configured
	$(MAKE) -C $(FFMPEG_DIR)
	touch $@

$(TARGET_DIR)/$(FFMPEG_TARGET_BINARY): $(FFMPEG_DIR)/$(FFMPEG_BINARY)
	#install -D $(FFMPEG_DIR)/$(FFMPEG_BINARY) $(TARGET_DIR)/$(FFMPEG_TARGET_BINARY)
	#$(STRIPCMD) $(STRIP_STRIP_ALL) $@

$(TARGET_DIR)/usr/lib/libavcodec.so.$(FFMPEG_LIBAVCODEC_REV): $(FFMPEG_DIR)/.compiled
	cp $(FFMPEG_DIR)/libavcodec/libavcodec.so.$(FFMPEG_LIBAVCODEC_REV) $@
	rm -f $(TARGET_DIR)/usr/lib/libavcodec.so && ln -s $(@F) $(TARGET_DIR)/usr/lib/libavcodec.so

$(TARGET_DIR)/usr/lib/libavformat.so.$(FFMPEG_LIBAVFORMAT_REV): $(FFMPEG_DIR)/.compiled
	cp $(FFMPEG_DIR)/libavformat/libavformat.so.$(FFMPEG_LIBAVFORMAT_REV) $@
	rm -f $(TARGET_DIR)/usr/lib/libavformat.so && ln -s $(@F) $(TARGET_DIR)/usr/lib/libavformat.so

$(TARGET_DIR)/usr/lib/libavutil.so.$(FFMPEG_LIBAVUTIL_REV): $(FFMPEG_DIR)/.compiled
	cp $(FFMPEG_DIR)/libavutil/libavutil.so.$(FFMPEG_LIBAVUTIL_REV) $@
	rm -f $(TARGET_DIR)/usr/lib/libavutil.so && ln -s $(@F) $(TARGET_DIR)/usr/lib/libavutil.so

$(FFMPEG_DIR)/.installed: $(TARGET_DIR)/usr/lib/libavcodec.so.$(FFMPEG_LIBAVCODEC_REV) $(TARGET_DIR)/usr/lib/libavformat.so.$(FFMPEG_LIBAVFORMAT_REV) $(TARGET_DIR)/usr/lib/libavutil.so.$(FFMPEG_LIBAVUTIL_REV)
	DESTDIR=$(STAGING_DIR) $(MAKE) -C $(FFMPEG_DIR) install
	touch $@

ffmpeg: uclibc zlib libfaac libfaad2 $(FFMPEG_DIR)/.installed

ffmpeg-clean:
	rm -f $(TARGET_DIR)/$(FFMPEG_TARGET_BINARY)
	-$(MAKE) -C $(FFMPEG_DIR) clean

ffmpeg-dirclean:
	rm -rf $(FFMPEG_DIR)
#############################################################
#
# Toplevel Makefile options
#
#############################################################
ifeq ($(strip $(BR2_PACKAGE_FFMPEG)),y)
TARGETS+=ffmpeg
endif
endif
