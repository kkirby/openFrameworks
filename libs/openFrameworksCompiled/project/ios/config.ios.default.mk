# / --- Setup --- /

ifndef ABIS_TO_COMPILE_RELEASE
	ABIS_TO_COMPILE_RELEASE = armv7 arm64 i386.sim x86_64.sim
endif

ifndef ABIS_TO_COMPILE_DEBUG
	ABIS_TO_COMPILE_DEBUG = armv7 i386.sim x86_64.sim
endif

PLATFORM=iPhoneOS
ARCH = $(ABI)
ifeq ($(suffix $(ABI)),.sim)
	ARCH = $(basename $(ABI))
	PLATFORM=iPhoneSimulator
endif

PLATFORM_CORELIB_RELEASE_TARGET = $(OF_CORE_LIB_PATH)/$(ABI)/libopenFrameworks.a
PLATFORM_CORELIB_DEBUG_TARGET = $(OF_CORE_LIB_PATH)/$(ABI)/libopenFrameworksDebug.a

SYSROOT=/Applications/Xcode.app/Contents/Developer/Platforms/$(PLATFORM).platform/Developer/SDKs/$(PLATFORM).sdk

STANDARD_FLAGS = -isysroot $(SYSROOT) -arch $(ARCH) -fpic -miphoneos-version-min=8
CXX_ARGS = -std=c++11 -stdlib=libc++

ifndef IOS_FRAMEWORK_OUTPUT_PATH
	IOS_FRAMEWORK_OUTPUT_PATH = output/$(BIN_NAME)
endif

IOS_FRAMEWORK_OUTPUT = $(IOS_FRAMEWORK_OUTPUT_PATH)/$(APPNAME).framework

ifndef IOS_LIB_OUTPUT_PATH
	IOS_LIB_OUTPUT_PATH = lib
endif

# / --- Stuff --- /

PLATFORM_PROJECT_RELEASE_TARGET = $(IOS_LIB_OUTPUT_PATH)/$(ABI)/lib$(BIN_NAME).so
PLATFORM_PROJECT_DEBUG_TARGET = $(IOS_LIB_OUTPUT_PATH)/$(ABI)/lib$(BIN_NAME).so

PLATFORM_DEFINES = __MACOSX_CORE__
PLATFORM_REQUIRED_ADDONS = ofxiOS ofxAccelerometer

PLATFORM_CFLAGS += $(STANDARD_FLAGS) -fmessage-length=0 -fdiagnostics-show-note-include-stack -fmacro-backtrace-limit=0 -Wno-trigraphs -fpascal-strings -Wno-missing-field-initializers -Wno-missing-prototypes -Wno-implicit-atomic-properties -Wno-arc-repeated-use-of-weak -Wnon-virtual-dtor -Woverloaded-virtual -Wno-exit-time-destructors -Wno-missing-braces -Wparentheses -Wno-switch -Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-unused-variable -Wno-unused-value -Wno-empty-body -Wuninitialized -Wno-unknown-pragmas -Wno-shadow -Wno-four-char-constants -Wno-conversion -Wno-constant-conversion -Wno-int-conversion -Wno-bool-conversion -Wno-enum-conversion -Wno-shorten-64-to-32 -Wno-newline-eof -Wno-selector -Wno-strict-selector-match -Wno-undeclared-selector -Wno-deprecated-implementations -Wno-c++11-extensions -fstrict-aliasing -Wno-protocol -Wdeprecated-declarations -Wno-invalid-offsetof -Wno-sign-conversion -Wno-non-virtual-dtor -Wno-overloaded-virtual -Wno-c++11-narrowing
PLATFORM_CXXFLAGS += $(CXX_ARGS) -x objective-c++

PLATFORM_LDFLAGS = $(CXX_ARGS) $(STANDARD_FLAGS) -v -shared  -compatibility_version $(PROJECT_COMPATABILITY_VERSION) -current_version $(PROJECT_CURRENT_VERSION) -install_name @rpath/$(APPNAME).framework/$(APPNAME)

PLATFORM_OPTIMIZATION_CFLAGS_RELEASE = -O2 -fembed-bitcode
PLATFORM_OPTIMIZATION_CFLAGS_DEBUG = -g -O0 -fembed-bitcode-marker

