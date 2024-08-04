# created by xmake

cmake_minimum_required(VERSION 3.21)
project(y)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)
set(SOURCE
	src/audio/Sound.cpp
	src/audio/SoundAL.cpp
	src/fx/Beam.cpp
	src/fx/Particle.cpp
	src/fx/ParticleEmitter.cpp
	src/fx/ParticleManager.cpp
	src/gui/Font.cpp
	src/gui/gui.cpp
	src/gui/Node.cpp
	src/gui/Picture.cpp
	src/gui/Text.cpp
	src/helper/DeletionQueue.cpp
	src/helper/ErrorHandler.cpp
	src/helper/PerformanceMonitor.cpp
	src/helper/ResourceManager.cpp
	src/helper/Scheduler.cpp
	src/helper/TimeTable.cpp
	src/input/Gamepad.cpp
	src/input/InputManager.cpp
	src/input/Keyboard.cpp
	src/input/Mouse.cpp
	src/lib/any/any.cpp
	src/lib/base/array.cpp
	src/lib/base/pointer.cpp
	src/lib/base/strings.cpp
	src/lib/doc/chunked.cpp
	src/lib/doc/pdf.cpp
	src/lib/doc/ttf.cpp
	src/lib/doc/xml.cpp
	src/lib/fft/fft.cpp
	src/lib/hui_minimal/Application.cpp
	src/lib/hui_minimal/Callback.cpp
	src/lib/hui_minimal/error.cpp
	src/lib/hui_minimal/hui.cpp
	src/lib/image/color.cpp
	src/lib/image/image.cpp
	src/lib/image/image_bmp.cpp
	src/lib/image/image_jpg.cpp
	src/lib/image/image_png.cpp
	src/lib/image/image_tga.cpp
	src/lib/image/ImagePainter.cpp
	src/lib/kaba/asm/asm.cpp
	src/lib/kaba/asm/asm_arm.cpp
	src/lib/kaba/asm/asm_x86.cpp
	src/lib/kaba/compiler/Backend.cpp
	src/lib/kaba/compiler/BackendAmd64.cpp
	src/lib/kaba/compiler/BackendARM.cpp
	src/lib/kaba/compiler/BackendArm64.cpp
	src/lib/kaba/compiler/BackendX86.cpp
	src/lib/kaba/compiler/CommandList.cpp
	src/lib/kaba/compiler/Compiler.cpp
	src/lib/kaba/compiler/mapper.cpp
	src/lib/kaba/compiler/Serializer.cpp
	src/lib/kaba/compiler/SerialNode.cpp
	src/lib/kaba/dynamic/call.cpp
	src/lib/kaba/dynamic/dynamic.cpp
	src/lib/kaba/dynamic/exception.cpp
	src/lib/kaba/dynamic/sorting.cpp
	src/lib/kaba/lib/dict.cpp
	src/lib/kaba/lib/extern.cpp
	src/lib/kaba/lib/lib.cpp
	src/lib/kaba/lib/lib_base.cpp
	src/lib/kaba/lib/lib_doc.cpp
	src/lib/kaba/lib/lib_gl.cpp
	src/lib/kaba/lib/lib_hui.cpp
	src/lib/kaba/lib/lib_image.cpp
	src/lib/kaba/lib/lib_kaba.cpp
	src/lib/kaba/lib/lib_math.cpp
	src/lib/kaba/lib/lib_net.cpp
	src/lib/kaba/lib/lib_os.cpp
	src/lib/kaba/lib/lib_threads.cpp
	src/lib/kaba/lib/lib_time.cpp
	src/lib/kaba/lib/lib_vulkan.cpp
	src/lib/kaba/lib/list.cpp
	src/lib/kaba/parser/abstract.cpp
	src/lib/kaba/parser/Concretifier.cpp
	src/lib/kaba/parser/import.cpp
	src/lib/kaba/parser/lexical.cpp
	src/lib/kaba/parser/macros.cpp
	src/lib/kaba/parser/Parser.cpp
	src/lib/kaba/parser/Transformer.cpp
	src/lib/kaba/parser/type_matching.cpp
	src/lib/kaba/syntax/Block.cpp
	src/lib/kaba/syntax/Class.cpp
	src/lib/kaba/syntax/Constant.cpp
	src/lib/kaba/syntax/Function.cpp
	src/lib/kaba/syntax/Identifier.cpp
	src/lib/kaba/syntax/Node.cpp
	src/lib/kaba/syntax/Operator.cpp
	src/lib/kaba/syntax/preprocessor.cpp
	src/lib/kaba/syntax/Statement.cpp
	src/lib/kaba/syntax/SyntaxTree.cpp
	src/lib/kaba/syntax/Variable.cpp
	src/lib/kaba/template/implicit.cpp
	src/lib/kaba/template/implicit_array.cpp
	src/lib/kaba/template/implicit_callable.cpp
	src/lib/kaba/template/implicit_dict.cpp
	src/lib/kaba/template/implicit_enum.cpp
	src/lib/kaba/template/implicit_future.cpp
	src/lib/kaba/template/implicit_list.cpp
	src/lib/kaba/template/implicit_optional.cpp
	src/lib/kaba/template/implicit_pointer.cpp
	src/lib/kaba/template/implicit_product.cpp
	src/lib/kaba/template/implicit_regular.cpp
	src/lib/kaba/template/template.cpp
	src/lib/kaba/CompilerConfiguration.cpp
	src/lib/kaba/Context.cpp
	src/lib/kaba/Interpreter.cpp
	src/lib/kaba/kaba.cpp
	src/lib/kaba/Module.cpp
	src/lib/math/complex.cpp
	src/lib/math/interpolation.cpp
	src/lib/math/mat3.cpp
	src/lib/math/mat4.cpp
	src/lib/math/math.cpp
	src/lib/math/plane.cpp
	src/lib/math/quaternion.cpp
	src/lib/math/random.cpp
	src/lib/math/ray.cpp
	src/lib/math/rect.cpp
	src/lib/math/vec2.cpp
	src/lib/math/vec3.cpp
	src/lib/math/vec4.cpp
	src/lib/net/BinaryBuffer.cpp
	src/lib/net/net.cpp
	src/lib/net/Socket.cpp
	src/lib/nix/nix.cpp
	src/lib/nix/nix_buffer.cpp
	src/lib/nix/nix_draw.cpp
	src/lib/nix/nix_framebuffer.cpp
	src/lib/nix/nix_light.cpp
	src/lib/nix/nix_shader.cpp
	src/lib/nix/nix_textures.cpp
	src/lib/nix/nix_vertexbuffer.cpp
	src/lib/nix/nix_view.cpp
	src/lib/os/CommandLineParser.cpp
	src/lib/os/config.cpp
	src/lib/os/date.cpp
	src/lib/os/file.cpp
	src/lib/os/filesystem.cpp
	src/lib/os/formatter.cpp
	src/lib/os/msg.cpp
	src/lib/os/path.cpp
	src/lib/os/stream.cpp
	src/lib/os/terminal.cpp
	src/lib/os/time.cpp
	src/lib/vulkan/AccelerationStructure.cpp
	src/lib/vulkan/Buffer.cpp
	src/lib/vulkan/CommandBuffer.cpp
	src/lib/vulkan/DescriptorSet.cpp
	src/lib/vulkan/Device.cpp
	src/lib/vulkan/FrameBuffer.cpp
	src/lib/vulkan/helper.cpp
	src/lib/vulkan/Instance.cpp
	src/lib/vulkan/Pipeline.cpp
	src/lib/vulkan/Queue.cpp
	src/lib/vulkan/RenderPass.cpp
	src/lib/vulkan/Semaphore.cpp
	src/lib/vulkan/Shader.cpp
	src/lib/vulkan/SwapChain.cpp
	src/lib/vulkan/Texture.cpp
	src/lib/vulkan/VertexBuffer.cpp
	src/lib/vulkan/vulkan.cpp
	src/net/NetworkManager.cpp
	src/plugins/Controller.cpp
	src/plugins/PluginManager.cpp
	src/renderer/gui/GuiRendererGL.cpp
	src/renderer/gui/GuiRendererVulkan.cpp
	src/renderer/helper/jitter.cpp
	src/renderer/helper/Pipeline.cpp
	src/renderer/helper/PipelineManager.cpp
	src/renderer/helper/RendererFactory.cpp
	src/renderer/post/blur.cpp
	src/renderer/post/HDRRendererGL.cpp
	src/renderer/post/HDRRendererVulkan.cpp
	src/renderer/post/PostProcessor.cpp
	src/renderer/post/PostProcessorGL.cpp
	src/renderer/post/PostProcessorVulkan.cpp
	src/renderer/regions/RegionRendererCommon.cpp
	src/renderer/regions/RegionRendererGL.cpp
	src/renderer/regions/RegionRendererVulkan.cpp
	src/renderer/target/TargetRenderer.cpp
	src/renderer/target/TextureRendererGL.cpp
	src/renderer/target/WindowRendererGL.cpp
	src/renderer/target/WindowRendererVulkan.cpp
	src/renderer/world/geometry/GeometryRenderer.cpp
	src/renderer/world/geometry/GeometryRendererGL.cpp
	src/renderer/world/geometry/GeometryRendererVulkan.cpp
	src/renderer/world/geometry/SceneView.cpp
	src/renderer/world/pass/ShadowRendererGL.cpp
	src/renderer/world/pass/ShadowRendererVulkan.cpp
	src/renderer/world/WorldRenderer.cpp
	src/renderer/world/WorldRendererGL.cpp
	src/renderer/world/WorldRendererGLDeferred.cpp
	src/renderer/world/WorldRendererGLForward.cpp
	src/renderer/world/WorldRendererVulkan.cpp
	src/renderer/world/WorldRendererVulkanForward.cpp
	src/renderer/world/WorldRendererVulkanRayTracing.cpp
	src/renderer/base.cpp
	src/renderer/Renderer.cpp
	src/world/components/Animator.cpp
	src/world/components/Collider.cpp
	src/world/components/MultiInstance.cpp
	src/world/components/Skeleton.cpp
	src/world/components/SolidBody.cpp
	src/world/components/UserMesh.cpp
	src/world/Camera.cpp
	src/world/LevelData.cpp
	src/world/Light.cpp
	src/world/Link.cpp
	src/world/Material.cpp
	src/world/Model.cpp
	src/world/ModelManager.cpp
	src/world/Object.cpp
	src/world/Terrain.cpp
	src/world/World.cpp
	src/y/BaseClass.cpp
	src/y/Component.cpp
	src/y/ComponentManager.cpp
	src/y/EngineData.cpp
	src/y/Entity.cpp
	src/Config.cpp
	src/main.cpp
)

