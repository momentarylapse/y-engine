/*
 * PluginManager.cpp
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#include "PluginManager.h"
#include "Controller.h"
#include "../lib/kaba/kaba.h"
#include "../audio/Sound.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../gui/gui.h"
#include "../gui/Node.h"
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../helper/PerformanceMonitor.h"
#include "../helper/ResourceManager.h"
#include "../helper/Scheduler.h"
#include "../input/InputManager.h"
#include "../input/Gamepad.h"
#include "../input/Keyboard.h"
#include "../input/Mouse.h"
#include "../net/NetworkManager.h"
#include "../renderer/base.h"
#include "../renderer/Renderer.h"
#ifdef USING_OPENGL
#include "../renderer/RenderPathGL.h"
#include "../renderer/RenderPathGLForward.h"
#include "../renderer/RenderPathGLDeferred.h"
#include "../renderer/gui/GuiRendererGL.h"
#include "../renderer/post/HDRRendererGL.h"
#include "../renderer/target/WindowRendererGL.h"
#endif
#ifdef USING_VULKAN
#include "../renderer/RenderPathVulkan.h"
#include "../renderer/RenderPathVulkanForward.h"
#include "../renderer/gui/GuiRendererVulkan.h"
#include "../renderer/post/HDRRendererVulkan.h"
#include "../renderer/target/WindowRendererVulkan.h"
#endif
#include "../y/EngineData.h"
#include "../y/Entity.h"
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
#include "../world/Entity3D.h"
#include "../world/components/SolidBody.h"
#include "../world/components/Collider.h"
#include "../world/components/Animator.h"
#include "../world/components/Skeleton.h"
#include "../meta.h"
#include "../graphics-impl.h"
#include "../lib/kaba/dynamic/exception.h"


Array<Controller*> PluginManager::controllers;
int ch_controller = -1;


void global_delete(Entity *e) {
	//msg_error("global delete... " + p2s(e));
	world.unregister(e);
	e->on_delete_rec();
	delete e;
}


#pragma GCC push_options
#pragma GCC optimize("no-omit-frame-pointer")
#pragma GCC optimize("no-inline")
#pragma GCC optimize("0")

Entity3D* _create_object(World *w, const Path &filename, const vector &pos, const quaternion &ang) {
	KABA_EXCEPTION_WRAPPER( return w->create_object(filename, pos, ang); );
	return nullptr;
}

Entity3D* _create_object_no_reg(World *w, const Path &filename, const vector &pos, const quaternion &ang) {
	KABA_EXCEPTION_WRAPPER( return w->create_object_no_reg(filename, pos, ang); );
	return nullptr;
}

Model* _create_object_multi(World *w, const Path &filename, const Array<vector> &pos, const Array<quaternion> &ang) {
	KABA_EXCEPTION_WRAPPER( return w->create_object_multi(filename, pos, ang); );
	return nullptr;
}

void framebuffer_init(FrameBuffer *fb, const Array<Texture*> &tex) {
#ifdef USING_VULKAN
	kaba::kaba_raise_exception(new kaba::KabaException("not implemented: FrameBuffer.__init__()"));
#else
	fb->__init__(tex);
#endif
}

void *framebuffer_depthbuffer(FrameBuffer *fb) {
#ifdef USING_VULKAN
	return fb->attachments.back().get();
#else
	return fb->depth_buffer.get();
#endif
}

Array<Texture*> framebuffer_color_attachments(FrameBuffer *fb) {
#ifdef USING_VULKAN
	return weak(fb->attachments);//.sub_ref(0, -1));
#else
	return weak(fb->color_attachments);
#endif
}

#pragma GCC pop_options


void global_exit(EngineData& engine) {
	msg_error("exit by script...");
	exit(0);
}

void PluginManager::init(int ch_iter) {
	ch_controller = PerformanceMonitor::create_channel("controller", ch_iter);
	export_kaba();
	import_kaba();
}

void PluginManager::export_kaba() {

	Entity entity(Entity::Type::NONE);
	kaba::declare_class_size("Entity", sizeof(Entity));
//	kaba::link_external_class_func("Entity.__init__", &Entity::__init__);
	kaba::link_external_virtual("Entity.__delete__", &Entity::__delete__, &entity);
	kaba::link_external_virtual("Entity.on_init", &Entity::on_init, &entity);
	kaba::link_external_virtual("Entity.on_delete", &Entity::on_delete, &entity);
	kaba::link_external_virtual("Entity.on_iterate", &Entity::on_iterate, &entity);
	kaba::link_external_class_func("Entity.__del_override__", &global_delete);
	kaba::link_external_class_func("Entity.get_component", &Entity::_get_component_untyped_);
	kaba::link_external_class_func("Entity.add_component", &Entity::add_component);

	kaba::declare_class_size("Entity3D", sizeof(Entity3D));
	kaba::declare_class_element("Entity3D.pos", &Entity3D::pos);
	kaba::declare_class_element("Entity3D.ang", &Entity3D::ang);
	kaba::declare_class_element("Entity3D.parent", &Entity3D::parent);
	kaba::link_external_class_func("Entity3D.get_matrix", &Entity3D::get_matrix);

	Component component;
	kaba::declare_class_size("Component", sizeof(Component));
	kaba::declare_class_element("Component.owner", &Component::owner);
	kaba::link_external_class_func("Component.__init__", &Component::__init__);
	kaba::link_external_virtual("Component.__delete__", &Component::__delete__, &component);
	kaba::link_external_virtual("Component.on_init", &Component::on_init, &component);
	kaba::link_external_virtual("Component.on_delete", &Component::on_delete, &component);
	kaba::link_external_virtual("Component.on_iterate", &Component::on_iterate, &component);
	kaba::link_external_virtual("Component.on_collide", &Component::on_collide, &component);
	kaba::link_external_class_func("Component.set_variables", &Component::set_variables);

	Camera _cam(rect::ID);
	kaba::declare_class_size("Camera", sizeof(Camera));
	kaba::declare_class_element("Camera.dest", &Camera::dest);
	kaba::declare_class_element("Camera.fov", &Camera::fov);
	kaba::declare_class_element("Camera.exposure", &Camera::exposure);
	kaba::declare_class_element("Camera.bloom_radius", &Camera::bloom_radius);
	kaba::declare_class_element("Camera.bloom_factor", &Camera::bloom_factor);
	kaba::declare_class_element("Camera.focus_enabled", &Camera::focus_enabled);
	kaba::declare_class_element("Camera.focal_length", &Camera::focal_length);
	kaba::declare_class_element("Camera.focal_blur", &Camera::focal_blur);
	kaba::declare_class_element("Camera.enabled", &Camera::enabled);
	kaba::declare_class_element("Camera.show", &Camera::show);
	kaba::declare_class_element("Camera.min_depth", &Camera::min_depth);
	kaba::declare_class_element("Camera.max_depth", &Camera::max_depth);
	kaba::declare_class_element("Camera.m_view", &Camera::m_view);
	kaba::link_external_class_func("Camera.update_matrices", &Camera::update_matrices);
	kaba::link_external_class_func("Camera.project", &Camera::project);
	kaba::link_external_class_func("Camera.unproject", &Camera::unproject);


	kaba::declare_class_size("Model.Mesh", sizeof(Mesh));
	kaba::declare_class_element("Model.Mesh.bone_index", &Mesh::bone_index);
	kaba::declare_class_element("Model.Mesh.vertex", &Mesh::vertex);
	kaba::declare_class_element("Model.Mesh.sub", &Mesh::sub);

	kaba::declare_class_size("Model.Mesh.Sub", sizeof(SubMesh));
	kaba::declare_class_element("Model.Mesh.Sub.num_triangles", &SubMesh::num_triangles);
	kaba::declare_class_element("Model.Mesh.Sub.triangle_index", &SubMesh::triangle_index);
	kaba::declare_class_element("Model.Mesh.Sub.skin_vertex", &SubMesh::skin_vertex);
	kaba::declare_class_element("Model.Mesh.Sub.normal", &SubMesh::normal);

	Model model;
	kaba::declare_class_size("Model", sizeof(Model));
	kaba::declare_class_element("Model.mesh", &Model::mesh);
	kaba::declare_class_element("Model.materials", &Model::material);
	kaba::declare_class_element("Model.matrix", &Model::_matrix);
	kaba::declare_class_element("Model.radius", (char*)&model.prop.radius - (char*)&model);
	kaba::declare_class_element("Model.min", (char*)&model.prop.min - (char*)&model);
	kaba::declare_class_element("Model.max", (char*)&model.prop.max- (char*)&model);
	kaba::declare_class_element("Model.name", (char*)&model.script_data.name - (char*)&model);
	kaba::link_external_class_func("Model.__init__", &Model::__init__);
	kaba::link_external_virtual("Model.__delete__", &Model::__delete__, &model);
	kaba::link_external_class_func("Model.make_editable", &Model::make_editable);
	kaba::link_external_class_func("Model.begin_edit", &Model::begin_edit);
	kaba::link_external_class_func("Model.end_edit", &Model::end_edit);
	kaba::link_external_class_func("Model.get_vertex", &Model::get_vertex);
	kaba::link_external_class_func("Model.update_matrix", &Model::update_matrix);
//	kaba::link_external_class_func("Model.set_bone_model", &Model::set_bone_model);

	kaba::link_external_virtual("Model.on_init", &Model::on_init, &model);
	kaba::link_external_virtual("Model.on_delete", &Model::on_delete, &model);
	kaba::link_external_virtual("Model.on_iterate", &Model::on_iterate, &model);


	kaba::declare_class_size("Animator", sizeof(Animator));
	kaba::link_external_class_func("Animator.reset", &Animator::reset);
	kaba::link_external_class_func("Animator.add", &Animator::add);
	kaba::link_external_class_func("Animator.add_x", &Animator::add_x);
	kaba::link_external_class_func("Animator.is_done", &Animator::is_done);
	kaba::link_external_class_func("Animator.begin_edit", &Animator::reset); // JUST FOR COMPATIBILITY WITH OLD BRANCH


	kaba::declare_class_size("Skeleton", sizeof(Skeleton));
	kaba::declare_class_element("Skeleton.bones", &Skeleton::bones);
	kaba::declare_class_element("Skeleton.parents", &Skeleton::parents);
	kaba::declare_class_element("Skeleton.dpos", &Skeleton::dpos);
	kaba::declare_class_element("Skeleton.pos0", &Skeleton::pos0);
	kaba::link_external_class_func("Skeleton.reset", &Skeleton::reset);


	kaba::declare_class_size("SolidBody", sizeof(SolidBody));
	kaba::declare_class_element("SolidBody.vel", &SolidBody::vel);
	kaba::declare_class_element("SolidBody.rot", &SolidBody::rot);
	kaba::declare_class_element("SolidBody.mass", &SolidBody::mass);
	//kaba::declare_class_element("SolidBody.theta", &SolidBody::theta_world);
	kaba::declare_class_element("SolidBody.theta", &SolidBody::theta_0);
	kaba::declare_class_element("SolidBody.g_factor", &SolidBody::g_factor);
	kaba::declare_class_element("SolidBody.physics_active", &SolidBody::active);
	kaba::declare_class_element("SolidBody.physics_passive", &SolidBody::passive);
	kaba::link_external_class_func("SolidBody.add_force", &SolidBody::add_force);
	kaba::link_external_class_func("SolidBody.add_impulse", &SolidBody::add_impulse);
	kaba::link_external_class_func("SolidBody.add_torque", &SolidBody::add_torque);
	kaba::link_external_class_func("SolidBody.add_torque_impulse", &SolidBody::add_torque_impulse);
	kaba::link_external_class_func("SolidBody.update_motion", &SolidBody::update_motion);
	kaba::link_external_class_func("SolidBody.update_mass", &SolidBody::update_mass);

	kaba::declare_class_size("Collider", sizeof(Collider));


	kaba::declare_class_size("Terrain", sizeof(Terrain));
	//kaba::declare_class_element("Terrain.pos", &Terrain::pos);
	kaba::link_external_class_func("Terrain.get_height", &Terrain::gimme_height);

	kaba::declare_class_size("CollisionData", sizeof(CollisionData));
	kaba::declare_class_element("CollisionData.sb", &CollisionData::sb);
	kaba::declare_class_element("CollisionData.sub", &CollisionData::sub);
	kaba::declare_class_element("CollisionData.t", &CollisionData::t);
	kaba::declare_class_element("CollisionData.p", &CollisionData::p);
	kaba::declare_class_element("CollisionData.n", &CollisionData::n);

	kaba::declare_class_size("Material", sizeof(Material));
	kaba::declare_class_element("Material.textures", &Material::textures);
	kaba::declare_class_element("Material.shader", &Material::shader);
	kaba::declare_class_element("Material.albedo", &Material::albedo);
	kaba::declare_class_element("Material.roughness", &Material::roughness);
	kaba::declare_class_element("Material.metal", &Material::metal);
	kaba::declare_class_element("Material.emission", &Material::emission);
	kaba::declare_class_element("Material.cast_shadow", &Material::cast_shadow);
	kaba::link_external_class_func("Material.add_uniform", &Material::add_uniform);


	kaba::link_external("Material.load", (void*)&LoadMaterial);


	kaba::declare_class_element("World.objects", &World::objects);
	kaba::declare_class_element("World.terrains", &World::terrains);
	kaba::declare_class_element("World.background", &World::background);
	kaba::declare_class_element("World.skyboxes", &World::skybox);
	kaba::declare_class_element("World.lights", &World::lights);
	kaba::declare_class_element("World.links", &World::links);
	kaba::declare_class_element("World.ego", &World::ego);
	kaba::declare_class_element("World.fog", &World::fog);
	kaba::declare_class_element("World.gravity", &World::gravity);
	kaba::declare_class_element("World.physics_mode", &World::physics_mode);
	kaba::declare_class_element("World.msg_data", &World::msg_data);
	kaba::link_external_class_func("World.load_soon", &World::load_soon);
	kaba::link_external_class_func("World.create_object", &_create_object);
	kaba::link_external_class_func("World.create_object_no_reg", &_create_object_no_reg);
	kaba::link_external_class_func("World.create_object_multi", &_create_object_multi);
	kaba::link_external_class_func("World.create_terrain", &World::create_terrain);
	kaba::link_external_class_func("World.create_entity", &World::create_entity);
	kaba::link_external_class_func("World.register_entity", &World::register_entity);
	kaba::link_external_class_func("World.set_active_physics", &World::set_active_physics);
	kaba::link_external_class_func("World.add_light_parallel", &World::add_light_parallel);
	kaba::link_external_class_func("World.add_light_point", &World::add_light_point);
	kaba::link_external_class_func("World.add_light_cone", &World::add_light_cone);
	kaba::link_external_class_func("World.add_link", &World::add_link);
	kaba::link_external_class_func("World.add_particle", &World::add_particle);
	kaba::link_external_class_func("World.add_sound", &World::add_sound);
	kaba::link_external_class_func("World.shift_all", &World::shift_all);
	kaba::link_external_class_func("World.get_g", &World::get_g);
	kaba::link_external_class_func("World.trace", &World::trace);
	kaba::link_external_class_func("World.delete", &World::_delete);
	kaba::link_external_class_func("World.unregister", &World::unregister);
	kaba::link_external_class_func("World.subscribe", &World::subscribe);


	kaba::declare_class_element("World.MessageData.e", &World::MessageData::e);
	kaba::declare_class_element("World.MessageData.v", &World::MessageData::v);

	kaba::declare_class_element("Fog.color", &Fog::_color);
	kaba::declare_class_element("Fog.enabled", &Fog::enabled);
	kaba::declare_class_element("Fog.distance", &Fog::distance);


	Controller con;
	kaba::declare_class_size("Controller", sizeof(Controller));
	kaba::link_external_class_func("Controller.__init__", &Controller::__init__);
	kaba::link_external_virtual("Controller.__delete__", &Controller::__delete__, &con);
	kaba::link_external_virtual("Controller.on_init", &Controller::on_init, &con);
	kaba::link_external_virtual("Controller.on_delete", &Controller::on_delete, &con);
	kaba::link_external_virtual("Controller.on_iterate", &Controller::on_iterate, &con);
	kaba::link_external_virtual("Controller.on_iterate_pre", &Controller::on_iterate_pre, &con);
	kaba::link_external_virtual("Controller.on_draw_pre", &Controller::on_draw_pre, &con);
	kaba::link_external_virtual("Controller.on_input", &Controller::on_input, &con);
	kaba::link_external_virtual("Controller.on_key", &Controller::on_key, &con);
	kaba::link_external_virtual("Controller.on_key_down", &Controller::on_key_down, &con);
	kaba::link_external_virtual("Controller.on_key_up", &Controller::on_key_up, &con);
	kaba::link_external_virtual("Controller.on_left_button_down", &Controller::on_left_button_down, &con);
	kaba::link_external_virtual("Controller.on_left_button_up", &Controller::on_left_button_up, &con);
	kaba::link_external_virtual("Controller.on_middle_button_down", &Controller::on_middle_button_down, &con);
	kaba::link_external_virtual("Controller.on_middle_button_up", &Controller::on_middle_button_up, &con);
	kaba::link_external_virtual("Controller.on_right_button_down", &Controller::on_right_button_down, &con);
	kaba::link_external_virtual("Controller.on_right_button_up", &Controller::on_right_button_up, &con);
	kaba::link_external_virtual("Controller.on_render_inject", &Controller::on_render_inject, &con);

#define _OFFSET(VAR, MEMBER)	(char*)&VAR.MEMBER - (char*)&VAR

	Light light(Black, 0, 0);
	kaba::declare_class_size("Light", sizeof(Light));
	kaba::declare_class_element("Light.color", _OFFSET(light, light.col));
	kaba::declare_class_element("Light.radius", _OFFSET(light, light.radius));
	kaba::declare_class_element("Light.theta", _OFFSET(light, light.theta));
	kaba::declare_class_element("Light.harshness", _OFFSET(light, light.harshness));
	kaba::declare_class_element("Light.enabled", &Light::enabled);
	kaba::declare_class_element("Light.allow_shadow", &Light::allow_shadow);
	kaba::declare_class_element("Light.user_shadow_control", &Light::user_shadow_control);
	kaba::declare_class_element("Light.user_shadow_theta", &Light::user_shadow_theta);
	kaba::declare_class_element("Light.shadow_dist_max", &Light::shadow_dist_max);
	kaba::declare_class_element("Light.shadow_dist_min", &Light::shadow_dist_min);
	kaba::link_external_class_func("Light.set_direction", &Light::set_direction);

	/*kaba::link_external_class_func("Light.Parallel.__init__", &Light::__init_parallel__);
	kaba::link_external_class_func("Light.Spherical.__init__", &Light::__init_spherical__);
	kaba::link_external_class_func("Light.Cone.__init__", &Light::__init_cone__);*/

	kaba::declare_class_size("Link", sizeof(Link));
	kaba::declare_class_element("Link.a", &Link::a);
	kaba::declare_class_element("Link.b", &Link::b);
	kaba::link_external_class_func("Link.set_motor", &Link::set_motor);
	kaba::link_external_class_func("Link.set_frame", &Link::set_frame);
	//kaba::link_external_class_func("Link.set_axis", &Link::set_axis);


	kaba::link_external("get_component_list", (void*)&ComponentManager::get_list);

	Particle particle(vector::ZERO, 0, nullptr, -1);
	kaba::declare_class_size("Particle", sizeof(Particle));
	kaba::declare_class_element("Particle.pos", &Particle::pos);
	kaba::declare_class_element("Particle.vel", &Particle::vel);
	kaba::declare_class_element("Particle.radius", &Particle::radius);
	kaba::declare_class_element("Particle.time_to_live", &Particle::time_to_live);
	kaba::declare_class_element("Particle.suicidal", &Particle::suicidal);
	kaba::declare_class_element("Particle.texture", &Particle::texture);
	kaba::declare_class_element("Particle.color", &Particle::col);
	kaba::declare_class_element("Particle.source", &Particle::source);
	kaba::declare_class_element("Particle.enabled", &Particle::enabled);
	kaba::link_external_class_func("Particle.__init__", &Particle::__init__);
	kaba::link_external_virtual("Particle.__delete__", &Particle::__delete__, &particle);
	//kaba::link_external_virtual("Particle.on_iterate", &Particle::on_iterate, &particle);
	//kaba::link_external_class_func("Particle.__del_override__", &global_delete);

	kaba::declare_class_size("Beam", sizeof(Beam));
	kaba::declare_class_element("Beam.length", &Beam::length);
	kaba::link_external_class_func("Beam.__init__", &Beam::__init_beam__);


	kaba::declare_class_size("audio.Sound", sizeof(audio::Sound));
	kaba::link_external_class_func("audio.Sound.play", &audio::Sound::play);
	kaba::link_external_class_func("audio.Sound.stop", &audio::Sound::stop);
	kaba::link_external_class_func("audio.Sound.pause", &audio::Sound::pause);
	kaba::link_external_class_func("audio.Sound.has_ended", &audio::Sound::has_ended);
	kaba::link_external_class_func("audio.Sound.set", &audio::Sound::set_data);
	kaba::link_external_class_func("audio.Sound.load", &audio::Sound::load);
	kaba::link_external_class_func("audio.Sound.emit", &audio::Sound::emit);
	kaba::link_external_class_func("audio.Sound.__del_override__", &global_delete);

	gui::Node node(rect::ID);
	kaba::declare_class_size("ui.Node", sizeof(gui::Node));
	kaba::declare_class_element("ui.Node.x", &gui::Node::pos);
	kaba::declare_class_element("ui.Node.y", _OFFSET(node, pos.y));
	kaba::declare_class_element("ui.Node.pos", &gui::Node::pos);
	kaba::declare_class_element("ui.Node.width", &gui::Node::width);
	kaba::declare_class_element("ui.Node.height", &gui::Node::height);
	kaba::declare_class_element("ui.Node._eff_area", &gui::Node::eff_area);
	kaba::declare_class_element("ui.Node.margin", &gui::Node::margin);
	kaba::declare_class_element("ui.Node.align", &gui::Node::align);
	kaba::declare_class_element("ui.Node.dz", &gui::Node::dz);
	kaba::declare_class_element("ui.Node.color", &gui::Node::col);
	kaba::declare_class_element("ui.Node.visible", &gui::Node::visible);
	kaba::declare_class_element("ui.Node.children", &gui::Node::children);
	kaba::declare_class_element("ui.Node.parent", &gui::Node::parent);
	kaba::link_external_class_func("ui.Node.__init__", &gui::Picture::__init__); // argh
	kaba::link_external_virtual("ui.Node.__delete__", &gui::Node::__delete__, &node);
	kaba::link_external_class_func("ui.Node.__del_override__", &gui::delete_node);
	kaba::link_external_class_func("ui.Node.add", &gui::Node::add);
	kaba::link_external_class_func("ui.Node.set_area", &gui::Node::set_area);
	kaba::link_external_virtual("ui.Node.on_iterate", &gui::Node::on_iterate, &node);
	kaba::link_external_virtual("ui.Node.on_enter", &gui::Node::on_enter, &node);
	kaba::link_external_virtual("ui.Node.on_leave", &gui::Node::on_leave, &node);
	kaba::link_external_virtual("ui.Node.on_left_button_down", &gui::Node::on_left_button_down, &node);
	kaba::link_external_virtual("ui.Node.on_left_button_up", &gui::Node::on_left_button_up, &node);
	kaba::link_external_virtual("ui.Node.on_middle_button_down", &gui::Node::on_middle_button_down, &node);
	kaba::link_external_virtual("ui.Node.on_middle_button_up", &gui::Node::on_middle_button_up, &node);
	kaba::link_external_virtual("ui.Node.on_right_button_down", &gui::Node::on_right_button_down, &node);
	kaba::link_external_virtual("ui.Node.on_right_button_up", &gui::Node::on_right_button_up, &node);

	gui::Picture picture(rect::ID, nullptr);
	kaba::declare_class_size("ui.Picture", sizeof(gui::Picture));
	kaba::declare_class_element("ui.Picture.source", &gui::Picture::source);
	kaba::declare_class_element("ui.Picture.texture", &gui::Picture::texture);
	kaba::declare_class_element("ui.Picture.shader", &gui::Picture::shader);
	kaba::declare_class_element("ui.Picture.blur", &gui::Picture::bg_blur);
	kaba::declare_class_element("ui.Picture.angle", &gui::Picture::angle);
	kaba::link_external_class_func("ui.Picture.__init__:Picture:rect:Texture", &gui::Picture::__init2__);
	kaba::link_external_class_func("ui.Picture.__init__:Picture:rect:Texture:rect", &gui::Picture::__init3__);
	kaba::link_external_virtual("ui.Picture.__delete__", &gui::Picture::__delete__, &picture);

	gui::Text text(":::fake:::", 0, vec2::ZERO);
	kaba::declare_class_size("ui.Text", sizeof(gui::Text));
	kaba::declare_class_element("ui.Text.font_size", &gui::Text::font_size);
	kaba::declare_class_element("ui.Text.text", &gui::Text::text);
	kaba::link_external_class_func("ui.Text.__init__:Text:string:float", &gui::Text::__init2__);
	kaba::link_external_class_func("ui.Text.__init__:Text:string:float:vec2", &gui::Text::__init4__);
	kaba::link_external_virtual("ui.Text.__delete__", &gui::Text::__delete__, &text);
	kaba::link_external_class_func("ui.Text.set_text", &gui::Text::set_text);

	kaba::link_external_class_func("ui.HBox.__init__", &gui::HBox::__init__);
	kaba::link_external_class_func("ui.VBox.__init__", &gui::VBox::__init__);

	kaba::link_external("ui.key", (void*)&input::get_key);
	kaba::link_external("ui.key_down", (void*)&input::get_key_down);
	kaba::link_external("ui.key_up", (void*)&input::get_key_up);
	kaba::link_external("ui.button", (void*)&input::get_button);
	kaba::link_external("ui.toplevel", &gui::toplevel);
	kaba::link_external("ui.mouse", &input::mouse);
	kaba::link_external("ui.dmouse", &input::dmouse);
	kaba::link_external("ui.scroll", &input::scroll);
	kaba::link_external("ui.link_mouse_and_keyboard_into_pad", &input::link_mouse_and_keyboard_into_pad);
	kaba::link_external("ui.get_pad", (void*)&input::get_pad);

	kaba::declare_class_size("ui.Gamepad", sizeof(input::Gamepad));
	kaba::declare_class_element("ui.Gamepad.deadzone", &input::Gamepad::deadzone);
	kaba::link_external_class_func("ui.Gamepad.update", &input::Gamepad::update);
	kaba::link_external_class_func("ui.Gamepad.is_present", &input::Gamepad::is_present);
	kaba::link_external_class_func("ui.Gamepad.axis", &input::Gamepad::axis);
	kaba::link_external_class_func("ui.Gamepad.button", &input::Gamepad::button);
	kaba::link_external_class_func("ui.Gamepad.clicked", &input::Gamepad::clicked);

	kaba::declare_class_size("NetworkManager", sizeof(NetworkManager));
	kaba::declare_class_element("NewtorkManager.cur_con", &NetworkManager::cur_con);
	kaba::link_external_class_func("NetworkManager.connect_to_host", &NetworkManager::connect_to_host);
	kaba::link_external_class_func("NetworkManager.event", &NetworkManager::event);


	kaba::declare_class_size("NetworkManager.Connection", sizeof(NetworkManager::Connection));
	kaba::declare_class_element("NetworkManager.Connection.s", &NetworkManager::Connection::s);
	kaba::declare_class_element("NetworkManager.Connection.buffer", &NetworkManager::Connection::buffer);
	kaba::link_external_class_func("NetworkManager.Connection.start_block", &NetworkManager::Connection::start_block);
	kaba::link_external_class_func("NetworkManager.Connection.end_block", &NetworkManager::Connection::end_block);
	kaba::link_external_class_func("NetworkManager.Connection.send", &NetworkManager::Connection::send);

	kaba::link_external("network", &network_manager);

	kaba::declare_class_size("PerformanceMonitor.Channel", sizeof(PerformanceChannel));
	kaba::declare_class_element("PerformanceMonitor.Channel.name", &PerformanceChannel::name);
	kaba::declare_class_element("PerformanceMonitor.Channel.parent", &PerformanceChannel::parent);
	kaba::declare_class_element("PerformanceMonitor.Channel.average", &PerformanceChannel::average);

	kaba::declare_class_size("PerformanceMonitor", sizeof(PerformanceMonitor));
	kaba::link_external("PerformanceMonitor.avg_frame_time", &PerformanceMonitor::avg_frame_time);
	kaba::link_external("PerformanceMonitor.frames", &PerformanceMonitor::frames);
	kaba::link_external("PerformanceMonitor.channels", &PerformanceMonitor::channels);
	//kaba::link_external("perf_mon", &global_perf_mon);

	kaba::declare_class_size("EngineData", sizeof(EngineData));
	kaba::declare_class_element("EngineData.app_name", &EngineData::app_name);
	kaba::declare_class_element("EngineData.version", &EngineData::version);
	kaba::declare_class_element("EngineData.physics_enabled", &EngineData::physics_enabled);
	kaba::declare_class_element("EngineData.collisions_enabled", &EngineData::collisions_enabled);
	kaba::declare_class_element("EngineData.elapsed", &EngineData::elapsed);
	kaba::declare_class_element("EngineData.elapsed_rt", &EngineData::elapsed_rt);
	kaba::declare_class_element("EngineData.time_scale", &EngineData::time_scale);
	kaba::declare_class_element("EngineData.fps_min", &EngineData::fps_min);
	kaba::declare_class_element("EngineData.fps_max", &EngineData::fps_max);
	kaba::declare_class_element("EngineData.resolution_scale", &EngineData::resolution_scale_x);
	kaba::declare_class_element("EngineData.width", &EngineData::width);
	kaba::declare_class_element("EngineData.height", &EngineData::height);
	kaba::declare_class_element("EngineData.debug", &EngineData::debug);
	kaba::declare_class_element("EngineData.console_enabled", &EngineData::console_enabled);
	kaba::declare_class_element("EngineData.wire_mode", &EngineData::wire_mode);
	kaba::declare_class_element("EngineData.show_timings", &EngineData::show_timings);
	kaba::declare_class_element("EngineData.first_frame", &EngineData::first_frame);
	kaba::declare_class_element("EngineData.resetting_game", &EngineData::resetting_game);
	kaba::declare_class_element("EngineData.game_running", &EngineData::game_running);
	kaba::declare_class_element("EngineData.default_font", &EngineData::default_font);
	kaba::declare_class_element("EngineData.detail_level", &EngineData::detail_level);
	kaba::declare_class_element("EngineData.initial_world_file", &EngineData::initial_world_file);
	kaba::declare_class_element("EngineData.second_world_file", &EngineData::second_world_file);
	kaba::declare_class_element("EngineData.physical_aspect_ratio", &EngineData::physical_aspect_ratio);
	kaba::declare_class_element("EngineData.window_renderer", &EngineData::window_renderer);
	kaba::declare_class_element("EngineData.gui_renderer", &EngineData::gui_renderer);
	kaba::declare_class_element("EngineData.hdr_renderer", &EngineData::hdr_renderer);
	kaba::declare_class_element("EngineData.render_path", &EngineData::render_path);
	kaba::link_external_class_func("EngineData.exit", &global_exit);


	kaba::declare_class_size("Renderer", sizeof(Renderer));

