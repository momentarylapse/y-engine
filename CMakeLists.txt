# created by xmake

cmake_minimum_required(VERSION 3.21)
project(y)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)


#=======================================================================================
#    libraries
#=======================================================================================

find_package(PkgConfig REQUIRED)



# lib: zlib (mandatory)
find_package(ZLIB REQUIRED)
list(APPEND DEPS ZLIB::ZLIB)
list(APPEND COMPILE_DEFINITIONS HAS_LIB_ZLIB=1)


# lib: glfw (mandatory)
find_package(glfw3 REQUIRED)
list(APPEND DEPS glfw)
list(APPEND COMPILE_DEFINITIONS HAS_LIB_GLFW=1)


# lib: gl or vulkan (mandatory)
set(GL_OR_VULKAN "gl" CACHE STRING "Use gl or vulkan?")
find_package(OpenGL)
if(OPENGL_FOUND AND NOT APPLE AND NOT WIN32)
	list(APPEND GL_OR_VULKAN_FOUND "gl")
endif()
pkg_check_modules(VULKAN vulkan)
if(VULKAN_FOUND)
	list(APPEND GL_OR_VULKAN_FOUND "vulkan")
endif()
if(GL_OR_VULKAN_FOUND)
	if(NOT ${GL_OR_VULKAN} IN_LIST GL_OR_VULKAN_FOUND)
		list(GET GL_OR_VULKAN_FOUND 0 X)
		set(GL_OR_VULKAN ${X} CACHE STRING "choose" FORCE)
		message(WARNING "Choice for GL_OR_VULKAN invalid, using ${GL_OR_VULKAN} instead")
	endif()
else()
	message(SEND_ERROR "Neither gl or vulkan found") #  or/nor..
	set(GL_OR_VULKAN "")
endif()
if(GL_OR_VULKAN STREQUAL "gl")
	list(APPEND DEPS OpenGL::OpenGL)
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_GL=1)
endif()
if(GL_OR_VULKAN STREQUAL "vulkan")
	list(APPEND INCLUDE_DIRECTORIES ${VULKAN_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${VULKAN_LIBRARY_DIRS})
	list(APPEND DEPS ${VULKAN_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${VULKAN_CFLAGS_OTHER})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_VULKAN=1)
endif()


# lib: glew (optional)
set(LIB_GLEW_ENABLED true CACHE BOOL "Use glew library?")
pkg_check_modules(GLEW glew)
if(GLEW_FOUND)
else()
	set(LIB_GLEW_ENABLED false CACHE BOOL "glew library not found" FORCE)
endif()
if(${LIB_GLEW_ENABLED})
	list(APPEND INCLUDE_DIRECTORIES ${GLEW_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${GLEW_LIBRARY_DIRS})
	list(APPEND DEPS ${GLEW_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${GLEW_CFLAGS_OTHER})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_GLEW=1)
else()
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_GLEW=0)
endif()


# lib: freetype2 (mandatory)
find_package(Freetype REQUIRED)
list(APPEND DEPS Freetype::Freetype)
list(APPEND COMPILE_DEFINITIONS HAS_LIB_FREETYPE2=1)


# lib: unwind (optional)
set(LIB_UNWIND_ENABLED true CACHE BOOL "Use unwind library?")
pkg_check_modules(LIBUNWIND libunwind)
if(LIBUNWIND_FOUND)
else()
	set(LIB_UNWIND_ENABLED false CACHE BOOL "unwind library not found" FORCE)
endif()
if(${LIB_UNWIND_ENABLED})
	list(APPEND INCLUDE_DIRECTORIES ${LIBUNWIND_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${LIBUNWIND_LIBRARY_DIRS})
	list(APPEND DEPS ${LIBUNWIND_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${LIBUNWIND_CFLAGS_OTHER})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_UNWIND=1)
else()
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_UNWIND=0)
endif()


# lib: dl (optional)
if(WIN32)
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_DL=0)
else()
	list(APPEND DEPS ${CMAKE_DL_LIBS})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_DL=1)
endif()



# lib: bullet (optional)
set(LIB_BULLET_ENABLED true CACHE BOOL "Use bullet library?")
pkg_check_modules(BULLET bullet)
if(BULLET_FOUND)
else()
	set(LIB_BULLET_ENABLED false CACHE BOOL "bullet library not found" FORCE)
