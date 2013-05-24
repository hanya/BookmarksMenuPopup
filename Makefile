
EXTENSION_VERSION=0.1.0
EXTENSION_STATE=
EXTENSION_ID=mytools.bookmarks.AnotherBookmarksPopup
EXTENSION_NAME=BookmarksPopup
EXTENSION_DISPLAY_NAME=Popup menu frontend for Bookmarks Menu

IMPLE_NAME=bookmarks.AnotherBookmarksPopup
MANAGER_FACTORY_IMPLE_NAME=bookmarks.BookmarksPopupManagerFactory

PRJ=$(OO_SDK_HOME)
SETTINGS=$(PRJ)/settings

include $(SETTINGS)/settings.mk
include $(SETTINGS)/std.mk
include $(SETTINGS)/dk.mk

ifeq "$(SDKVERSION)" "3.4"
VERSION_DEF=
else
VERSION_DEF=-DAOO4
endif

COMP_NAME=bkmkpmcpp
COMP_IMPL_NAME=$(COMP_NAME).uno.$(SHAREDLIB_EXT)


ifeq "$(OS)" "WIN"
ORIGIN_NAME=%%origin%%
CC_FLAGS+= /O2 
else
ORIGIN_NAME=%origin%
CC_FLAGS=-c -O -fpic
COMP_LINK_FLAGS=$(LIBRARY_LINK_FLAGS) 
ifeq "$(SDKVERSION)" "3.4"
CC_FLAGS+= -fvisibility=hidden
else
COMP_LINK_FLAGS+= -Wl,--version-script,$(SETTINGS)/component.uno.map 
endif
endif

SRC=./src
OUT=.
IDL_LOC_DIR=./idl
EXT_DIR=./ext
BUILD_DIR=./build

OUT_COMP_INC=$(OUT_INC)/$(COMP_NAME)
OUT_COMP_GEN=$(OUT_MISC)/$(COMP_NAME)
OUT_COMP_SLO=$(OUT_SLO)/$(COMP_NAME)


CXXFILES = bkpmc.cpp basepopup.cpp services.cpp executor.cpp bookmarks.cpp tools.cpp manager.cpp tagpmc.cpp
CFILES = parson.c

OBJFILES = $(patsubst %.cpp,$(OUT_SLO)/%.$(OBJ_EXT),$(CXXFILES))
COJBFILES = $(patsubst %.c,$(OUT_SLO)/%.$(OBJ_EXT),$(CFILES))

PARSON_INC=./parson
IDL_INC=../cww/inc
IDL_LOC_INC=./inc
CC_INCLUDES=-I. -I$(IDL_INC) -I$(IDL_LOC_INC) -I$(PRJ)/include -I$(PARSON_INC)

MANIFEST=$(BUILD_DIR)/META-INF/manifest.xml
DESCRIPTION=$(BUILD_DIR)/description.xml
COMP_DIR=$(BUILD_DIR)/libs
COMP_REGISTRATION=$(COMP_DIR)/registration.components

ifneq "$(PLATFORM)" "windows"
TAEGET_PLATFORM=$(PLATFORM)_x86
else
ifneq "$(PLATFORM)" "linux"
ifneq "$(PROCTYPE)" "i386"
TAEGET_PLATFORM=$(PLATFORM)_x86
else
ifneq "$(PROCTYPE)" "x86_64"
TAEGET_PLATFORM=$(PLATFORM)_x86_64
endif
endif
endif
endif

UNO_PKG_NAME=$(EXTENSION_NAME)-$(EXTENSION_VERSION)$(EXTENSION_STATE)-$(subst _,-,$(TAEGET_PLATFORM)).$(UNOOXT_EXT)

#CC=ccache gcc

.PHONY: ALL
ALL : \
	BookmarksMenuPopup

include $(SETTINGS)/stdtarget.mk

BookmarksMenuPopup : $(UNO_PKG_NAME)