#ifdef USING_VULKAN
	using WR = WindowRendererVulkan;
	using HR = HDRRendererVulkan;
	using GR = GuiRendererVulkan;
	using RP = RenderPathVulkan;
	using RPF = RenderPathVulkanForward;
#endif
#ifdef USING_OPENGL
	using WR = WindowRendererGL;
	using HR = HDRRendererGL;
	using GR = GuiRendererGL;
	using RP = RenderPathGL;
	using RPF = RenderPathGLForward;
	using RPD = RenderPathGLDeferred;
#endif
	kaba::declare_class_size("RenderPath", sizeof(RP));
	kaba::declare_class_element("RenderPath.type", &RP::type);
//	kaba::declare_class_element("RenderPath.depth_buffer", &RP::depth_buffer);
	kaba::declare_class_element("RenderPath.cube_map", &RP::cube_map);
	kaba::declare_class_element("RenderPath.fb2", &RP::fb2);
	kaba::declare_class_element("RenderPath.fb3", &RP::fb3);
	kaba::declare_class_element("RenderPath.shader_fx", &RP::shader_fx);
	kaba::declare_class_element("RenderPath.fb_shadow", &RP::fb_shadow);
	kaba::declare_class_element("RenderPath.fb_shadow2", &RP::fb_shadow2);
