/*
 * PluginManager.cpp
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#include "PluginManager.h"
#include "Controller.h"
#include "../lib/kaba/kaba.h"
#include "../audio/SoundSource.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../fx/ParticleEmitter.h"
#include "../gui/gui.h"
#include "../gui/Node.h"
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../helper/DeletionQueue.h"
#include "../helper/PerformanceMonitor.h"
#include "../helper/ResourceManager.h"
#include "../helper/Scheduler.h"
#include "../helper/TimeTable.h"
#include "../input/InputManager.h"
#include "../input/Gamepad.h"
#include "../input/Keyboard.h"
#include "../input/Mouse.h"
#include "../net/NetworkManager.h"
#include "../renderer/base.h"
#include "../renderer/Renderer.h"
#include "../renderer/helper/RendererFactory.h"
#ifdef USING_OPENGL
#include "../renderer/world/WorldRendererGL.h"
#include "../renderer/world/WorldRendererGLForward.h"
#include "../renderer/world/WorldRendererGLDeferred.h"
#include "../renderer/gui/GuiRendererGL.h"
#include "../renderer/regions/RegionRendererGL.h"
#include "../renderer/post/HDRRendererGL.h"
#include "../renderer/post/PostProcessorGL.h"
#include "../renderer/target/WindowRendererGL.h"
#endif
#ifdef USING_VULKAN
#include "../renderer/world/WorldRendererVulkan.h"
#include "../renderer/world/WorldRendererVulkanForward.h"
#include "../renderer/gui/GuiRendererVulkan.h"
#include "../renderer/regions/RegionRendererVulkan.h"
#include "../renderer/post/HDRRendererVulkan.h"
#include "../renderer/post/PostProcessorVulkan.h"
#include "../renderer/target/WindowRendererVulkan.h"
#endif

#include "../renderer/world/geometry/SceneView.h"
#include "../y/EngineData.h"
#include "../y/Component.h"
#include "../y/ComponentManager.h"
#include "../world/Camera.h"
#include "../world/Link.h"
#include "../world/Model.h"
#include "../world/ModelManager.h"
#include "../world/Object.h"
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../world/Light.h"
#include "../world/components/SolidBody.h"
#include "../world/components/Collider.h"
#include "../world/components/Animator.h"
#include "../world/components/Skeleton.h"
#include "../world/components/UserMesh.h"
#include "../world/components/MultiInstance.h"
#include "../meta.h"
#include "../graphics-impl.h"
#include "../lib/kaba/dynamic/exception.h"
#include "../lib/os/msg.h"


Array<Controller*> PluginManager::controllers;
int ch_controller = -1;


/*void global_delete(BaseClass *e) {
	//msg_error("global delete... " + p2s(e));
	world.unregister(e);
	if (e->type == BaseClass::Type::ENTITY)
		reinterpret_cast<Entity*>(e)->on_delete_rec();
	else
		e->on_delete();
	delete e;
}*/


#pragma GCC push_options
#pragma GCC optimize("no-omit-frame-pointer")
#pragma GCC optimize("no-inline")
#pragma GCC optimize("0")

Model* _create_object(World *w, const Path &filename, const vec3 &pos, const quaternion &ang) {
	KABA_EXCEPTION_WRAPPER( return w->create_object(filename, pos, ang); );
	return nullptr;
}

Model* _create_object_no_reg(World *w, const Path &filename, const vec3 &pos, const quaternion &ang) {
	KABA_EXCEPTION_WRAPPER( return w->create_object_no_reg(filename, pos, ang); );
	return nullptr;
}

MultiInstance* _create_object_multi(World *w, const Path &filename, const Array<vec3> &pos, const Array<quaternion> &ang) {
	KABA_EXCEPTION_WRAPPER( return w->create_object_multi(filename, pos, ang); );
	return nullptr;
}

Model* _attach_model(World *w, Entity& e, const Path &filename) {
	KABA_EXCEPTION_WRAPPER( return &w->attach_model(e, filename); );
	return nullptr;
}

void framebuffer_init(FrameBuffer *fb, const shared_array<Texture> &tex) {
#ifdef USING_VULKAN
	kaba::kaba_raise_exception(new kaba::KabaException("not implemented: FrameBuffer.__init__() for vulkan"));
#else
	new(fb) FrameBuffer(tex);
#endif
}

shared<Texture> framebuffer_depthbuffer(FrameBuffer *fb) {
#ifdef USING_VULKAN
	return fb->attachments.back().get();
#else
	return fb->depth_buffer.get();
#endif
}

shared_array<Texture> framebuffer_color_attachments(FrameBuffer *fb) {
#ifdef USING_VULKAN
	return fb->attachments;//.sub_ref(0, -1));
#else
	return fb->color_attachments;
#endif
}


void vertexbuffer_init(VertexBuffer *vb, const string &format) {
	new(vb) VertexBuffer(format);
}

void vertexbuffer_update(VertexBuffer *vb, const DynamicArray &vertices) {
	vb->update(vertices);
}

void texture_init(Texture *t, int w, int h, const string &format) {
	new(t) Texture(w, h, format);
}

void texture_delete(Texture *t) {
	t->~Texture();
}

void texture_write(Texture *t, const Image &im) {
	t->write(im);
}

void texture_write_float(Texture *t, const DynamicArray& data) {
#ifdef USING_VULKAN
	msg_error("unimplemented:  Texture.write_float()");
#else
	t->write_float(data);
#endif
}

void cubemap_init(CubeMap *t, int size, const string &format) {
	new(t) CubeMap(size, format);
}

void depthbuffer_init(DepthBuffer *t, int w, int h, const string &format) {
#ifdef USING_VULKAN
	new(t) DepthBuffer(w, h, format, true);
#else
	new(t) DepthBuffer(w, h, format);
#endif
}

void volumetexture_init(VolumeTexture *t, int nx, int ny, int nz, const string &format) {
#ifdef USING_VULKAN
	//new(t) DepthBuffer(w, h, format, true);
#else
	new(t) VolumeTexture(nx, ny, nz, format);
#endif
}

void shader_set_float(Shader *s, const string &name, float f) {
#ifdef USING_VULKAN
	msg_error("unimplemented:  Shader.set_float()");
#else
	s->set_float(name, f);
#endif
}

void shader_set_floats(Shader *s, const string &name, float *f, int num) {
#ifdef USING_VULKAN
	msg_error("unimplemented:  Shader.set_floats()");
#else
	s->set_floats(name, f, num);
#endif
}

#pragma GCC pop_options

void screenshot(Image& im) {
#ifdef USING_VULKAN
	msg_error("unimplemented:  screenshot()");
#else
	engine.context->default_framebuffer->read(im);
#endif
}


xfer<Model> __load_model(const Path& filename) {
	return engine.resource_manager->load_model(filename);
}

shared<Shader> __load_shader(const Path& filename) {
	return engine.resource_manager->load_shader(filename);
}

xfer<Shader> __create_shader(const string& source) {
	return engine.resource_manager->create_shader(source);
}

