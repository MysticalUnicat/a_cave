#version 450

#extension GL_GOOGLE_include_directive : require

#include "backend_vk_conf.h"

layout(set=0, binding=0) uniform Frame {
	float real_time;
	float real_time_delta;
} frame;
layout(set=0, binding=1) uniform sampler2D samplers[2];

layout(set=1, binding=0) uniform View {
	mat4 camera;
	float game_time;
	float game_time_delta;
} view;

struct DrawData {
	uint index_count;
  uint instance_count;
  uint first_index;
  int vertex_offset;
  uint first_instance;
	uint material;
};
layout(set=2, binding=0) buffer Draw {
	DrawData data[];
} draw;
struct Material {
	uint texture;
};
layout(set=2, binding=1) buffer MaterialBatch {
	Material data[];
} material_batch;
layout(set=2, binding=2) uniform texture2D texture_batch[VULKAN_UI_TEXTURE_BATCH_SIZE];

layout(location=0) in vec2 v_xy;
layout(location=1) in vec4 v_rgba;
layout(location=2) in vec2 v_st;

layout(location=0) out flat uint f_texture;
layout(location=1) out      vec4 f_rgba;
layout(location=2) out      vec2 f_st;

void main() {
	uint material = draw.data[gl_InstanceIndex].material;

	f_texture = material_batch.data[material].texture;
	f_rgba = v_rgba;
	f_st = v_st;

	gl_Position = view.camera * vec4(v_xy, 0., 1.);
}