endif()
if(${LIB_BULLET_ENABLED})
	list(APPEND INCLUDE_DIRECTORIES ${BULLET_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${BULLET_LIBRARY_DIRS})
	list(APPEND DEPS ${BULLET_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${BULLET_CFLAGS_OTHER})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_BULLET=1)
else()
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_BULLET=0)
endif()


# lib: openal (optional)
set(LIB_OPENAL_ENABLED true CACHE BOOL "Use openal library?")
find_package(OpenAL)
if(OPENAL_FOUND)
else()
	set(LIB_OPENAL_ENABLED false CACHE BOOL "openal library not found" FORCE)
endif()
if(${LIB_OPENAL_ENABLED})
	list(APPEND DEPS OpenAL::OpenAL)
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_OPENAL=1)
else()
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_OPENAL=0)
endif()


# lib: ogg (optional)
set(LIB_OGG_ENABLED true CACHE BOOL "Use ogg library?")
pkg_check_modules(OGG ogg)
pkg_check_modules(VORBIS vorbis)
pkg_check_modules(VORBISFILE vorbisfile)
pkg_check_modules(VORBISENC vorbisenc)
if(OGG_FOUND AND VORBIS_FOUND AND VORBISFILE_FOUND AND VORBISENC_FOUND)
else()
	set(LIB_OGG_ENABLED false CACHE BOOL "ogg library not found" FORCE)