shared<Texture> __load_texture(const Path& filename) {
	return engine.resource_manager->load_texture(filename);
}

xfer<Material> __load_material(const Path& filename) {
	return engine.resource_manager->load_material(filename);
}


void timetable_at(TimeTable *tt, float t, Callable<void()> *f) {
	tt->at(t, [f] { (*f)(); });
}


CubeMap* world_renderer_get_cubemap(WorldRenderer &r) {
	return r.scene_view.cube_map.get();
}

Array<FrameBuffer*> world_renderer_get_fb_shadow(WorldRenderer &r) {
	return {r.scene_view.fb_shadow1.get(), r.scene_view.fb_shadow2.get()};
}

FrameBuffer* world_renderer_get_gbuffer(WorldRenderer &r) {
#ifdef USING_OPENGL
	if (r.type == RenderPathType::DEFERRED)
		return reinterpret_cast<WorldRendererGLDeferred&>(r).gbuffer.get();
#endif
	return nullptr;
}

Array<FrameBuffer*> hdr_renderer_get_fb_bloom(HDRRenderer &r) {
	return {r.bloom_levels[0].fb_out.get(), r.bloom_levels[1].fb_out.get(), r.bloom_levels[2].fb_out.get(), r.bloom_levels[3].fb_out.get()};
}

template<class T>
void generic_init(T* t) {
	new(t) T;
}

template<class T>
void generic_delete(T* t) {
	t->~T();
}

void PluginManager::init(int ch_iter) {
	ch_controller = PerformanceMonitor::create_channel("controller", ch_iter);
	export_kaba();
	import_kaba();
}

void PluginManager::export_kaba() {
	auto ext = kaba::default_context->external.get();

	BaseClass entity(BaseClass::Type::NONE);
	ext->declare_class_size("BaseClass", sizeof(BaseClass));
//	ext->link_class_func("BaseClass.__init__", &Entity::__init__);
	ext->link_virtual("BaseClass.__delete__", &BaseClass::__delete__, &entity);
	ext->link_virtual("BaseClass.on_init", &BaseClass::on_init, &entity);
	ext->link_virtual("BaseClass.on_delete", &BaseClass::on_delete, &entity);
	ext->link_virtual("BaseClass.on_iterate", &BaseClass::on_iterate, &entity);

	ext->declare_class_size("Entity", sizeof(Entity));
	ext->declare_class_element("Entity.pos", &Entity::pos);
	ext->declare_class_element("Entity.ang", &Entity::ang);
	ext->declare_class_element("Entity.parent", &Entity::parent);
	ext->link_class_func("Entity.get_matrix", &Entity::get_matrix);
	ext->link_class_func("Entity.__get_component", &Entity::_get_component_untyped_);
	ext->link_class_func("Entity.__add_component", &Entity::_add_component_untyped_);
	ext->link_class_func("Entity.delete_component", &Entity::delete_component);
	ext->link_class_func("Entity.__del_override__", &DeletionQueue::add);

	Component component;
	ext->declare_class_size("Component", sizeof(Component));
	ext->declare_class_element("Component.owner", &Component::owner);
	ext->link_class_func("Component.__init__", &generic_init<Component>);
	ext->link_virtual("Component.__delete__", &Component::__delete__, &component);
	ext->link_virtual("Component.on_init", &Component::on_init, &component);
	ext->link_virtual("Component.on_delete", &Component::on_delete, &component);
	ext->link_virtual("Component.on_iterate", &Component::on_iterate, &component);
	ext->link_virtual("Component.on_collide", &Component::on_collide, &component);
	ext->link_class_func("Component.set_variables", &Component::set_variables);

	Camera _cam;
	ext->declare_class_size("Camera", sizeof(Camera));
	ext->declare_class_element("Camera.fov", &Camera::fov);
	ext->declare_class_element("Camera.exposure", &Camera::exposure);
	ext->declare_class_element("Camera.bloom_radius", &Camera::bloom_radius);
	ext->declare_class_element("Camera.bloom_factor", &Camera::bloom_factor);
	ext->declare_class_element("Camera.focus_enabled", &Camera::focus_enabled);
	ext->declare_class_element("Camera.focal_length", &Camera::focal_length);
	ext->declare_class_element("Camera.focal_blur", &Camera::focal_blur);
	ext->declare_class_element("Camera.enabled", &Camera::enabled);
	ext->declare_class_element("Camera.show", &Camera::show);
	ext->declare_class_element("Camera.min_depth", &Camera::min_depth);
	ext->declare_class_element("Camera.max_depth", &Camera::max_depth);
	ext->declare_class_element("Camera.m_view", &Camera::m_view);
	ext->link_class_func("Camera.update_matrices", &Camera::update_matrices);
	ext->link_class_func("Camera.project", &Camera::project);
	ext->link_class_func("Camera.unproject", &Camera::unproject);


	ext->declare_class_size("Model.Mesh", sizeof(Mesh));
	ext->declare_class_element("Model.Mesh.bone_index", &Mesh::bone_index);
	ext->declare_class_element("Model.Mesh.vertex", &Mesh::vertex);
	ext->declare_class_element("Model.Mesh.sub", &Mesh::sub);

	ext->declare_class_size("Model.Mesh.Sub", sizeof(SubMesh));
	ext->declare_class_element("Model.Mesh.Sub.num_triangles", &SubMesh::num_triangles);
	ext->declare_class_element("Model.Mesh.Sub.triangle_index", &SubMesh::triangle_index);
	ext->declare_class_element("Model.Mesh.Sub.skin_vertex", &SubMesh::skin_vertex);
	ext->declare_class_element("Model.Mesh.Sub.normal", &SubMesh::normal);

	Model model;
	ext->declare_class_size("Model", sizeof(Model));
	ext->declare_class_element("Model.mesh", &Model::mesh);
	ext->declare_class_element("Model.materials", &Model::material);
	ext->declare_class_element("Model.matrix", &Model::_matrix);
	ext->declare_class_element("Model.radius", (char*)&model.prop.radius - (char*)&model);
	ext->declare_class_element("Model.min", (char*)&model.prop.min - (char*)&model);
	ext->declare_class_element("Model.max", (char*)&model.prop.max- (char*)&model);
	ext->declare_class_element("Model.name", (char*)&model.script_data.name - (char*)&model);
	ext->link_class_func("Model.__init__", &Model::__init__);
	ext->link_virtual("Model.__delete__", &Model::__delete__, &model);
	ext->link_class_func("Model.make_editable", &Model::make_editable);
	ext->link_class_func("Model.begin_edit", &Model::begin_edit);
	ext->link_class_func("Model.end_edit", &Model::end_edit);
	ext->link_class_func("Model.get_vertex", &Model::get_vertex);
	ext->link_class_func("Model.update_matrix", &Model::update_matrix);
//	ext->link_class_func("Model.set_bone_model", &Model::set_bone_model);

	ext->link_virtual("Model.on_init", &Model::on_init, &model);
	ext->link_virtual("Model.on_delete", &Model::on_delete, &model);
	ext->link_virtual("Model.on_iterate", &Model::on_iterate, &model);


	ext->declare_class_size("UserMesh", sizeof(UserMesh));
	ext->declare_class_element("UserMesh.vertex_buffer", &UserMesh::vertex_buffer);
	ext->declare_class_element("UserMesh.material", &UserMesh::material);
	ext->declare_class_element("UserMesh.vertex_shader_module", &UserMesh::vertex_shader_module);
	ext->declare_class_element("UserMesh.geometry_shader_module", &UserMesh::geometry_shader_module);
	ext->declare_class_element("UserMesh.topology", &UserMesh::topology);


	ext->declare_class_size("Animator", sizeof(Animator));
	ext->link_class_func("Animator.reset", &Animator::reset);
	ext->link_class_func("Animator.add", &Animator::add);
	ext->link_class_func("Animator.add_x", &Animator::add_x);
	ext->link_class_func("Animator.is_done", &Animator::is_done);
	ext->link_class_func("Animator.begin_edit", &Animator::reset); // JUST FOR COMPATIBILITY WITH OLD BRANCH


	ext->declare_class_size("Skeleton", sizeof(Skeleton));
	ext->declare_class_element("Skeleton.bones", &Skeleton::bones);
	ext->declare_class_element("Skeleton.parents", &Skeleton::parents);
	ext->declare_class_element("Skeleton.dpos", &Skeleton::dpos);
	ext->declare_class_element("Skeleton.pos0", &Skeleton::pos0);
	ext->link_class_func("Skeleton.reset", &Skeleton::reset);


	ext->declare_class_size("SolidBody", sizeof(SolidBody));
	ext->declare_class_element("SolidBody.vel", &SolidBody::vel);
	ext->declare_class_element("SolidBody.rot", &SolidBody::rot);
	ext->declare_class_element("SolidBody.mass", &SolidBody::mass);
	//ext->declare_class_element("SolidBody.theta", &SolidBody::theta_world);
	ext->declare_class_element("SolidBody.theta", &SolidBody::theta_0);
	ext->declare_class_element("SolidBody.g_factor", &SolidBody::g_factor);
	ext->declare_class_element("SolidBody.physics_active", &SolidBody::active);
	ext->declare_class_element("SolidBody.physics_passive", &SolidBody::passive);
	ext->link_class_func("SolidBody.add_force", &SolidBody::add_force);
	ext->link_class_func("SolidBody.add_impulse", &SolidBody::add_impulse);
	ext->link_class_func("SolidBody.add_torque", &SolidBody::add_torque);
	ext->link_class_func("SolidBody.add_torque_impulse", &SolidBody::add_torque_impulse);
	ext->link_class_func("SolidBody.update_motion", &SolidBody::update_motion);
	ext->link_class_func("SolidBody.update_mass", &SolidBody::update_mass);

	ext->declare_class_size("Collider", sizeof(Collider));

	ext->declare_class_size("MultiInstance", sizeof(MultiInstance));
	ext->declare_class_element("MultiInstance.model", &MultiInstance::model);
	ext->declare_class_element("MultiInstance.matrices", &MultiInstance::matrices);


	ext->declare_class_size("Terrain", sizeof(Terrain));
	//ext->declare_class_element("Terrain.pos", &Terrain::pos);
	ext->declare_class_element("Terrain.material", &Terrain::material);
	ext->declare_class_element("Terrain.height", &Terrain::height);
	ext->declare_class_element("Terrain.vertex", &Terrain::vertex);
	ext->declare_class_element("Terrain.normal", &Terrain::normal);
	ext->declare_class_element("Terrain.pattern", &Terrain::pattern);
	ext->declare_class_element("Terrain.num_x", &Terrain::num_x);
	ext->declare_class_element("Terrain.num_z", &Terrain::num_z);
	ext->declare_class_element("Terrain.vertex_shader_module", &Terrain::vertex_shader_module);
	ext->link_class_func("Terrain.update", &Terrain::update);
	ext->link_class_func("Terrain.get_height", &Terrain::gimme_height);

	ext->declare_class_size("CollisionData", sizeof(CollisionData));
	ext->declare_class_element("CollisionData.entity", &CollisionData::entity);
	ext->declare_class_element("CollisionData.body", &CollisionData::body);
	ext->declare_class_element("CollisionData.pos", &CollisionData::pos);
	ext->declare_class_element("CollisionData.n", &CollisionData::n);

	ext->declare_class_size("MaterialPass", sizeof(Material::RenderPassData));
	ext->declare_class_element("MaterialPass.shader_path", &Material::RenderPassData::shader_path);

	ext->declare_class_size("Material", sizeof(Material));
	ext->declare_class_element("Material.textures", &Material::textures);
	ext->declare_class_element("Material.pass0", &Material::pass0);
	ext->declare_class_element("Material.albedo", &Material::albedo);
	ext->declare_class_element("Material.roughness", &Material::roughness);
	ext->declare_class_element("Material.metal", &Material::metal);
	ext->declare_class_element("Material.emission", &Material::emission);
	ext->declare_class_element("Material.cast_shadow", &Material::cast_shadow);
	ext->link_class_func("Material.add_uniform", &Material::add_uniform);


	ext->declare_class_element("World.background", &World::background);
	ext->declare_class_element("World.skyboxes", &World::skybox);
	ext->declare_class_element("World.links", &World::links);
	ext->declare_class_element("World.ego", &World::ego);
	ext->declare_class_element("World.fog", &World::fog);
	ext->declare_class_element("World.gravity", &World::gravity);
	ext->declare_class_element("World.physics_mode", &World::physics_mode);
	ext->declare_class_element("World.msg_data", &World::msg_data);
	ext->link_class_func("World.load_soon", &World::load_soon);
	ext->link_class_func("World.create_object", &_create_object);
	ext->link_class_func("World.create_object_no_reg", &_create_object_no_reg);
	ext->link_class_func("World.create_object_multi", &_create_object_multi);
	ext->link_class_func("World.create_terrain", &World::create_terrain);
	ext->link_class_func("World.create_entity", &World::create_entity);
	ext->link_class_func("World.register_entity", &World::register_entity);
	ext->link_class_func("World.set_active_physics", &World::set_active_physics);
	ext->link_class_func("World.create_light_parallel", &World::create_light_parallel);
	ext->link_class_func("World.create_light_point", &World::create_light_point);
	ext->link_class_func("World.create_light_cone", &World::create_light_cone);
	ext->link_class_func("World.create_camera", &World::create_camera);
	ext->link_class_func("World.attach_model", &_attach_model);
	ext->link_class_func("World.unattach_model", &World::unattach_model);
	ext->link_class_func("World.add_link", &World::add_link);
	ext->link_class_func("World.add_particle", &World::add_legacy_particle);
	ext->link_class_func("World.shift_all", &World::shift_all);
	ext->link_class_func("World.get_g", &World::get_g);
	ext->link_class_func("World.trace", &World::trace);
	ext->link_class_func("World.unregister", &World::unregister);
	ext->link_class_func("World.delete_entity", &World::delete_entity);
	ext->link_class_func("World.delete_particle", &World::delete_legacy_particle);
	ext->link_class_func("World.delete_link", &World::delete_link);
	ext->link_class_func("World.subscribe", &World::subscribe);


	ext->declare_class_element("World.MessageData.e", &World::MessageData::e);
	ext->declare_class_element("World.MessageData.v", &World::MessageData::v);

	ext->declare_class_element("Fog.color", &Fog::_color);
	ext->declare_class_element("Fog.enabled", &Fog::enabled);
	ext->declare_class_element("Fog.distance", &Fog::distance);


	Controller con;
	ext->declare_class_size("Controller", sizeof(Controller));
	ext->link_class_func("Controller.__init__", &Controller::__init__);
	ext->link_virtual("Controller.__delete__", &Controller::__delete__, &con);
	ext->link_virtual("Controller.on_init", &Controller::on_init, &con);
	ext->link_virtual("Controller.on_delete", &Controller::on_delete, &con);
	ext->link_virtual("Controller.on_iterate", &Controller::on_iterate, &con);
	ext->link_virtual("Controller.on_iterate_pre", &Controller::on_iterate_pre, &con);
	ext->link_virtual("Controller.on_draw_pre", &Controller::on_draw_pre, &con);
	ext->link_virtual("Controller.on_input", &Controller::on_input, &con);
	ext->link_virtual("Controller.on_key", &Controller::on_key, &con);
	ext->link_virtual("Controller.on_key_down", &Controller::on_key_down, &con);
	ext->link_virtual("Controller.on_key_up", &Controller::on_key_up, &con);
	ext->link_virtual("Controller.on_left_button_down", &Controller::on_left_button_down, &con);
	ext->link_virtual("Controller.on_left_button_up", &Controller::on_left_button_up, &con);
	ext->link_virtual("Controller.on_middle_button_down", &Controller::on_middle_button_down, &con);
	ext->link_virtual("Controller.on_middle_button_up", &Controller::on_middle_button_up, &con);
	ext->link_virtual("Controller.on_right_button_down", &Controller::on_right_button_down, &con);
	ext->link_virtual("Controller.on_right_button_up", &Controller::on_right_button_up, &con);
	ext->link_virtual("Controller.on_render_inject", &Controller::on_render_inject, &con);
	ext->link_class_func("Controller.__del_override__", &DeletionQueue::add);

#define _OFFSET(VAR, MEMBER)	(char*)&VAR.MEMBER - (char*)&VAR

	Light light(Black, 0, 0);
	ext->declare_class_size("Light", sizeof(Light));
	ext->declare_class_element("Light.dir", _OFFSET(light, light.dir));
	ext->declare_class_element("Light.color", _OFFSET(light, light.col));
	ext->declare_class_element("Light.radius", _OFFSET(light, light.radius));
	ext->declare_class_element("Light.theta", _OFFSET(light, light.theta));
	ext->declare_class_element("Light.harshness", _OFFSET(light, light.harshness));
	ext->declare_class_element("Light.enabled", &Light::enabled);
	ext->declare_class_element("Light.allow_shadow", &Light::allow_shadow);
	ext->declare_class_element("Light.user_shadow_control", &Light::user_shadow_control);
	ext->declare_class_element("Light.user_shadow_theta", &Light::user_shadow_theta);
	ext->declare_class_element("Light.shadow_dist_max", &Light::shadow_dist_max);
	ext->declare_class_element("Light.shadow_dist_min", &Light::shadow_dist_min);
	ext->link_class_func("Light.set_direction", &Light::set_direction);

	/*ext->link_class_func("Light.Parallel.__init__", &Light::__init_parallel__);
	ext->link_class_func("Light.Spherical.__init__", &Light::__init_spherical__);
	ext->link_class_func("Light.Cone.__init__", &Light::__init_cone__);*/

	ext->declare_class_size("Link", sizeof(Link));
	ext->declare_class_element("Link.a", &Link::a);
	ext->declare_class_element("Link.b", &Link::b);
	ext->link_class_func("Link.set_motor", &Link::set_motor);
	ext->link_class_func("Link.set_frame", &Link::set_frame);
	//ext->link_class_func("Link.set_axis", &Link::set_axis);
	ext->link_class_func("Link.__del_override__", &DeletionQueue::add);


	ext->link("__get_component_list", (void*)&ComponentManager::_get_list);
	ext->link("__get_component_family_list", (void*)&ComponentManager::_get_list_family);
	ext->link("__get_component_list2", (void*)&ComponentManager::_get_list2);

	ext->declare_class_size("Particle", sizeof(Particle));
	ext->declare_class_element("Particle.pos", &Particle::pos);
	ext->declare_class_element("Particle.vel", &Particle::vel);
	ext->declare_class_element("Particle.radius", &Particle::radius);
	ext->declare_class_element("Particle.time_to_live", &Particle::time_to_live);
	ext->declare_class_element("Particle.suicidal", &Particle::suicidal);
	ext->declare_class_element("Particle.color", &Particle::col);
	ext->declare_class_element("Particle.enabled", &Particle::enabled);

	ext->declare_class_size("Beam", sizeof(Beam));
	ext->declare_class_element("Beam.length", &Beam::length);


	LegacyParticle particle(vec3::ZERO, 0, nullptr, -1);
	ext->declare_class_size("LegacyParticle", sizeof(LegacyParticle));
	ext->declare_class_element("LegacyParticle.pos", &LegacyParticle::pos);
	ext->declare_class_element("LegacyParticle.vel", &LegacyParticle::vel);
	ext->declare_class_element("LegacyParticle.radius", &LegacyParticle::radius);
	ext->declare_class_element("LegacyParticle.time_to_live", &LegacyParticle::time_to_live);
	ext->declare_class_element("LegacyParticle.suicidal", &LegacyParticle::suicidal);
	ext->declare_class_element("LegacyParticle.texture", &LegacyParticle::texture);
	ext->declare_class_element("LegacyParticle.color", &LegacyParticle::col);
	ext->declare_class_element("LegacyParticle.source", &LegacyParticle::source);
	ext->declare_class_element("LegacyParticle.enabled", &LegacyParticle::enabled);
	ext->link_class_func("LegacyParticle.__init__", &LegacyParticle::__init__);
	ext->link_virtual("LegacyParticle.__delete__", &LegacyParticle::__delete__, &particle);
	//ext->link_virtual("LegacyParticle.on_iterate", &Particle::on_iterate, &particle);
	//ext->link_class_func("LegacyParticle.__del_override__", &global_delete);
	ext->link_class_func("LegacyParticle.__del_override__", &DeletionQueue::add);

	ext->declare_class_size("LegacyBeam", sizeof(LegacyBeam));
	ext->declare_class_element("LegacyBeam.length", &LegacyBeam::length);
	ext->link_class_func("LegacyBeam.__init__", &LegacyBeam::__init_beam__);

	{
	ParticleGroup group;
	ext->declare_class_size("ParticleGroup", sizeof(ParticleGroup));
	ext->declare_class_element("ParticleGroup.source", &ParticleGroup::source);
	ext->link_class_func("ParticleGroup.__init__", &ParticleGroup::__init__);
	ext->link_class_func("ParticleGroup.emit", &ParticleGroup::emit_particle);
	ext->link_class_func("ParticleGroup.emit_beam", &ParticleGroup::emit_beam);
	ext->link_class_func("ParticleGroup.iterate_particles", &ParticleGroup::iterate_particles);
	ext->link_virtual("ParticleGroup.__delete__", &ParticleGroup::__delete__, &group);
	ext->link_virtual("ParticleGroup.on_iterate", &ParticleGroup::on_iterate, &group);
	ext->link_virtual("ParticleGroup.on_iterate_particle", &ParticleGroup::on_iterate_particle, &group);
	ext->link_virtual("ParticleGroup.on_iterate_beam", &ParticleGroup::on_iterate_beam, &group);
	//ext->link_class_func("ParticleGroup.__del_override__", &DeletionQueue::add);
	}

	{
	ParticleEmitter emitter;
	ext->declare_class_size("ParticleEmitter", sizeof(ParticleEmitter));
	ext->declare_class_element("ParticleEmitter.spawn_beams", &ParticleEmitter::spawn_beams);
	ext->declare_class_element("ParticleEmitter.spawn_dt", &ParticleEmitter::spawn_dt);
	ext->declare_class_element("ParticleEmitter.spawn_time_to_live", &ParticleEmitter::spawn_time_to_live);
	ext->link_class_func("ParticleEmitter.__init__", &ParticleEmitter::__init__);
	ext->link_class_func("ParticleEmitter.emit_particle", &ParticleEmitter::emit_particle);
	ext->link_class_func("ParticleEmitter.iterate_emitter", &ParticleEmitter::iterate_emitter);
	ext->link_virtual("ParticleEmitter.__delete__", &ParticleEmitter::__delete__, &emitter);
	ext->link_virtual("ParticleEmitter.on_iterate", &ParticleEmitter::on_iterate, &emitter);
	ext->link_virtual("ParticleEmitter.on_init_particle", &ParticleEmitter::on_init_particle, &emitter);
	ext->link_virtual("ParticleEmitter.on_init_beam", &ParticleEmitter::on_init_beam, &emitter);
	//ext->link_class_func("ParticleEmitter.__del_override__", &DeletionQueue::add);
	}

	ext->declare_class_size("SoundSource", sizeof(audio::SoundSource));
	ext->declare_class_element("SoundSource.loop", &audio::SoundSource::loop);
	ext->declare_class_element("SoundSource.suicidal", &audio::SoundSource::suicidal);
	ext->declare_class_element("SoundSource.min_distance", &audio::SoundSource::min_distance);
	ext->declare_class_element("SoundSource.max_distance", &audio::SoundSource::max_distance);
	ext->declare_class_element("SoundSource.volume", &audio::SoundSource::volume);
	ext->declare_class_element("SoundSource.speed", &audio::SoundSource::speed);
	ext->link_class_func("SoundSource.play", &audio::SoundSource::play);
	ext->link_class_func("SoundSource.stop", &audio::SoundSource::stop);
	ext->link_class_func("SoundSource.pause", &audio::SoundSource::pause);
	ext->link_class_func("SoundSource.has_ended", &audio::SoundSource::has_ended);
	ext->link_class_func("SoundSource.update", &audio::SoundSource::_apply_data);
	ext->link_class_func("SoundSource.set_buffer", &audio::SoundSource::set_buffer);
	ext->link_class_func("SoundSource.__del_override__", &DeletionQueue::add);

	gui::Node node(rect::ID);
	ext->declare_class_size("Node", sizeof(gui::Node));
	ext->declare_class_element("Node.x", &gui::Node::pos);
	ext->declare_class_element("Node.y", _OFFSET(node, pos.y));
	ext->declare_class_element("Node.pos", &gui::Node::pos);
	ext->declare_class_element("Node.width", &gui::Node::width);
	ext->declare_class_element("Node.height", &gui::Node::height);
	ext->declare_class_element("Node._eff_area", &gui::Node::eff_area);
	ext->declare_class_element("Node.margin", &gui::Node::margin);
	ext->declare_class_element("Node.align", &gui::Node::align);
	ext->declare_class_element("Node.dz", &gui::Node::dz);
	ext->declare_class_element("Node.color", &gui::Node::col);
	ext->declare_class_element("Node.visible", &gui::Node::visible);
	ext->declare_class_element("Node.children", &gui::Node::children);
	ext->declare_class_element("Node.parent", &gui::Node::parent);
	ext->link_class_func("Node.__init__", &gui::Node::__init_base__);
	ext->link_virtual("Node.__delete__", &gui::Node::__delete__, &node);
	ext->link_class_func("Node.__del_override__", &DeletionQueue::add);
	ext->link_class_func("Node.add", &gui::Node::add);
	ext->link_class_func("Node.remove", &gui::Node::remove);
	ext->link_class_func("Node.remove_all_children", &gui::Node::remove_all_children);
	ext->link_class_func("Node.set_area", &gui::Node::set_area);
	ext->link_virtual("Node.on_iterate", &gui::Node::on_iterate, &node);
	ext->link_virtual("Node.on_enter", &gui::Node::on_enter, &node);
	ext->link_virtual("Node.on_leave", &gui::Node::on_leave, &node);
	ext->link_virtual("Node.on_left_button_down", &gui::Node::on_left_button_down, &node);
	ext->link_virtual("Node.on_left_button_up", &gui::Node::on_left_button_up, &node);
	ext->link_virtual("Node.on_middle_button_down", &gui::Node::on_middle_button_down, &node);
	ext->link_virtual("Node.on_middle_button_up", &gui::Node::on_middle_button_up, &node);
	ext->link_virtual("Node.on_right_button_down", &gui::Node::on_right_button_down, &node);
	ext->link_virtual("Node.on_right_button_up", &gui::Node::on_right_button_up, &node);

	gui::Picture picture(rect::ID, nullptr);
	ext->declare_class_size("Picture", sizeof(gui::Picture));
	ext->declare_class_element("Picture.source", &gui::Picture::source);
	ext->declare_class_element("Picture.texture", &gui::Picture::texture);
	ext->declare_class_element("Picture.shader", &gui::Picture::shader);
	ext->declare_class_element("Picture.shader_data", &gui::Picture::shader_data);
	ext->declare_class_element("Picture.blur", &gui::Picture::bg_blur);
	ext->declare_class_element("Picture.angle", &gui::Picture::angle);
	ext->link_class_func("Picture.__init__", &gui::Picture::__init__);
	ext->link_virtual("Picture.__delete__", &gui::Picture::__delete__, &picture);

	gui::Text text(":::fake:::", 0, vec2::ZERO);
	ext->declare_class_size("Text", sizeof(gui::Text));
	ext->declare_class_element("Text.font_size", &gui::Text::font_size);
	ext->declare_class_element("Text.text", &gui::Text::text);
	ext->link_class_func("Text.__init__", &gui::Text::__init__);
	ext->link_virtual("Text.__delete__", &gui::Text::__delete__, &text);
	ext->link_class_func("Text.set_text", &gui::Text::set_text);

	ext->link_class_func("HBox.__init__", &gui::HBox::__init__);
	ext->link_class_func("VBox.__init__", &gui::VBox::__init__);

	ext->link("key_state", (void*)&input::get_key);
	ext->link("key_down", (void*)&input::get_key_down);
	ext->link("key_up", (void*)&input::get_key_up);
	ext->link("button", (void*)&input::get_button);
	ext->link("toplevel", &gui::toplevel);
	ext->link("mouse", &input::mouse);
	ext->link("dmouse", &input::dmouse);
	ext->link("scroll", &input::scroll);
	ext->link("link_mouse_and_keyboard_into_pad", &input::link_mouse_and_keyboard_into_pad);
	ext->link("get_pad", (void*)&input::get_pad);

	ext->declare_class_size("Gamepad", sizeof(input::Gamepad));
	ext->declare_class_element("Gamepad.deadzone", &input::Gamepad::deadzone);
	ext->link_class_func("Gamepad.update", &input::Gamepad::update);
	ext->link_class_func("Gamepad.is_present", &input::Gamepad::is_present);
	ext->link_class_func("Gamepad.name", &input::Gamepad::name);
	ext->link_class_func("Gamepad.axis", &input::Gamepad::axis);
	ext->link_class_func("Gamepad.button", &input::Gamepad::button);
	ext->link_class_func("Gamepad.clicked", &input::Gamepad::clicked);

	ext->declare_class_size("NetworkManager", sizeof(NetworkManager));
	ext->declare_class_element("NewtorkManager.cur_con", &NetworkManager::cur_con);
	ext->link_class_func("NetworkManager.connect_to_host", &NetworkManager::connect_to_host);
	ext->link_class_func("NetworkManager.event", &NetworkManager::event);


	ext->declare_class_size("Connection", sizeof(NetworkManager::Connection));
	ext->declare_class_element("Connection.s", &NetworkManager::Connection::s);
	ext->declare_class_element("Connection.buffer", &NetworkManager::Connection::buffer);
	ext->link_class_func("Connection.start_block", &NetworkManager::Connection::start_block);
	ext->link_class_func("Connection.end_block", &NetworkManager::Connection::end_block);
	ext->link_class_func("Connection.send", &NetworkManager::Connection::send);

	ext->link("network", &network_manager);

	ext->declare_class_size("PerformanceMonitor.Channel", sizeof(PerformanceChannel));
	ext->declare_class_element("PerformanceMonitor.Channel.name", &PerformanceChannel::name);
	ext->declare_class_element("PerformanceMonitor.Channel.parent", &PerformanceChannel::parent);
	ext->declare_class_element("PerformanceMonitor.Channel.average", &PerformanceChannel::average);

	ext->declare_class_size("PerformanceMonitor.TimingData", sizeof(TimingData));
	ext->declare_class_element("PerformanceMonitor.TimingData.channel", &TimingData::channel);
	ext->declare_class_element("PerformanceMonitor.TimingData.offset", &TimingData::offset);

	ext->declare_class_size("PerformanceMonitor.FrameTimingData", sizeof(FrameTimingData));
	ext->declare_class_element("PerformanceMonitor.FrameTimingData.cpu0", &FrameTimingData::cpu0);
	ext->declare_class_element("PerformanceMonitor.FrameTimingData.gpu", &FrameTimingData::gpu);
	ext->declare_class_element("PerformanceMonitor.FrameTimingData.total_time", &FrameTimingData::total_time);

	ext->declare_class_size("PerformanceMonitor", sizeof(PerformanceMonitor));
	ext->link("PerformanceMonitor.get_name", (void*)&PerformanceMonitor::get_name);
	ext->link("PerformanceMonitor.avg_frame_time", &PerformanceMonitor::avg_frame_time);
	ext->link("PerformanceMonitor.frames", &PerformanceMonitor::frames);
	ext->link("PerformanceMonitor.channels", &PerformanceMonitor::channels);
	ext->link("PerformanceMonitor.previous_frame_timing", &PerformanceMonitor::previous_frame_timing);
	//ext->link("perf_mon", &global_perf_mon);


	// unused
	ext->declare_class_size("ResourceManager", sizeof(ResourceManager));
	ext->link_class_func("ResourceManager.load_shader", &ResourceManager::load_shader);
	ext->link_class_func("ResourceManager.create_shader", &ResourceManager::create_shader);
	ext->link_class_func("ResourceManager.load_texture", &ResourceManager::load_texture);
	ext->link_class_func("ResourceManager.load_material", &ResourceManager::load_material);
	ext->link_class_func("ResourceManager.load_model", &ResourceManager::load_model);


	ext->declare_class_size("EngineData", sizeof(EngineData));
	ext->declare_class_element("EngineData.app_name", &EngineData::app_name);
	ext->declare_class_element("EngineData.version", &EngineData::version);
	ext->declare_class_element("EngineData.context", &EngineData::context);
	ext->declare_class_element("EngineData.physics_enabled", &EngineData::physics_enabled);
	ext->declare_class_element("EngineData.collisions_enabled", &EngineData::collisions_enabled);
	ext->declare_class_element("EngineData.elapsed", &EngineData::elapsed);
	ext->declare_class_element("EngineData.elapsed_rt", &EngineData::elapsed_rt);
	ext->declare_class_element("EngineData.time_scale", &EngineData::time_scale);
	ext->declare_class_element("EngineData.fps_min", &EngineData::fps_min);
	ext->declare_class_element("EngineData.fps_max", &EngineData::fps_max);
	ext->declare_class_element("EngineData.resolution_scale", &EngineData::resolution_scale_x);
	ext->declare_class_element("EngineData.width", &EngineData::width);
	ext->declare_class_element("EngineData.height", &EngineData::height);
	ext->declare_class_element("EngineData.debug", &EngineData::debug);
	ext->declare_class_element("EngineData.console_enabled", &EngineData::console_enabled);
	ext->declare_class_element("EngineData.wire_mode", &EngineData::wire_mode);
	ext->declare_class_element("EngineData.show_timings", &EngineData::show_timings);
	ext->declare_class_element("EngineData.first_frame", &EngineData::first_frame);
	ext->declare_class_element("EngineData.resetting_game", &EngineData::resetting_game);
	ext->declare_class_element("EngineData.game_running", &EngineData::game_running);
	ext->declare_class_element("EngineData.default_font", &EngineData::default_font);
	ext->declare_class_element("EngineData.detail_level", &EngineData::detail_level);
	ext->declare_class_element("EngineData.initial_world_file", &EngineData::initial_world_file);
	ext->declare_class_element("EngineData.second_world_file", &EngineData::second_world_file);
	ext->declare_class_element("EngineData.physical_aspect_ratio", &EngineData::physical_aspect_ratio);
	ext->declare_class_element("EngineData.window_renderer", &EngineData::window_renderer);
	ext->declare_class_element("EngineData.gui_renderer", &EngineData::gui_renderer);
	ext->declare_class_element("EngineData.region_renderer", &EngineData::region_renderer);
	ext->declare_class_element("EngineData.hdr_renderer", &EngineData::hdr_renderer);
	ext->declare_class_element("EngineData.post_processor", &EngineData::post_processor);
	ext->declare_class_element("EngineData.render_path", &EngineData::world_renderer);
	ext->link_class_func("EngineData.exit", &EngineData::exit);


	ext->declare_class_size("Renderer", sizeof(Renderer));

#ifdef USING_VULKAN
//	using WR = WindowRendererVulkan;
//	using GR = GuiRendererVulkan;
	using RP = WorldRendererVulkan;
	using RPF = WorldRendererVulkanForward;
	using PP = PostProcessorVulkan;
#endif
#ifdef USING_OPENGL
//	using WR = WindowRendererGL;
//	using GR = GuiRendererGL;
	using RP = WorldRendererGL;
	using RPF = WorldRendererGLForward;
	using RPD = WorldRendererGLDeferred;
	using PP = PostProcessorGL;
#endif
	ext->declare_class_size("RenderPath", sizeof(RP));
	ext->declare_class_element("RenderPath.type", &WorldRenderer::type);
	ext->declare_class_element("RenderPath.shader_fx", &WorldRenderer::shader_fx);
	ext->declare_class_element("RenderPath.wireframe", &WorldRenderer::wireframe);
//	ext->link_virtual("RenderPath.render_into_texture", &RPF::render_into_texture, engine.world_renderer);
	ext->link_class_func("RenderPath.render_into_cubemap", &RPF::render_into_cubemap);
	ext->link_class_func("RenderPath.get_cubemap", &world_renderer_get_cubemap);
	ext->link_class_func("RenderPath.get_fb_shadow", &world_renderer_get_fb_shadow);
	ext->link_class_func("RenderPath.get_gbuffer", &world_renderer_get_gbuffer);


	ext->declare_class_size("RegionsRenderer", sizeof(RegionRenderer));
	ext->declare_class_element("RegionRenderer.regions", &RegionRenderer::regions);
	ext->link_class_func("RegionRenderer.add_region", &RegionRenderer::add_region);

	ext->declare_class_size("RegionRenderer.Region", sizeof(RegionRenderer::Region));
	ext->declare_class_element("RegionRenderer.Region.dest", &RegionRenderer::Region::dest);
	ext->declare_class_element("RegionRenderer.Region.z", &RegionRenderer::Region::z);
	ext->declare_class_element("RegionRenderer.Region.renderer", &RegionRenderer::Region::renderer);

	ext->declare_class_size("PostProcessor", sizeof(PP));
	ext->declare_class_element("PostProcessor.fb1", &PP::fb1);
	ext->declare_class_element("PostProcessor.fb2", &PP::fb2);
	ext->link_class_func("PostProcessor.next_fb", &PP::next_fb);
	ext->link_class_func("PostProcessor.process", &PP::process);
	ext->link_class_func("PostProcessor.add_stage", &PP::add_stage);

	ext->declare_class_size("WindowRenderer", sizeof(RP));
	ext->declare_class_element("RenderPath.type", &RP::type);

	ext->declare_class_size("HDRRenderer", sizeof(HDRRenderer));
	ext->declare_class_element("HDRRenderer.fb_main", &HDRRenderer::fb_main);
	ext->link_class_func("HDRRenderer.fb_bloom", &hdr_renderer_get_fb_bloom);


	ext->declare_class_size("FrameBuffer", sizeof(FrameBuffer));
	ext->declare_class_element("FrameBuffer.width", &FrameBuffer::width);
	ext->declare_class_element("FrameBuffer.height", &FrameBuffer::height);
	ext->link_class_func("FrameBuffer.__init__", &framebuffer_init);
	ext->link_class_func("FrameBuffer.depth_buffer", &framebuffer_depthbuffer);
	ext->link_class_func("FrameBuffer.color_attachments", &framebuffer_color_attachments);

	ext->declare_class_size("VertexBuffer", sizeof(VertexBuffer));
	ext->link_class_func("VertexBuffer.__init__", &vertexbuffer_init);
	ext->link_class_func("VertexBuffer.update", &vertexbuffer_update);

	ext->declare_class_size("Texture", sizeof(Texture));
	ext->declare_class_element("Texture.width", &Texture::width);
	ext->declare_class_element("Texture.height", &Texture::height);
	ext->link_class_func("Texture.__init__", &texture_init);
	ext->link_class_func("Texture.__delete__", &texture_delete);
	ext->link_class_func("Texture.write", &texture_write);
	ext->link_class_func("Texture.write_float", &texture_write_float);
	ext->link_class_func("Texture.set_options", &Texture::set_options);

	ext->link_class_func("CubeMap.__init__", &cubemap_init);

	ext->link_class_func("DepthBuffer.__init__", &depthbuffer_init);

	ext->link_class_func("VolumeTexture.__init__", &volumetexture_init);

	ext->link_class_func("Shader.set_float", &shader_set_float);
	ext->link_class_func("Shader.set_floats", &shader_set_floats);


	ext->declare_class_size("TimeTable", sizeof(TimeTable));
	ext->link_class_func("TimeTable.__init__", &generic_init<TimeTable>);
	ext->link_class_func("TimeTable.iterate", &TimeTable::iterate);
	ext->link_class_func("TimeTable.at", &timetable_at);


	ext->link("tex_white", &tex_white);
	ext->link("world", &world);
	ext->link("cam", &cam_main);
	ext->link("engine", &engine);
	ext->link("__get_controller", (void*)&PluginManager::get_controller);
	ext->link("Scheduler.subscribe", (void*)&Scheduler::subscribe);

	ext->link("load_model", (void*)&__load_model);
	ext->link("load_shader", (void*)&__load_shader);
	ext->link("create_shader", (void*)&__create_shader);
	ext->link("load_texture", (void*)&__load_texture);
	ext->link("load_material", (void*)&__load_material);
	ext->link("screenshot", (void*)&screenshot);
	ext->link("create_render_path", (void*)&create_render_path);


	ext->link("load_buffer", (void*)&audio::load_buffer);
	ext->link("create_buffer", (void*)&audio::create_buffer);
	ext->link("emit_sound", (void*)&audio::emit_sound);
	ext->link("emit_sound_file", (void*)&audio::emit_sound_file);
}

template<class C>
void import_component_class(shared<kaba::Module> m, const string &name) {
	for (auto c: m->classes()) {
		if (c->name == name)
			C::_class = c;
	}
	if (!C::_class)
		throw Exception(format("y.kaba: %s missing", name));
	if (!C::_class->is_derived_from_s("ecs.Component"))
		throw Exception(format("y.kaba: %s not derived from Component", name));
}

void PluginManager::import_kaba() {
	auto m_model = kaba::default_context->load_module("y/model.kaba");
	import_component_class<Animator>(m_model, "Animator");
	import_component_class<Skeleton>(m_model, "Skeleton");
	import_component_class<Model>(m_model, "Model");

	auto m_world = kaba::default_context->load_module("y/world.kaba");
	import_component_class<SolidBody>(m_world, "SolidBody");
	import_component_class<Collider>(m_world, "Collider");
	import_component_class<MeshCollider>(m_world, "MeshCollider");
	import_component_class<SphereCollider>(m_world, "SphereCollider");
	import_component_class<BoxCollider>(m_world, "BoxCollider");
	import_component_class<TerrainCollider>(m_world, "TerrainCollider");
	import_component_class<MultiInstance>(m_world, "MultiInstance");
	import_component_class<Terrain>(m_world, "Terrain");
	import_component_class<Light>(m_world, "Light");
	import_component_class<Camera>(m_world, "Camera");

	auto m_fx = kaba::default_context->load_module("y/fx.kaba");
	import_component_class<ParticleGroup>(m_fx, "ParticleGroup");
	import_component_class<ParticleEmitter>(m_fx, "ParticleEmitter");

	auto m_audio = kaba::default_context->load_module("y/audio.kaba");
	import_component_class<audio::SoundSource>(m_audio, "SoundSource");

	auto m_y = kaba::default_context->load_module("y/y.kaba");
	import_component_class<UserMesh>(m_y, "UserMesh");

	//msg_write(MeshCollider::_class->name);
	//msg_write(MeshCollider::_class->parent->name);
	//msg_write(MeshCollider::_class->parent->parent->name);
}

void PluginManager::reset() {
	msg_write("del controller");
	for (auto *c: controllers)
		delete c;
	controllers.clear();
	Scheduler::reset();
}

Array<TemplateDataScriptVariable> parse_variables(const string &var) {
	Array<TemplateDataScriptVariable> r;
	auto xx = var.explode(",");
	for (auto &x: xx) {
		auto y = x.explode(":");
		auto name = y[0].trim().lower().replace("_", "");
		if (y[1].trim().match("\"*\""))
			r.add({name, y[1].trim().sub_ref(1, -1)});
		else
			r.add({name, y[1].trim().unescape()});
	}
	return r;
}

void PluginManager::assign_variables(void *_p, const kaba::Class *c, const Array<TemplateDataScriptVariable> &variables) {
	char *p = (char*)_p;
	for (auto &v: variables) {
		for (auto &e: c->elements) {
			if (v.name == e.name.lower().replace("_", "")) {
				//msg_write("  " + e.type->long_name() + " " + e.name + " = " + v.value);
				if (e.type == kaba::TypeInt32)
					*(int*)(p + e.offset) = v.value._int();
				else if (e.type == kaba::TypeFloat32)
					*(float*)(p + e.offset) = v.value._float();
				else if (e.type == kaba::TypeBool)
					*(bool*)(p + e.offset) = v.value._bool();
				else if (e.type == kaba::TypeString)
					*(string*)(p + e.offset) = v.value;
			}
		}
	}
}