$(UNO_PKG_NAME) : $(COMP_DIR)/$(COMP_IMPL_NAME) $(MANIFEST) $(DESCRIPTION) $(COMP_REGISTRATION) 
	$(COPY) README $(subst /,$(PS),$(BUILD_DIR)/README)
	$(COPY) LICENSE $(subst /,$(PS),$(BUILD_DIR)/LICENSE)
	$(COPY) Controller_top.xcu $(subst /,$(PS),$(BUILD_DIR)/Controller_top.xcu)
	$(COPY) base.json $(subst /,$(PS),$(BUILD_DIR)/base.json)
	$(COPY) -r $(EXT_DIR)/* $(BUILD_DIR)
	cd $(BUILD_DIR) && $(SDK_ZIP) -9 -r -o ../$(UNO_PKG_NAME) META-INF/manifest.xml README LICENSE description.xml libs/*.$(SHAREDLIB_EXT) libs/*.components descriptions/* Controller_top.xcu base.json

$(OUT_SLO)/%.$(OBJ_EXT) : $(SRC)/%.cpp $(SDKTYPEFLAG)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(CC) $(CC_OUTPUT_SWITCH)$(subst /,$(PS),$@) $(CC_FLAGS) $< $(CC_INCLUDES) $(CC_DEFINES) $(VERSION_DEF) 

$(OUT_SLO)/%.$(OBJ_EXT) : ./parson/%.c $(SDKTYPEFLAG)
	-$(MKDIR) $(subst /,$(PS),$(@D))
	$(CC) $(CC_OUTPUT_SWITCH)$(subst /,$(PS),$@) $(CC_FLAGS) $< $(CC_INCLUDES) $(CC_DEFINES) $(VERSION_DEF) -D_PTHREADS 


ifeq "$(OS)" "WIN"
LINK_OUT_FLAG=/OUT:
MATH_LIB=-lm
ADDITIONAL_LIBS=msvcrt.lib kernel32.lib
else
LINK_OUT_FLAG=-o 
MATH_LIB=
ADDITIONAL_LIBS=-Wl,--as-needed -ldl -lpthread -lm -Wl,--no-as-needed -Wl,-Bdynamic
endif

$(COMP_DIR)/$(COMP_IMPL_NAME) : $(OBJFILES) $(COJBFILES)
	-$(MKDIR) $(subst /,$(PS),$(COMP_DIR))
	$(LINK) $(LINK_OUT_FLAG)$(COMP_DIR)/$(COMP_IMPL_NAME) $(COMP_LINK_FLAGS) $(OBJFILES) $(COJBFILES) $(LINK_LIBS) $(SALLIB) $(SALDYLIB) $(SALHELPERLIB) $(CPPULIB) $(CPPUHELPERLIB) $(STC++LIB) $(CPPUHELPERDYLIB) $(CPPUDYLIB) $(ADDITIONAL_LIBS)

$(MANIFEST) : 
	@-$(MKDIR) $(subst /,$(PS),$(@D))
	@echo $(OSEP)?xml version="$(QM)1.0$(QM)" encoding="$(QM)UTF-8$(QM)"?$(CSEP) > $@
	@echo $(OSEP)manifest:manifest$(CSEP) >> $@
	@echo $(OSEP)manifest:file-entry manifest:full-path="$(QM)libs/registration.components$(QM)"  >> $@
	@echo manifest:media-type="$(QM)application/vnd.sun.star.uno-components;platform=$(UNOPKG_PLATFORM)$(QM)"/$(CSEP)  >> $@
	@echo $(OSEP)manifest:file-entry manifest:full-path="$(QM)Controller_top.xcu$(QM)" >> $@
	@echo manifest:media-type="$(QM)application/vnd.sun.star.configuration-data$(QM)"/$(CSEP) >> $@
	@echo $(OSEP)/manifest:manifest$(CSEP) >> $@

$(DESCRIPTION) : 
	@echo $(OSEP)?xml version="$(QM)1.0$(QM)" encoding="$(QM)UTF-8$(QM)"?$(CSEP) > $@
	@echo $(OSEP)description xmlns="$(QM)http://openoffice.org/extensions/description/2006$(QM)" >> $@
	@echo xmlns:xlink="$(QM)http://www.w3.org/1999/xlink$(QM)" >> $@
	@echo xmlns:d="$(QM)http://openoffice.org/extensions/description/2006$(QM)"$(CSEP) >> $@
	@echo $(OSEP)identifier value="$(QM)$(EXTENSION_ID)$(QM)" /$(CSEP) >> $@
	@echo $(OSEP)dependencies$(CSEP) >> $@
	@echo $(OSEP)OpenOffice.org-minimal-version value="$(QM)3.4$(QM)" d:name="$(QM)OpenOffice.org 3.4$(QM)" /$(CSEP) >> $@
	@echo $(OSEP)/dependencies$(CSEP) >> $@
	@echo $(OSEP)version value="$(QM)$(EXTENSION_VERSION)$(QM)" /$(CSEP) >> $@
	@echo $(OSEP)registration$(CSEP) >> $@
	@echo $(OSEP)simple-license accept-by="$(QM)admin$(QM)" default-license-id="$(QM)this$(QM)" suppress-on-update="$(QM)true$(QM)"$(CSEP) >> $@
	@echo $(OSEP)license-text xlink:href="$(QM)LICENSE$(QM)" license-id="$(QM)this$(QM)" /$(CSEP) >> $@
	@echo $(OSEP)/simple-license$(CSEP) >> $@
	@echo $(OSEP)/registration$(CSEP) >> $@
	@echo $(OSEP)platform value="$(QM)$(TAEGET_PLATFORM)$(QM)" /$(CSEP) >> $@
	@echo $(OSEP)display-name$(CSEP) >> $@
	@echo $(OSEP)name lang="$(QM)en$(QM)"$(CSEP)$(EXTENSION_DISPLAY_NAME)$(OSEP)/name$(CSEP) >> $@
	@echo $(OSEP)/display-name$(CSEP) >> $@
	@echo $(OSEP)extension-description$(CSEP) >> $@
	@echo $(OSEP)src lang="$(QM)en$(QM)" xlink:href="$(QM)descriptions/desc.en$(QM)"/$(CSEP) >> $@
	@echo $(OSEP)/extension-description$(CSEP) >> $@
	@echo $(OSEP)/description$(CSEP) >> $@

$(COMP_REGISTRATION) : 
	@echo $(OSEP)?xml version="$(QM)1.0$(QM)" encoding="$(QM)UTF-8$(QM)"?$(CSEP) >> $@
	@echo $(OSEP)components xmlns="$(QM)http://openoffice.org/2010/uno-components$(QM)"$(CSEP) >> $@
	@echo $(OSEP)component loader="$(QM)com.sun.star.loader.SharedLibrary$(QM)" uri="$(QM)$(COMP_IMPL_NAME)$(QM)"$(CSEP) >> $@
	@echo $(OSEP)implementation name="$(QM)$(IMPLE_NAME)$(QM)"$(CSEP) >> $@
	@echo $(OSEP)service name="$(QM)com.sun.star.frame.PopupMenuController$(QM)"/$(CSEP) >> $@
	@echo $(OSEP)service name="$(QM)$(IMPLE_NAME)$(QM)"/$(CSEP) >> $@
	@echo $(OSEP)/implementation$(CSEP) >> $@
	@echo $(OSEP)implementation name="$(QM)$(MANAGER_FACTORY_IMPLE_NAME)$(QM)"$(CSEP) >> $@
	@echo $(OSEP)service name="$(QM)$(MANAGER_FACTORY_IMPLE_NAME)$(QM)"/$(CSEP) >> $@
	@echo $(OSEP)/implementation$(CSEP) >> $@
	@echo $(OSEP)/component$(CSEP) >> $@
	@echo $(OSEP)/components$(CSEP) >> $@

clean : 
	@- $(RM) $(subst \\,\,$(subst /,$(PS),$(MANIFEST)))
	@- $(RM) $(subst \\,\,$(subst /,$(PS),$(DESCRIPTION)))
	@- $(DELRECURSIVE) $(subst \,$(PS),$(OUT_SLO))
	@- $(RM) $(subst \\,\,$(subst /,$(PS),$(COMP_DIR)/$(COMP_IMPL_NAME)))
	@- $(RM) $(subst /,$(PS),$(BUILD_DIR)/LICENSE)
	@- $(RM) $(subst /,$(PS),$(BUILD_DIR)/README)
	@- $(RM) $(UNO_PKG_NAME)
#	@- $(DELRECURSIVE) $(BUILD_DIR)