endif()
if(${LIB_OGG_ENABLED})
	list(APPEND INCLUDE_DIRECTORIES ${OGG_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${OGG_LIBRARY_DIRS})
	list(APPEND DEPS ${OGG_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${OGG_CFLAGS_OTHER})
	list(APPEND INCLUDE_DIRECTORIES ${VORBIS_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${VORBIS_LIBRARY_DIRS})
	list(APPEND DEPS ${VORBIS_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${VORBIS_CFLAGS_OTHER})
	list(APPEND INCLUDE_DIRECTORIES ${VORBISFILE_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${VORBISFILE_LIBRARY_DIRS})
	list(APPEND DEPS ${VORBISFILE_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${VORBISFILE_CFLAGS_OTHER})
	list(APPEND INCLUDE_DIRECTORIES ${VORBISENC_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${VORBISENC_LIBRARY_DIRS})
	list(APPEND DEPS ${VORBISENC_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${VORBISENC_CFLAGS_OTHER})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_OGG=1)
else()
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_OGG=0)
endif()


# lib: shaderc (optional)
set(LIB_SHADERC_ENABLED true CACHE BOOL "Use shaderc library?")
pkg_check_modules(SHADERC shaderc)
if(SHADERC_FOUND)
else()
	set(LIB_SHADERC_ENABLED false CACHE BOOL "shaderc library not found" FORCE)
endif()
if(${LIB_SHADERC_ENABLED})
	list(APPEND INCLUDE_DIRECTORIES ${SHADERC_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${SHADERC_LIBRARY_DIRS})
	list(APPEND DEPS ${SHADERC_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${SHADERC_CFLAGS_OTHER})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_SHADERC=1)
else()
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_SHADERC=0)
endif()


# lib: spirv (optional)
set(LIB_SPIRV_ENABLED true CACHE BOOL "Use spirv library?")
pkg_check_modules(SPIRV spirv)
if(SPIRV_FOUND)
else()
	set(LIB_SPIRV_ENABLED false CACHE BOOL "spirv library not found" FORCE)
endif()
if(${LIB_SPIRV_ENABLED})
	list(APPEND INCLUDE_DIRECTORIES ${SPIRV_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${SPIRV_LIBRARY_DIRS})
	list(APPEND DEPS ${SPIRV_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${SPIRV_CFLAGS_OTHER})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_SPIRV=1)
else()
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_SPIRV=0)
endif()


# lib: fftw3 (optional)
set(LIB_FFTW3_ENABLED true CACHE BOOL "Use fftw3 library?")
pkg_check_modules(FFTW3F fftw3f)
if(FFTW3F_FOUND)
else()
	set(LIB_FFTW3_ENABLED false CACHE BOOL "fftw3 library not found" FORCE)
endif()
if(${LIB_FFTW3_ENABLED})
	list(APPEND INCLUDE_DIRECTORIES ${FFTW3F_INCLUDE_DIRS})
	list(APPEND LINK_DIRECTORIES ${FFTW3F_LIBRARY_DIRS})
	list(APPEND DEPS ${FFTW3F_LIBRARIES})
	list(APPEND COMPILE_OPTIONS ${FFTW3F_CFLAGS_OTHER})
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_FFTW3=1)
else()
	list(APPEND COMPILE_DEFINITIONS HAS_LIB_FFTW3=0)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
list(APPEND DEPS Threads::Threads)


#=======================================================================================
#    target y
#=======================================================================================

add_executable(y
	src/audio/audio.cpp
	src/audio/AudioBuffer.cpp
	src/audio/AudioStream.cpp
	src/audio/Listener.cpp
	src/audio/Loading.cpp
	src/audio/SoundSource.cpp
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
	src/lib/kaba/lib/lib_async.cpp
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
	src/lib/threads/Mutex.cpp
	src/lib/threads/Thread.cpp
	src/lib/threads/ThreadedWork.cpp
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
	src/plugins/ControllerManager.cpp
	src/plugins/PluginManager.cpp
	src/renderer/gui/GuiRendererGL.cpp
	src/renderer/gui/GuiRendererVulkan.cpp
	src/renderer/helper/ComputeTask.cpp
	src/renderer/helper/CubeMapSource.cpp
	src/renderer/helper/jitter.cpp
	src/renderer/helper/LightMeter.cpp
	src/renderer/helper/Pipeline.cpp
	src/renderer/helper/PipelineManager.cpp
	src/renderer/helper/RendererFactory.cpp
	src/renderer/path/RenderPath.cpp
	src/renderer/path/RenderPathDirect.cpp
	src/renderer/post/HDRResolver.cpp
	src/renderer/post/MultisampleResolver.cpp
	src/renderer/post/PostProcessor.cpp
	src/renderer/post/PostProcessorGL.cpp
	src/renderer/post/PostProcessorVulkan.cpp
	src/renderer/post/ThroughShaderRenderer.cpp
	src/renderer/regions/RegionRenderer.cpp
	src/renderer/regions/RegionRendererGL.cpp
	src/renderer/regions/RegionRendererVulkan.cpp
	src/renderer/target/HuiWindowRendererGL.cpp
	src/renderer/target/HuiWindowRendererVulkan.cpp
	src/renderer/target/TargetRenderer.cpp
	src/renderer/target/TextureRendererGL.cpp
	src/renderer/target/TextureRendererVulkan.cpp
	src/renderer/target/WindowRendererGL.cpp
	src/renderer/target/WindowRendererVulkan.cpp
	src/renderer/world/geometry/GeometryRenderer.cpp
	src/renderer/world/geometry/GeometryRendererGL.cpp
	src/renderer/world/geometry/GeometryRendererVulkan.cpp
	src/renderer/world/geometry/RenderViewData.cpp
	src/renderer/world/geometry/SceneView.cpp
	src/renderer/world/pass/ShadowRenderer.cpp
	src/renderer/world/WorldRenderer.cpp
	src/renderer/world/WorldRendererGLDeferred.cpp
	src/renderer/world/WorldRendererGLForward.cpp
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
	src/world/Terrain.cpp
	src/world/World.cpp
	src/y/BaseClass.cpp
	src/y/Component.cpp
	src/y/ComponentManager.cpp
	src/y/EngineData.cpp
	src/y/Entity.cpp
	src/Config.cpp
	src/main.cpp)
target_include_directories(y PUBLIC ${INCLUDE_DIRECTORIES})
target_link_directories(y PUBLIC ${LINK_DIRECTORIES})
target_compile_options(y PUBLIC ${COMPILE_OPTIONS})
target_compile_definitions(y PUBLIC ${COMPILE_DEFINITIONS} INSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")
target_link_libraries(y PUBLIC ${DEPS})
target_include_directories(y PUBLIC src/)


#=======================================================================================
#    install
#=======================================================================================

set(RESOURCE_DESTINATION share/y)

# binaries
install(TARGETS y DESTINATION bin)	

# resources
install(DIRECTORY "static" DESTINATION ${RESOURCE_DESTINATION})