void PluginManager::assign_variables(void *_p, const kaba::Class *c, const string &variables) {
	assign_variables(_p, c, parse_variables(variables));
}

const kaba::Class *PluginManager::find_class_derived(const Path &filename, const string &base_class) {
	//msg_write(format("INSTANCE  %s:   %s", filename, base_class));
	try {
		auto s = kaba::default_context->load_module(filename);
		for (auto c: s->classes()) {
			if (c->is_derived_from_s(base_class)) {
				return c;
			}
		}
	} catch (kaba::Exception &e) {
		msg_error(e.message());
		throw Exception(e.message());
	}
	throw Exception(format("script does not contain a class derived from '%s'", base_class));
	return nullptr;
}

const kaba::Class *PluginManager::find_class(const Path &filename, const string &name) {
	//msg_write(format("INSTANCE  %s:   %s", filename, base_class));
	try {
		auto s = kaba::default_context->load_module(filename);
		for (auto c: s->classes()) {
			if (c->name == name) {
				return c;
			}
		}
	} catch (kaba::Exception &e) {
		msg_error(e.message());
		throw;
	}
	throw Exception(format("script does not contain a class named '%s'", name));
	return nullptr;
}

void *PluginManager::create_instance(const kaba::Class *c, const string &variables) {
	return create_instance(c, parse_variables(variables));
}

