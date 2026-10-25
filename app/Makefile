# Based on the official switchbrew libnx application template.
.SUFFIXES:
ifeq ($(strip $(DEVKITPRO)),)
$(error Set DEVKITPRO, e.g. export DEVKITPRO=/opt/devkitpro)
endif
TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules
TARGET := SwitchTalk
BUILD := build
SOURCES := source
RTC := $(TOPDIR)/vendor/rtc
PEER := $(wildcard $(RTC)/libpeer-*)
INCLUDES := include vendor/rtc/$(notdir $(PEER))/src vendor/rtc/mbedtls-2.28.10/include
ARCH := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS := -g -O2 -std=gnu11 -Wall -Wextra -Wno-misleading-indentation -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS += $(INCLUDE) -D__SWITCH__
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,--wrap=sslConnectionDoHandshake -Wl,-Map,$(notdir $*.map)
LIBS := -lrtc-switch -lopus -lcurl -lz -lnx -lm
LIBDIRS := $(PORTLIBS) $(LIBNX)
export APP_TITLE := SwitchTalk
export APP_AUTHOR := Monster Review
export APP_VERSION := 1.0.1
ICON := icon.jpg
ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
CFILES := $(addsuffix .c,visibility json rich_text main talk tls messages network pcm mic_input audio_test call_state call_view gui server_url saved_login avatars avatar_http avatar_image cJSON call_http call_ws call_protocol call)
export LD := $(CC)
export OFILES := $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := -L$(RTC)/build $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export APP_ICON := $(TOPDIR)/$(ICON)
export NROFLAGS += --icon=$(APP_ICON) --nacp=$(CURDIR)/$(TARGET).nacp
.PHONY: $(BUILD) clean all
all: $(BUILD)
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
clean:
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf
else
.PHONY: all
DEPENDS := $(OFILES:.o=.d)
all: $(OUTPUT).nro
$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif
