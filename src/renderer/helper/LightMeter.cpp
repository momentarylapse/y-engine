//
// Created by michi on 12/29/24.
//

#include "LightMeter.h"
#include "../helper/ComputeTask.h"
#include <world/Camera.h>
#include "../../graphics-impl.h"
#include "../base.h"
#include "../../helper/PerformanceMonitor.h"
#include "../../helper/ResourceManager.h"



LightMeter::LightMeter(ResourceManager* resource_manager, Texture* tex, int channel) {
	ch_post_brightness = PerformanceMonitor::create_channel("expo", channel);
	compute = new ComputeTask(resource_manager->load_shader("compute/brightness.shader"));
#ifdef USING_VULKAN
	params = new UniformBuffer(8);
	buf = new ShaderStorageBuffer(256*4);
#else
	params = new UniformBuffer();
	buf = new ShaderStorageBuffer();
#endif
	compute->bind_texture(0, tex);
	compute->bind_storage_buffer(1, buf);
	compute->bind_uniform_buffer(2, params);
}

void LightMeter::measure(const RenderParams& _params, Texture* tex) {
	PerformanceMonitor::begin(ch_post_brightness);
	gpu_timestamp_begin(_params, ch_post_brightness);

	int NBINS = 256;
	const int NSAMPLES = 256;
#ifdef USING_VULKAN
	auto cb = _params.command_buffer;

	int pp[2] = {tex->width, tex->height};
	params->update(&pp);

	if (histogram.num == NBINS) {
		void* p = buf->map();
		memcpy(&histogram[0], p, NBINS*sizeof(int));
		buf->unmap();
		//msg_write(str(histogram));

		int thresh = (NSAMPLES * 16 * 16) / 200 * 199;
		int n = 0;
		int ii = 0;
		for (int i=0; i<NBINS; i++) {
			n += histogram[i];
			if (n > thresh) {
				ii = i;
				break;
			}
		}
		brightness = pow(2.0f, ((float)ii / (float)NBINS) * 20.0f - 10.0f);
	}

	histogram.resize(NBINS);
	memset(&histogram[0], 0, NBINS * sizeof(int));
	buf->update(&histogram[0]);
	compute->dispatch(cb, NSAMPLES, 1, 1);
#else
	histogram.resize(NBINS);
	memset(&histogram[0], 0, NBINS * sizeof(int));
	buf->update(&histogram[0], NBINS * sizeof(int));

	int pp[2] = {tex->width, tex->height};
	params->update(&pp, sizeof(pp));
	compute->dispatch(NSAMPLES, 1, 1);

	buf->read(&histogram[0], NBINS*sizeof(int));
	//msg_write(str(histogram));

	gpu_timestamp_end(_params, ch_post_brightness);

	/*int s = 0;
	for (int i=0; i<NBINS; i++)
		s += histogram[i];
	msg_write(format("%d  %d", s, NSAMPLES*256));*/

	int thresh = (NSAMPLES * 16 * 16) / 200 * 199;
	int n = 0;
	int ii = 0;
	for (int i=0; i<NBINS; i++) {
		n += histogram[i];
		if (n > thresh) {
			ii = i;
			break;
		}
	}
	brightness = pow(2.0f, ((float)ii / (float)NBINS) * 20.0f - 10.0f);
#endif
	PerformanceMonitor::end(ch_post_brightness);
}

void LightMeter::adjust_camera(Camera *cam) {
	float exposure = clamp((float)pow(1.0f / brightness, 0.8f), cam->auto_exposure_min, cam->auto_exposure_max);
	if (exposure > cam->exposure)
		cam->exposure *= 1.05f;
	if (exposure < cam->exposure)
		cam->exposure /= 1.05f;
}