PLATFORM_CORE_EXCLUSIONS =

# core sources
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/video/ofQtUtils.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/video/ofQuickTimeGrabber.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/video/ofQuickTimePlayer.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/video/ofDirectShowGrabber.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/video/ofDirectShowPlayer.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/video/ofQTKitGrabber.mm
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/video/ofGstUtils.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/video/ofGstVideoGrabber.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/video/ofGstVideoPlayer.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/app/ofAppGlutWindow.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/app/ofAppEGLWindow.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/app/ofAppGLFWWindow.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/graphics/ofCairoRenderer.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/sound/ofFmodSoundPlayer.cpp
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/openFrameworks/sound/ofOpenALSoundPlayer.cpp

# third party

PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/poco/include/CppUnit
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/poco/include/CppUnit/%
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/poco/include/Poco
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/poco/include/Poco/%
PLATFORM_CORE_EXCLUSIONS += $(OF_LIBS_PATH)/quicktime/%

PLATFORM_HEADER_SEARCH_PATHS += "$(OF_ROOT)/addons/ofxiOS/src"
PLATFORM_HEADER_SEARCH_PATHS += "$(OF_ROOT)/addons/ofxiOS/src/app"
PLATFORM_HEADER_SEARCH_PATHS += "$(OF_ROOT)/addons/ofxiOS/src/core"
PLATFORM_HEADER_SEARCH_PATHS += "$(OF_ROOT)/addons/ofxiOS/src/events"
PLATFORM_HEADER_SEARCH_PATHS += "$(OF_ROOT)/addons/ofxiOS/src/gl"
PLATFORM_HEADER_SEARCH_PATHS += "$(OF_ROOT)/addons/ofxiOS/src/sound"
PLATFORM_HEADER_SEARCH_PATHS += "$(OF_ROOT)/addons/ofxiOS/src/utils"
PLATFORM_HEADER_SEARCH_PATHS += "$(OF_ROOT)/addons/ofxiOS/src/video"

PLATFORM_LIBRARIES = 

PLATFORM_STATIC_LIBRARIES =

PLATFORM_SHARED_LIBRARIES = 

PLATFORM_LIBRARY_SEARCH_PATHS = 

PLATFORM_FRAMEWORKS =
PLATFORM_FRAMEWORKS += AVFoundation
PLATFORM_FRAMEWORKS += CoreMedia
PLATFORM_FRAMEWORKS += CoreVideo
PLATFORM_FRAMEWORKS += MapKit
PLATFORM_FRAMEWORKS += OpenAL
PLATFORM_FRAMEWORKS += CoreGraphics
PLATFORM_FRAMEWORKS += AudioToolbox
PLATFORM_FRAMEWORKS += QuartzCore
PLATFORM_FRAMEWORKS += OpenGLES
PLATFORM_FRAMEWORKS += UIKit
PLATFORM_FRAMEWORKS += Foundation
PLATFORM_FRAMEWORKS += CoreGraphics
PLATFORM_FRAMEWORKS += CoreLocation
PLATFORM_FRAMEWORKS += Accelerate

PLATFORM_FRAMEWORKS_SEARCH_PATHS = 

define PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleName</key>
	<string>$(APPNAME)</string>
	<key>CFBundleIdentifier</key>
	<string>$(PROJECT_BUNDLE_ID)</string>
	<key>CFBundleSupportedPlatforms</key>
	<array>
		<string>iPhoneSimulator</string>
		<string>iPhoneOS</string>
	</array>
	<key>CFBundleExecutable</key>
	<string>$(APPNAME)</string>
</dict>
</plist>
endef
export PLIST

afterplatform: $(TARGET_NAME)
	@rm -rf IOS_FRAMEWORK_OUTPUT
	@mkdir -p $(IOS_FRAMEWORK_OUTPUT)/Headers
	@xcrun lipo -create `find $(IOS_LIB_OUTPUT_PATH) -name "lib$(BIN_NAME).so"` -output $(IOS_FRAMEWORK_OUTPUT)/$(APPNAME)
	@echo $$PLIST > $(IOS_FRAMEWORK_OUTPUT)/Info.plist
	@echo Created framework at $(IOS_FRAMEWORK_OUTPUT)
	
	