void *PluginManager::create_instance(const kaba::Class *c, const Array<TemplateDataScriptVariable> &variables) {
	//msg_write(format("INSTANCE  %s:   %s", filename, base_class));
	msg_write(format("creating instance  %s", c->long_name()));
	if (c == SolidBody::_class)
		return new SolidBody;
	if (c == MeshCollider::_class)
		return new MeshCollider;
	if (c == TerrainCollider::_class)
		return new TerrainCollider;
	if (c == Terrain::_class)
		return new Terrain;
	if (c == Animator::_class)
		return new Animator;
	if (c == Skeleton::_class)
		return new Skeleton;
	if (c == Light::_class)
		return new Light(White, -1, -1);
	if (c == Camera::_class)
		return new Camera;
	if (c == audio::SoundSource::_class)
		return new audio::SoundSource;
	void *p = c->create_instance();
	assign_variables(p, c, variables);
	return p;
}

void *PluginManager::create_instance(const Path &filename, const string &base_class, const Array<TemplateDataScriptVariable> &variables) {
	//msg_write(format("INSTANCE  %s:   %s", filename, base_class));
	auto c = find_class_derived(filename, base_class);
	if (!c)
		return nullptr;
	return create_instance(c, variables);
}

void PluginManager::add_controller(const Path &name, const Array<TemplateDataScriptVariable> &variables) {
	msg_write("add controller: " + name.str());
	auto type = find_class_derived(name, "ui.Controller");;
	auto *c = (Controller*)create_instance(type, variables);
	c->_class = type;
	c->ch_iterate = PerformanceMonitor::create_channel(type->long_name(), ch_controller);

	controllers.add(c);
	c->on_init();
}

Controller *PluginManager::get_controller(const kaba::Class *type) {
	for (auto c: controllers)
		if (c->_class == type)
			return c;
	return nullptr;
}



string callable_name(const void *c) {
	auto t = kaba::default_context->get_dynamic_type((const VirtualBase*)c);
	if (!t)
		return "callable:" + p2s(c);
	static const bool EXTRACT_FUNCTION_NAME = false;
	if (t->is_callable_bind()) {
		if (EXTRACT_FUNCTION_NAME) {
			for (auto &e: t->elements)
				if (e.name == "_fp")
					return "BIND:" + callable_name(*(const char**)((const char*)c + e.offset));
			return "kaba:bind:" + t->name_space->name;
		}
		return t->name_space->owner->module->filename.basename();
	}
	if (t->is_callable_fp()) {
		if (EXTRACT_FUNCTION_NAME) {
			for (auto &e: t->elements)
				if (e.name == "_fp") {
					auto func = *(const kaba::Function**)((const char*)c + e.offset);
					return "FUNC:" + func->long_name();
				}
			return "func:" + t->long_name();
		}
		return t->name_space->owner->module->filename.basename();//relative_to(engine.script_dir).str();
	}
	return "callable:" + p2s(c);
}


