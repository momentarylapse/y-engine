#include "ComputeTask.h"
#include <renderer/base.h>
#include "../../graphics-impl.h"
#include "../../helper/PerformanceMonitor.h"


#ifdef USING_VULKAN
void apply_shader_data(CommandBuffer* cb, const Any &shader_data);
#else
void apply_shader_data(Shader *s, const Any &shader_data);
#endif

ComputeTask::ComputeTask(const string& name, const shared<Shader>& _shader, int _nx, int _ny, int _nz) :
    RenderTask(name)
{
    shader = _shader;
    nx = _nx;
    ny = _ny;
    nz = _nz;
#ifdef USING_VULKAN
    if (name != "") {
        pool = new vulkan::DescriptorPool("sampler:8,buffer:8,storage-buffer:8,image:8", 1);
        dset = pool->create_set_from_layout(shader->descr_layouts[0]);
        pipeline = new vulkan::ComputePipeline(shader.get());
    }
#endif
}

void ComputeTask::render(const RenderParams &params) {
	PerformanceMonitor::begin(ch_draw);
    gpu_timestamp_begin(params, ch_draw);
#ifdef USING_OPENGL
    for (auto& b: bindings) {
        if (b.type == Binding::Type::Texture)
            nix::bind_texture(b.index, static_cast<Texture*>(b.p));
        else if (b.type == Binding::Type::Image)
            nix::bind_image(b.index, static_cast<nix::ImageTexture*>(b.p), 0, 0, true);
        else if (b.type == Binding::Type::UniformBuffer)
            nix::bind_uniform_buffer(b.index, static_cast<nix::UniformBuffer*>(b.p));
        else if (b.type == Binding::Type::StorageBuffer)
            nix::bind_storage_buffer(b.index, static_cast<nix::ShaderStorageBuffer*>(b.p));
    }
    apply_shader_data(shader.get(), shader_data);
    shader->dispatch(nx, ny, nz);
    nix::image_barrier();
#endif
#ifdef USING_VULKAN
    auto cb = params.command_buffer;
    cb->set_bind_point(vulkan::PipelineBindPoint::COMPUTE);
    cb->bind_pipeline(pipeline.get());
    cb->bind_descriptor_set(0, dset.get());
    apply_shader_data(cb, shader_data);
    cb->dispatch(nx, ny, nz);
    cb->set_bind_point(vulkan::PipelineBindPoint::GRAPHICS);
    // TODO barriers
#endif
    gpu_timestamp_end(params, ch_draw);
	PerformanceMonitor::end(ch_draw);
}

void ComputeTask::bind_texture(int index, Texture *texture) {
#ifdef USING_OPENGL
    bindings.add({index, Binding::Type::Texture, texture});
#endif
#ifdef USING_VULKAN
    dset->set_texture(index, texture);
    dset->update();
#endif
}

void ComputeTask::bind_image(int index, ImageTexture *texture) {
#ifdef USING_OPENGL
    bindings.add({index, Binding::Type::Image, texture});
#endif
#ifdef USING_VULKAN
    dset->set_storage_image(index, texture);
    dset->update();
#endif
}

void ComputeTask::bind_uniform_buffer(int index, Buffer *buffer) {
#ifdef USING_OPENGL
    bindings.add({index, Binding::Type::UniformBuffer, buffer});
#endif
#ifdef USING_VULKAN
    dset->set_uniform_buffer(index, buffer);
    dset->update();
#endif
}

void ComputeTask::bind_storage_buffer(int index, Buffer *buffer) {
#ifdef USING_OPENGL
    bindings.add({index, Binding::Type::StorageBuffer, buffer});
#endif
#ifdef USING_VULKAN
    dset->set_storage_buffer(index, buffer);
    dset->update();
#endif
}