#ifdef USING_OPENGL
	kaba::declare_class_element("RenderPath.gbuffer", &RPD::gbuffer);
#else
	kaba::declare_class_element("RenderPath.gbuffer", &RP::fb2); // TODO
#endif
	kaba::link_external_virtual("RenderPath.render_into_texture", &RPF::render_into_texture, engine.render_path);
	kaba::link_external_class_func("RenderPath.render_into_cubemap", &RPF::render_into_cubemap);
	kaba::link_external_class_func("RenderPath.next_fb", &RP::next_fb);
	kaba::link_external_class_func("RenderPath.process", &RP::process);
	kaba::link_external_class_func("RenderPath.add_post_processor", &RP::add_post_processor);
	kaba::link_external_class_func("RenderPath.add_fx_injector", &RP::add_fx_injector);


	kaba::declare_class_size("WindowRenderer", sizeof(RP));
	kaba::declare_class_element("RenderPath.type", &RP::type);

	kaba::declare_class_size("HDRRenderer", sizeof(HR));
	kaba::declare_class_element("HDRRenderer.fb_main", &HR::fb_main);
	kaba::declare_class_element("HDRRenderer.fb_small1", &HR::fb_small1);
	kaba::declare_class_element("HDRRenderer.fb_small2", &HR::fb_small2);


	kaba::declare_class_size("FrameBuffer", sizeof(FrameBuffer));
	kaba::declare_class_element("FrameBuffer.width", &FrameBuffer::width);
	kaba::declare_class_element("FrameBuffer.height", &FrameBuffer::height);
	kaba::link_external_class_func("FrameBuffer.__init__", &framebuffer_init);
	kaba::link_external_class_func("FrameBuffer.depth_buffer", &framebuffer_depthbuffer);
	kaba::link_external_class_func("FrameBuffer.color_attachments", &framebuffer_color_attachments);



	kaba::link_external("tex_white", &tex_white);
	kaba::link_external("world", &world);
	kaba::link_external("cam", &cam);
	kaba::link_external("engine", &engine);
	kaba::link_external("load_model", (void*)&ModelManager::load);
	kaba::link_external("load_shader", (void*)&ResourceManager::load_shader);
	kaba::link_external("load_texture", (void*)&ResourceManager::load_texture);
	kaba::link_external("get_controller", (void*)&PluginManager::get_controller);
	kaba::link_external("add_camera", (void*)&add_camera);
	kaba::link_external("Scheduler.subscribe", (void*)&Scheduler::subscribe);
}