add_executable(y ${SOURCE})
target_include_directories(y PRIVATE src)

find_package(PkgConfig REQUIRED)


# lib: zlib mandatory
pkg_check_modules(ZLIB REQUIRED zlib)
target_include_directories(y PUBLIC ${ZLIB_INCLUDE_DIRS})
target_link_directories(y PUBLIC ${ZLIB_LIBRARY_DIRS})
list(APPEND DEPS ${ZLIB_LIBRARIES})
target_compile_options(y PUBLIC ${ZLIB_CFLAGS_OTHER})
target_compile_definitions(y PUBLIC HAS_LIB_ZLIB=1)

# lib: glfw mandatory
pkg_check_modules(GLFW REQUIRED glfw3)
target_include_directories(y PUBLIC ${GLFW_INCLUDE_DIRS})
target_link_directories(y PUBLIC ${GLFW_LIBRARY_DIRS})
list(APPEND DEPS ${GLFW_LIBRARIES})
target_compile_options(y PUBLIC ${GLFW_CFLAGS_OTHER})
target_compile_definitions(y PUBLIC HAS_LIB_GLFW=1)

# lib: gl or vulkan (mandatory)
set(GL_OR_VULKAN "gl" CACHE STRING "Use gl or vulkan?")
pkg_check_modules(GL gl)
if(GL_FOUND)
	list(APPEND GL_OR_VULKAN_FOUND "gl")
