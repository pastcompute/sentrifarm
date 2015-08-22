#
# Copyright (C) 2013 Riccardo Ferrazzo <f.riccardo87@gmail.com>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
# 

include $(TOPDIR)/rules.mk

PKG_NAME:=qtserialport
PKG_VERSION:=5.1.1
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/qtproject/qtserialport.git
# PKG_SOURCE_VERSION:=58365228b3181ed03188fc8ff56007ea22678bb5 # V5.1 # 39f90528abda2f5ec7575acf2e1a352702a202e8 # v5.1.1
PKG_SOURCE_VERSION:=39f90528abda2f5ec7575acf2e1a352702a202e8 # v5.1.1
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.bz2
PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_USE_MIPS16:=0
PKG_BUILD_DEPENDS:=+qt5-core
CMAKE_INSTALL:=1

CMAKE_OPTIONS += -DCMAKE_FIND_ROOT_PATH=$(STAGING_DIR) \
                 -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=BOTH \
                 -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
                 -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
                 -DCMAKE_RUNTIME_OUTPUT_DIRECTORY="$(PKG_INSTALL_DIR)"

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

MAKE_FLAGS += VERBOSE=1

define 	Package/qtserialport
  CATEGORY:=Libraries
  SECTION:=libraries
  TITLE:=qtserialport
  DEPENDS:=+qt5-core
  URL:=https://github.com/qtproject/qtserialport.git
endef

define Build/Prepare
	$(call Build/Prepare/Default)
	$(CP) ./files/CMakeLists.txt $(PKG_BUILD_DIR)
	$(INSTALL_DIR) $(PKG_INSTALL_DIR)/usr/include/QtSerialPort
	$(CP) ./files/Q* $(PKG_INSTALL_DIR)/usr/include/QtSerialPort/
	$(CP) $(PKG_BUILD_DIR)/src/serialport/*.h $(PKG_INSTALL_DIR)/usr/include/QtSerialPort/
#	$(INSTALL_DIR) $(PKG_BUILD_DIR)/QtSerialPort
#	$(CP) -v ./files/Q* $(PKG_BUILD_DIR)/QtSerialPort/
#	$(CP) -v $(PKG_BUILD_DIR)/src/serialport/*.h $(PKG_BUILD_DIR)/QtSerialPort/
#	$(INSTALL_DIR) $(PKG_BUILD_DIR)/private
#	$(CP) -v $(PKG_BUILD_DIR)/src/serialport/qt4support/*p.h $(PKG_BUILD_DIR)/private
endef

define Build/InstallDev
	$(INSTALL_DIR) $(1)/usr/include/QtSerialPort
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_INSTALL_DIR)/usr/include/QtSerialPort/* \
		$(1)/usr/include/QtSerialPort/
	$(CP) \
		$(PKG_BUILD_DIR)/libqtserialport.so* \
		$(1)/usr/lib/
endef

define Package/qtserialport/install
	$(INSTALL_DIR) $(1)/usr/lib
	$(CP) \
		$(PKG_BUILD_DIR)/libqtserialport.so* \
		$(1)/usr/lib/
endef

$(eval $(call BuildPackage,qtserialport))
