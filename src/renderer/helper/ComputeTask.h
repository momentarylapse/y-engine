#ifndef COMPUTETASK_H
#define COMPUTETASK_H


#include "../Renderer.h"
#include <lib/base/base.h>
#include <lib/base/pointer.h>
#include <lib/image/image.h>
#include "../../graphics-fwd.h"

class ComputeTask : public RenderTask {
public:
    explicit ComputeTask(const string& name, const shared<Shader>& shader, int nx, int ny, int nz);
    shared<Shader> shader = nullptr;

    struct Binding {
        enum class Type {
            Texture,
            Image,
            UniformBuffer,
            StorageBuffer
        };
        int index;
        Type type;
        void* p;
    };
    Array<Binding> bindings;
    int nx, ny, nz;

    void bind_texture(int index, Texture* texture);
    void bind_image(int index, ImageTexture* image);
    void bind_uniform_buffer(int index, Buffer* buffer);
    void bind_storage_buffer(int index, Buffer* buffer);

#ifdef USING_VULKAN
    owned<vulkan::ComputePipeline> pipeline;
    owned<vulkan::DescriptorPool> pool;
    owned<vulkan::DescriptorSet> dset;
#endif

    void render(const RenderParams &params) override;
};


#endif //COMPUTETASK_H