endif()
pkg_check_modules(VULKAN vulkan)
if(VULKAN_FOUND)
	list(APPEND GL_OR_VULKAN_FOUND "vulkan")
endif()
if(GL_OR_VULKAN_FOUND)
	if(NOT ${GTK3_OR_GTK4} IN_LIST GL_OR_VULKAN_FOUND)
		list(GET GL_OR_VULKAN_FOUND 0 X)
		set(GL_OR_VULKAN ${X} CACHE STRING "choose" FORCE)
		message(WARNING "Choice for GL_OR_VULKAN invalid, using ${GL_OR_VULKAN} instead")
	endif()
else()
	message(SEND_ERROR "Neither gl or vulkan found") #  or/nor..
	set(GL_OR_VULKAN "")
endif()
if(GL_OR_VULKAN STREQUAL "gl")
	target_include_directories(y PUBLIC ${GL_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${GL_LIBRARY_DIRS})
	list(APPEND DEPS ${GL_LIBRARIES})
	target_compile_options(y PUBLIC ${GL_CFLAGS_OTHER})
	target_compile_definitions(y PUBLIC HAS_LIB_GL=1)
endif()
if(GL_OR_VULKAN STREQUAL "vulkan")
	target_include_directories(y PUBLIC ${VULKAN_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${VULKAN_LIBRARY_DIRS})
	list(APPEND DEPS ${VULKAN_LIBRARIES})
	target_compile_options(y PUBLIC ${VULKAN_CFLAGS_OTHER})
	target_compile_definitions(y PUBLIC HAS_LIB_VULKAN=1)
endif()

# lib: freetype2 mandatory
pkg_check_modules(FREETYPE2 REQUIRED freetype2)
target_include_directories(y PUBLIC ${FREETYPE2_INCLUDE_DIRS})
target_link_directories(y PUBLIC ${FREETYPE2_LIBRARY_DIRS})
list(APPEND DEPS ${FREETYPE2_LIBRARIES})
target_compile_options(y PUBLIC ${FREETYPE2_CFLAGS_OTHER})
target_compile_definitions(y PUBLIC HAS_LIB_FREETYPE2=1)

# lib: unwind (optional)
set(LIB_UNWIND_ENABLED true CACHE BOOL "Use unwind library?")
pkg_check_modules(LIBUNWIND libunwind)
if(LIBUNWIND_FOUND)
	target_include_directories(y PUBLIC ${LIBUNWIND_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${LIBUNWIND_LIBRARY_DIRS})
	list(APPEND DEPS ${LIBUNWIND_LIBRARIES})
	target_compile_options(y PUBLIC ${LIBUNWIND_CFLAGS_OTHER})
else()
	set(LIB_UNWIND_ENABLED false CACHE BOOL "unwind library not found" FORCE)
endif()
if(${LIB_UNWIND_ENABLED})
	target_compile_definitions(y PUBLIC HAS_LIB_UNWIND=1)
else()
	target_compile_definitions(y PUBLIC HAS_LIB_UNWIND=0)
endif()

# lib: dl (optional)
if(WIN32)
	target_compile_definitions(y PUBLIC HAS_LIB_DL=0)
else()
	list(APPEND DEPS ${CMAKE_DL_LIBS})
	target_compile_definitions(y PUBLIC HAS_LIB_DL=1)
endif()


# lib: bullet (optional)
set(LIB_BULLET_ENABLED true CACHE BOOL "Use bullet library?")
pkg_check_modules(BULLET bullet)
if(BULLET_FOUND)
	target_include_directories(y PUBLIC ${BULLET_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${BULLET_LIBRARY_DIRS})
	list(APPEND DEPS ${BULLET_LIBRARIES})
	target_compile_options(y PUBLIC ${BULLET_CFLAGS_OTHER})
else()
	set(LIB_BULLET_ENABLED false CACHE BOOL "bullet library not found" FORCE)
endif()
if(${LIB_BULLET_ENABLED})
	target_compile_definitions(y PUBLIC HAS_LIB_BULLET=1)
else()
	target_compile_definitions(y PUBLIC HAS_LIB_BULLET=0)
endif()

# lib: openal (optional)
set(LIB_OPENAL_ENABLED true CACHE BOOL "Use openal library?")
pkg_check_modules(OPENAL openal)
if(OPENAL_FOUND)
	target_include_directories(y PUBLIC ${OPENAL_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${OPENAL_LIBRARY_DIRS})
	list(APPEND DEPS ${OPENAL_LIBRARIES})
	target_compile_options(y PUBLIC ${OPENAL_CFLAGS_OTHER})
else()
	set(LIB_OPENAL_ENABLED false CACHE BOOL "openal library not found" FORCE)
endif()
if(${LIB_OPENAL_ENABLED})
	target_compile_definitions(y PUBLIC HAS_LIB_OPENAL=1)
else()
	target_compile_definitions(y PUBLIC HAS_LIB_OPENAL=0)
endif()

# lib: ogg (optional)
set(LIB_OGG_ENABLED true CACHE BOOL "Use ogg library?")
pkg_check_modules(OGG ogg)
pkg_check_modules(VORBIS vorbis)
pkg_check_modules(VORBISFILE vorbisfile)
pkg_check_modules(VORBISENC vorbisenc)
if(OGG_FOUND AND VORBIS_FOUND AND VORBISFILE_FOUND AND VORBISENC_FOUND)
	target_include_directories(y PUBLIC ${OGG_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${OGG_LIBRARY_DIRS})
	list(APPEND DEPS ${OGG_LIBRARIES})
	target_compile_options(y PUBLIC ${OGG_CFLAGS_OTHER})
	target_include_directories(y PUBLIC ${VORBIS_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${VORBIS_LIBRARY_DIRS})
	list(APPEND DEPS ${VORBIS_LIBRARIES})
	target_compile_options(y PUBLIC ${VORBIS_CFLAGS_OTHER})
	target_include_directories(y PUBLIC ${VORBISFILE_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${VORBISFILE_LIBRARY_DIRS})
	list(APPEND DEPS ${VORBISFILE_LIBRARIES})
	target_compile_options(y PUBLIC ${VORBISFILE_CFLAGS_OTHER})
	target_include_directories(y PUBLIC ${VORBISENC_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${VORBISENC_LIBRARY_DIRS})
	list(APPEND DEPS ${VORBISENC_LIBRARIES})
	target_compile_options(y PUBLIC ${VORBISENC_CFLAGS_OTHER})
else()
	set(LIB_OGG_ENABLED false CACHE BOOL "ogg library not found" FORCE)
endif()
if(${LIB_OGG_ENABLED})
	target_compile_definitions(y PUBLIC HAS_LIB_OGG=1)
else()
	target_compile_definitions(y PUBLIC HAS_LIB_OGG=0)
endif()

# lib: shaderc (optional)
set(LIB_SHADERC_ENABLED true CACHE BOOL "Use shaderc library?")
pkg_check_modules(SHADERC shaderc)
if(SHADERC_FOUND)
	target_include_directories(y PUBLIC ${SHADERC_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${SHADERC_LIBRARY_DIRS})
	list(APPEND DEPS ${SHADERC_LIBRARIES})
	target_compile_options(y PUBLIC ${SHADERC_CFLAGS_OTHER})
else()
	set(LIB_SHADERC_ENABLED false CACHE BOOL "shaderc library not found" FORCE)
endif()
if(${LIB_SHADERC_ENABLED})
	target_compile_definitions(y PUBLIC HAS_LIB_SHADERC=1)
else()
	target_compile_definitions(y PUBLIC HAS_LIB_SHADERC=0)
endif()

# lib: fftw3 (optional)
set(LIB_FFTW3_ENABLED true CACHE BOOL "Use fftw3 library?")
pkg_check_modules(FFTW3F fftw3f)
if(FFTW3F_FOUND)
	target_include_directories(y PUBLIC ${FFTW3F_INCLUDE_DIRS})
	target_link_directories(y PUBLIC ${FFTW3F_LIBRARY_DIRS})
	list(APPEND DEPS ${FFTW3F_LIBRARIES})
	target_compile_options(y PUBLIC ${FFTW3F_CFLAGS_OTHER})
else()
	set(LIB_FFTW3_ENABLED false CACHE BOOL "fftw3 library not found" FORCE)
endif()
if(${LIB_FFTW3_ENABLED})
	target_compile_definitions(y PUBLIC HAS_LIB_FFTW3=1)
else()
	target_compile_definitions(y PUBLIC HAS_LIB_FFTW3=0)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND DEPS Threads::Threads)

target_link_libraries(y PRIVATE ${DEPS})

target_compile_definitions(y PUBLIC INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

install(TARGETS y DESTINATION bin)
install(DIRECTORY "static/shader" DESTINATION share/y)