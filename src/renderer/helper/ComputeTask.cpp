#include "ComputeTask.h"
#include "../../graphics-impl.h"


ComputeTask::ComputeTask(const shared<Shader>& _shader) {
    shader = _shader;
}

void ComputeTask::dispatch(int nx, int ny, int nz) {
#ifdef USING_OPENGL
    for (auto& b: bindings) {
        if (b.type == Binding::Type::Texture)
            nix::bind_texture(b.index, static_cast<Texture*>(b.p));
        else if (b.type == Binding::Type::Image)
            nix::bind_image(b.index, static_cast<nix::ImageTexture*>(b.p), 0, 0, true);
        else if (b.type == Binding::Type::Buffer)
            nix::bind_buffer(b.index, static_cast<UniformBuffer*>(b.p));
    }
    shader->dispatch(nx, ny, nz);
    //nix::image_barrier();
#endif
}

void ComputeTask::bind_texture(int index, Texture *texture) {
    bindings.add({index, Binding::Type::Texture, texture});
}

void ComputeTask::bind_image(int index, ImageTexture *texture) {
    bindings.add({index, Binding::Type::Image, texture});
}

void ComputeTask::bind_buffer(int index, Buffer *buffer) {
    bindings.add({index, Binding::Type::Buffer, buffer});
}