template<class C>
void import_component_class(shared<kaba::Script> s, const string &name) {
	for (auto c: s->classes()) {
		if (c->name == name)
			C::_class = c;
	}
	if (!C::_class)
		throw Exception(format("y.kaba: %s missing", name));
	if (!C::_class->is_derived_from_s("y.Component"))
		throw Exception(format("y.kaba: %s not derived from Component", name));
}

void PluginManager::import_kaba() {
	auto s = kaba::load("y.kaba");
	import_component_class<SolidBody>(s, "SolidBody");
	import_component_class<Collider>(s, "Collider");
	import_component_class<MeshCollider>(s, "MeshCollider");
	import_component_class<SphereCollider>(s, "SphereCollider");
	import_component_class<BoxCollider>(s, "BoxCollider");
	import_component_class<TerrainCollider>(s, "TerrainCollider");
	import_component_class<Animator>(s, "Animator");
	import_component_class<Skeleton>(s, "Skeleton");
	import_component_class<Model>(s, "Model");
	import_component_class<Terrain>(s, "Terrain");
	import_component_class<Light>(s, "Light");
	import_component_class<Camera>(s, "Camera");
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
				if (e.type == kaba::TypeInt)
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
		auto s = kaba::load(filename);
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
		auto s = kaba::load(filename);
		for (auto c: s->classes()) {
			if (c->name == name) {
				return c;
			}
		}
	} catch (kaba::Exception &e) {
		msg_error(e.message());
		throw Exception(e.message());
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
		return new Camera(rect::ID);
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
	auto type = find_class_derived(name, "y.Controller");;
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
	auto t = kaba::get_dynamic_type((const VirtualBase*)c);
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
		return t->name_space->owner->script->filename.basename();
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
		return t->name_space->owner->script->filename.basename();//relative_to(engine.script_dir).str();
	}
	return "callable:" + p2s(c);
}


