LOCAL_PATH := $(call my-dir)/../


include $(CLEAR_VARS)

LOCAL_MODULE := doom3_rb


LOCAL_C_INCLUDES :=  \
$(TOP_DIR) \
$(TOP_DIR)/Doom/d3es/neo/mobile \
$(SDL_INCLUDE_PATHS) \
$(TOP_DIR)/AudioLibs_OpenTouch/openal/include/ \
$(TOP_DIR)/AudioLibs_OpenTouch/liboggvorbis/include \
$(TOP_DIR)/MobileTouchControls \
$(TOP_DIR)/Clibs_OpenTouch \
$(TOP_DIR)/Clibs_OpenTouch/idtech1 \


LOCAL_CPPFLAGS += -std=gnu++11 -D__DOOM_DLL__ -frtti -fexceptions  -Wno-error=format-security


LOCAL_CPPFLAGS += -Wno-sign-compare \
                  -Wno-switch \
                  -Wno-format-security \

LOCAL_CPPFLAGS +=

LOCAL_CPPFLAGS += -DNO_LIGHT

# Not avaliable in Android until N
LOCAL_CFLAGS := -DIOAPI_NO_64

LOCAL_CFLAGS += -fno-unsafe-math-optimizations -fno-strict-aliasing -fno-math-errno -fno-trapping-math -fomit-frame-pointer -fvisibility=hidden -fsigned-char


SRC_ANDROID = mobile/game_interface.cpp \
              ../../../Clibs_OpenTouch/idtech1/android_jni.cpp \
              ../../../Clibs_OpenTouch/idtech1/touch_interface.cpp \


src_renderer = \
	renderer/draw_gles2.cpp \
	renderer/draw_common.cpp \
	renderer/tr_backend.cpp \
	renderer/tr_render.cpp \
	renderer/qgl.cpp


src_renderer_glsl = \
    renderer/glsl/cubeMapShaderFP.cpp \
    renderer/glsl/diffuseMapShaderVP.cpp \
    renderer/glsl/diffuseCubeShaderVP.cpp \
    renderer/glsl/diffuseMapShaderFP.cpp \
    renderer/glsl/fogShaderFP.cpp \
    renderer/glsl/fogShaderVP.cpp \
    renderer/glsl/blendLightShaderVP.cpp \
    renderer/glsl/interactionPhongShaderFP.cpp \
    renderer/glsl/interactionPhongShaderVP.cpp \
    renderer/glsl/interactionShaderFP.cpp \
    renderer/glsl/interactionShaderVP.cpp \
    renderer/glsl/reflectionCubeShaderVP.cpp \
    renderer/glsl/skyboxCubeShaderVP.cpp \
    renderer/glsl/stencilShadowShaderFP.cpp \
    renderer/glsl/stencilShadowShaderVP.cpp \
    renderer/glsl/zfillClipShaderFP.cpp \
    renderer/glsl/zfillClipShaderVP.cpp \
    renderer/glsl/zfillShaderFP.cpp \
    renderer/glsl/zfillShaderVP.cpp \


src_core = \
    	${src_renderer} \
    	$(src_renderer_glsl)


LOCAL_SRC_FILES = $(src_core)





LOCAL_SHARED_LIBRARIES :=

LOCAL_STATIC_LIBRARIES :=

LOCAL_LDLIBS :=  -llog -lm  -lEGL -lGLESv2 -lz
include $(BUILD_SHARED_LIBRARY)
