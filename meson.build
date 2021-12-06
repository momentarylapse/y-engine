project('y', 'cpp', default_options : ['cpp_std=c++17', 'warning_level=1', 'optimization=2', 'debug=true'])
add_project_arguments('-Wno-unused', '-Wstrict-aliasing=0', '-fno-omit-frame-pointer', language : ['cpp'])

#install_data
#meson.add_install_script('')
#install_subdir

cc = meson.get_compiler('cpp')

# mandatory dependencies
mdep = cc.find_library('m', required : false)
threaddep = dependency('threads')
zlibdep = dependency('zlib')
add_project_arguments('-DHAS_LIB_ZLIB=1', language : ['cpp'])
glfwdep = dependency('glfw')
add_project_arguments('-DHAS_LIB_GLFW=1', language : ['cpp'])
gldep = dependency('GL')
add_project_arguments('-DHAS_LIB_GL=1', language : ['cpp'])
freetype2dep = dependency('freetype2')
add_project_arguments('-DHAS_LIB_FREETYPE2=1', language : ['cpp'])
deps = [mdep, threaddep, zlibdep, glfwdep, gldep, freetype2dep]

# optional dependency: unwind
d = cc.find_library('unwind', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_UNWIND=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_UNWIND=0', language : ['cpp'])
endif

# optional dependency: dl
d = cc.find_library('dl', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_DL=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_DL=0', language : ['cpp'])
endif

# optional dependency: bullet
d1 = cc.find_library('BulletSoftBody', required : false)
d2 = cc.find_library('BulletDynamics', required : false)
d3 = cc.find_library('BulletCollision', required : false)
d4 = cc.find_library('LinearMath', required : false)
if d1.found() and d2.found() and d3.found() and d4.found()
	add_project_arguments('-DHAS_LIB_BULLET=1', language : ['cpp'])
	deps += [d1,d2,d3,d4]
else
	add_project_arguments('-DHAS_LIB_BULLET=0', language : ['cpp'])
endif

# optional dependency: openal
d = cc.find_library('openal', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_OPENAL=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_OPENAL=0', language : ['cpp'])
endif

# optional dependency: ogg
d1 = cc.find_library('ogg', required : false)
d2 = cc.find_library('vorbis', required : false)
d3 = cc.find_library('vorbisfile', required : false)
d4 = cc.find_library('vorbisenc', required : false)
if d1.found() and d2.found() and d3.found() and d4.found()
	add_project_arguments('-DHAS_LIB_OGG=1', language : ['cpp'])
	deps += [d1,d2,d3,d4]
else
	add_project_arguments('-DHAS_LIB_OGG=0', language : ['cpp'])
endif

# optional dependency: vulkan
d = cc.find_library('vulkan', required : false)
if d.found()
	add_project_arguments('-DHAS_LIB_VULKAN=1', language : ['cpp'])
	deps += [d]
else
	add_project_arguments('-DHAS_LIB_VULKAN=0', language : ['cpp'])
endif

# optional dependency: shaderc
if 
	add_project_arguments('-DHAS_LIB_SHADERC=1', language : ['cpp'])
	deps += []
else
	add_project_arguments('-DHAS_LIB_SHADERC=0', language : ['cpp'])
endif


sources = [
	'src/audio/Sound.cpp',
	'src/audio/SoundAL.cpp',
	'src/fx/Beam.cpp',
	'src/fx/Particle.cpp',
	'src/fx/ParticleManager.cpp',
	'src/gui/Font.cpp',
	'src/gui/gui.cpp',
	'src/gui/Node.cpp',
	'src/gui/Picture.cpp',
	'src/gui/Text.cpp',
	'src/helper/ErrorHandler.cpp',
	'src/helper/PerformanceMonitor.cpp',
	'src/helper/ResourceManager.cpp',
	'src/helper/Scheduler.cpp',
	'src/input/Gamepad.cpp',
	'src/input/InputManager.cpp',
	'src/input/Keyboard.cpp',
	'src/input/Mouse.cpp',
	'src/lib/any/any.cpp',
	'src/lib/base/array.cpp',
	'src/lib/base/pointer.cpp',
	'src/lib/base/strings.cpp',
	'src/lib/file/file.cpp',
	'src/lib/file/file_op.cpp',
	'src/lib/file/msg.cpp',
	'src/lib/file/path.cpp',
	'src/lib/hui_minimal/Application.cpp',
	'src/lib/hui_minimal/Callback.cpp',
	'src/lib/hui_minimal/Config.cpp',
	'src/lib/hui_minimal/error.cpp',
	'src/lib/hui_minimal/hui.cpp',
	'src/lib/hui_minimal/Timer.cpp',
	'src/lib/image/color.cpp',
	'src/lib/image/image.cpp',
	'src/lib/image/image_bmp.cpp',
	'src/lib/image/image_jpg.cpp',
	'src/lib/image/image_png.cpp',
	'src/lib/image/image_tga.cpp',
	'src/lib/image/ImagePainter.cpp',
	'src/lib/kaba/asm/asm.cpp',
	'src/lib/kaba/asm/asm_arm.cpp',
	'src/lib/kaba/asm/asm_x86.cpp',
	'src/lib/kaba/compiler/Backend.cpp',
	'src/lib/kaba/compiler/BackendAmd64.cpp',
	'src/lib/kaba/compiler/BackendARM.cpp',
	'src/lib/kaba/compiler/BackendX86.cpp',
	'src/lib/kaba/compiler/CommandList.cpp',
	'src/lib/kaba/compiler/compiler.cpp',
	'src/lib/kaba/compiler/mapper.cpp',
	'src/lib/kaba/compiler/serializer.cpp',
	'src/lib/kaba/compiler/SerializerX.cpp',
	'src/lib/kaba/compiler/SerialNode.cpp',
	'src/lib/kaba/dynamic/call.cpp',
	'src/lib/kaba/dynamic/dynamic.cpp',
	'src/lib/kaba/dynamic/exception.cpp',
	'src/lib/kaba/lib/dict.cpp',
	'src/lib/kaba/lib/extern.cpp',
	'src/lib/kaba/lib/lib.cpp',
	'src/lib/kaba/lib/lib_base.cpp',
	'src/lib/kaba/lib/lib_doc.cpp',
	'src/lib/kaba/lib/lib_hui.cpp',
	'src/lib/kaba/lib/lib_image.cpp',
	'src/lib/kaba/lib/lib_kaba.cpp',
	'src/lib/kaba/lib/lib_math.cpp',
	'src/lib/kaba/lib/lib_net.cpp',
	'src/lib/kaba/lib/lib_nix.cpp',
	'src/lib/kaba/lib/lib_os.cpp',
	'src/lib/kaba/lib/lib_threads.cpp',
	'src/lib/kaba/lib/lib_time.cpp',
	'src/lib/kaba/lib/lib_vulkan.cpp',
	'src/lib/kaba/syntax/Block.cpp',
	'src/lib/kaba/syntax/Class.cpp',
	'src/lib/kaba/syntax/Constant.cpp',
	'src/lib/kaba/syntax/Function.cpp',
	'src/lib/kaba/syntax/Identifier.cpp',
	'src/lib/kaba/syntax/implicit.cpp',
	'src/lib/kaba/syntax/lexical.cpp',
	'src/lib/kaba/syntax/macros.cpp',
	'src/lib/kaba/syntax/Node.cpp',
	'src/lib/kaba/syntax/Operator.cpp',
	'src/lib/kaba/syntax/Parser.cpp',
	'src/lib/kaba/syntax/preprocessor.cpp',
	'src/lib/kaba/syntax/Statement.cpp',
	'src/lib/kaba/syntax/SyntaxTree.cpp',
	'src/lib/kaba/syntax/Variable.cpp',
	'src/lib/kaba/CompilerConfiguration.cpp',
	'src/lib/kaba/Interpreter.cpp',
	'src/lib/kaba/kaba.cpp',
	'src/lib/math/complex.cpp',
	'src/lib/math/interpolation.cpp',
	'src/lib/math/math.cpp',
	'src/lib/math/matrix.cpp',
	'src/lib/math/matrix3.cpp',
	'src/lib/math/plane.cpp',
	'src/lib/math/quaternion.cpp',
	'src/lib/math/random.cpp',
	'src/lib/math/ray.cpp',
	'src/lib/math/rect.cpp',
	'src/lib/math/vec2.cpp',
	'src/lib/math/vec4.cpp',
	'src/lib/math/vector.cpp',
	'src/lib/net/BinaryBuffer.cpp',
	'src/lib/net/net.cpp',
	'src/lib/net/Socket.cpp',
	'src/lib/nix/nix.cpp',
	'src/lib/nix/nix_buffer.cpp',
	'src/lib/nix/nix_draw.cpp',
	'src/lib/nix/nix_framebuffer.cpp',
	'src/lib/nix/nix_light.cpp',
	'src/lib/nix/nix_shader.cpp',
	'src/lib/nix/nix_textures.cpp',
	'src/lib/nix/nix_vertexbuffer.cpp',
	'src/lib/nix/nix_view.cpp',
	'src/lib/vulkan/AccelerationStructure.cpp',
	'src/lib/vulkan/Buffer.cpp',
	'src/lib/vulkan/CommandBuffer.cpp',
	'src/lib/vulkan/DescriptorSet.cpp',
	'src/lib/vulkan/Device.cpp',
	'src/lib/vulkan/FrameBuffer.cpp',
	'src/lib/vulkan/helper.cpp',
	'src/lib/vulkan/Instance.cpp',
	'src/lib/vulkan/Pipeline.cpp',
	'src/lib/vulkan/Queue.cpp',
	'src/lib/vulkan/RenderPass.cpp',
	'src/lib/vulkan/Semaphore.cpp',
	'src/lib/vulkan/Shader.cpp',
	'src/lib/vulkan/SwapChain.cpp',
	'src/lib/vulkan/Texture.cpp',
	'src/lib/vulkan/VertexBuffer.cpp',
	'src/lib/vulkan/vulkan.cpp',
	'src/lib/xfile/chunked.cpp',
	'src/lib/xfile/pdf.cpp',
	'src/lib/xfile/xml.cpp',
	'src/net/NetworkManager.cpp',
	'src/plugins/Controller.cpp',
	'src/plugins/PluginManager.cpp',
	'src/renderer/gui/GuiRendererGL.cpp',
	'src/renderer/gui/GuiRendererVulkan.cpp',
	'src/renderer/helper/jitter.cpp',
	'src/renderer/post/blur.cpp',
	'src/renderer/post/HDRRendererGL.cpp',
	'src/renderer/post/HDRRendererVulkan.cpp',
	'src/renderer/target/TargetRenderer.cpp',
	'src/renderer/target/WindowRendererGL.cpp',
	'src/renderer/target/WindowRendererVulkan.cpp',
	'src/renderer/base.cpp',
	'src/renderer/Renderer.cpp',
	'src/renderer/RenderPath.cpp',
	'src/renderer/RenderPathGL.cpp',
	'src/renderer/RenderPathGLDeferred.cpp',
	'src/renderer/RenderPathGLForward.cpp',
	'src/renderer/RenderPathVulkan.cpp',
	'src/renderer/RenderPathVulkanForward.cpp',
	'src/world/components/Animator.cpp',
	'src/world/components/Collider.cpp',
	'src/world/components/Skeleton.cpp',
	'src/world/components/SolidBody.cpp',
	'src/world/Camera.cpp',
	'src/world/Entity3D.cpp',
	'src/world/LevelData.cpp',
	'src/world/Light.cpp',
	'src/world/Link.cpp',
	'src/world/Material.cpp',
	'src/world/Model.cpp',
	'src/world/ModelManager.cpp',
	'src/world/Object.cpp',
	'src/world/Terrain.cpp',
	'src/world/World.cpp',
	'src/y/Component.cpp',
	'src/y/ComponentManager.cpp',
	'src/y/EngineData.cpp',
	'src/y/Entity.cpp',
	'src/Config.cpp',
	'src/main.cpp'
]


executable('y', sources, dependencies : deps, install : true)

install_subdir('static/shader', install_dir : join_paths(get_option('datadir'), 'y'))
