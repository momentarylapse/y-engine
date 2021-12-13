/*
 * PostProcessor.cpp
 *
 *  Created on: Dec 13, 2021
 *      Author: michi
 */

#include "PostProcessor.h"


string callable_name(const void *c);

PostProcessorStage::PostProcessorStage(const string &name) : Renderer(name, nullptr) {
}


PostProcessorStageUser::PostProcessorStageUser(const PostProcessorStageUser::Callback *p, const PostProcessorStageUser::Callback *d) :
		PostProcessorStage(callable_name(d)) {
}

void PostProcessorStageUser::prepare() {
	if (func_prepare)
		(*func_prepare)();
}
void PostProcessorStageUser::draw() {
	if (func_draw)
		(*func_draw)();
}

PostProcessor::PostProcessor(Renderer *parent) : Renderer("post", parent) {
}

PostProcessor::~PostProcessor() {
}


void PostProcessor::add_stage(const PostProcessorStageUser::Callback *p, const PostProcessorStageUser::Callback *d) {
	stages.add(new PostProcessorStageUser(p, d));
}
void PostProcessor::reset() {
	stages.clear();
}
