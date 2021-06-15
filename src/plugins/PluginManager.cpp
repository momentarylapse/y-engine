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
#include "../fx/Light.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../gui/gui.h"
#include "../gui/Node.h"
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../helper/PerformanceMonitor.h"
#include "../helper/ResourceManager.h"
#include "../input/InputManager.h"
#include "../net/NetworkManager.h"
#include "../renderer/RenderPathGL.h"
#include "../renderer/RenderPathGLForward.h"
#include "../y/EngineData.h"
#include "../world/Camera.h"
#include "../world/Link.h"
#include "../world/Model.h"
#include "../world/ModelManager.h"
#include "../world/Object.h"
#include "../world/Terrain.h"
#include "../world/World.h"
#include "../meta.h"
#include "../lib/kaba/dynamic/exception.h"


PluginManager plugin_manager;
PerformanceMonitor *global_perf_mon;


extern nix::Texture *_tex_white;

void global_delete(Entity *e) {
	//msg_error("global delete... " + p2s(e));
	world.unregister(e);
	e->on_delete();
	delete e;
}


#pragma GCC push_options
#pragma GCC optimize("no-omit-frame-pointer")
#pragma GCC optimize("no-inline")
#pragma GCC optimize("0")

Model* _create_object(World *w, const Path &filename, const vector &pos, const quaternion &ang) {
	KABA_EXCEPTION_WRAPPER( return w->create_object(filename, pos, ang); );
	return nullptr;
}
Model* _create_object_multi(World *w, const Path &filename, const Array<vector> &pos, const Array<quaternion> &ang) {
	KABA_EXCEPTION_WRAPPER( return w->create_object_multi(filename, pos, ang); );
	return nullptr;
}

#pragma GCC pop_options


void global_exit(EngineData& engine) {
	msg_error("exit by script...");
	exit(0);
}

void PluginManager::link_kaba() {

	Camera _cam(v_0, quaternion::ID, rect::ID);
	kaba::declare_class_size("Camera", sizeof(Camera));
	kaba::declare_class_element("Camera.pos", &Camera::pos);
	kaba::declare_class_element("Camera.ang", &Camera::ang);
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
	kaba::link_external_virtual("Camera.__delete__", &Camera::__delete__, &_cam);
	kaba::link_external_virtual("Camera.on_init", &Camera::on_init, &_cam);
	kaba::link_external_virtual("Camera.on_delete", &Camera::on_delete, &_cam);
	kaba::link_external_virtual("Camera.on_iterate", &Camera::on_iterate, &_cam);


	kaba::declare_class_size("Model.Mesh", sizeof(Mesh));
	kaba::declare_class_element("Model.Mesh.bone_index", &Mesh::bone_index);
	kaba::declare_class_element("Model.Mesh.vertex", &Mesh::vertex);
	kaba::declare_class_element("Model.Mesh.sub", &Mesh::sub);

	kaba::declare_class_size("Model.Mesh.Sub", sizeof(SubMesh));
	kaba::declare_class_element("Model.Mesh.Sub.num_triangles", &SubMesh::num_triangles);
	kaba::declare_class_element("Model.Mesh.Sub.triangle_index", &SubMesh::triangle_index);
	kaba::declare_class_element("Model.Mesh.Sub.skin_vertex", &SubMesh::skin_vertex);
	kaba::declare_class_element("Model.Mesh.Sub.normal", &SubMesh::normal);

	kaba::declare_class_size("Model.Bone", sizeof(Bone));
	kaba::declare_class_element("Model.Bone.parent", &Bone::parent);
	kaba::declare_class_element("Model.Bone.pos", &Bone::delta_pos);
	kaba::declare_class_element("Model.Bone.model", &Bone::model);
	kaba::declare_class_element("Model.Bone.dmatrix", &Bone::dmatrix);
	kaba::declare_class_element("Model.Bone.cur_ang", &Bone::cur_ang);
	kaba::declare_class_element("Model.Bone.cur_pos", &Bone::cur_pos);

	Model model;
	kaba::declare_class_size("Model", sizeof(Model));
	kaba::declare_class_element("Model.pos", &Model::pos);
	kaba::declare_class_element("Model.vel", &Model::vel);
	kaba::declare_class_element("Model.ang", &Model::ang);
	kaba::declare_class_element("Model.rot", &Model::rot);
	kaba::declare_class_element("Model.mesh", &Model::mesh);
	kaba::declare_class_element("Model.materials", &Model::material);
	kaba::declare_class_element("Model.bones", &Model::bone);
	kaba::declare_class_element("Model.matrix", &Model::_matrix);
	kaba::declare_class_element("Model.radius", (char*)&model.prop.radius - (char*)&model);
	kaba::declare_class_element("Model.min", (char*)&model.prop.min - (char*)&model);
	kaba::declare_class_element("Model.max", (char*)&model.prop.max- (char*)&model);
	kaba::declare_class_element("Model.name", (char*)&model.script_data.name - (char*)&model);
	kaba::declare_class_element("Model.vars", (char*)&model.script_data.var - (char*)&model);
	kaba::declare_class_element("Model.vars_i", (char*)&model.script_data.var - (char*)&model);
	kaba::declare_class_element("Model.mass", (char*)&model.physics_data.mass - (char*)&model);
	kaba::declare_class_element("Model.theta", (char*)&model.physics_data.theta_0 - (char*)&model);
	kaba::declare_class_element("Model.g_factor", (char*)&model.physics_data.g_factor - (char*)&model);
	kaba::declare_class_element("Model.physics_active", (char*)&model.physics_data.active - (char*)&model);
	kaba::declare_class_element("Model.physics_passive", (char*)&model.physics_data.passive - (char*)&model);
	kaba::declare_class_element("Model.parent", &Model::parent);
	kaba::link_external_class_func("Model.__init__", &Model::__init__);
	kaba::link_external_virtual("Model.__delete__", &Model::__delete__, &model);
	kaba::link_external_class_func("Model.make_editable", &Model::make_editable);
	kaba::link_external_class_func("Model.begin_edit", &Model::begin_edit);
	kaba::link_external_class_func("Model.end_edit", &Model::end_edit);
	kaba::link_external_class_func("Model.update_motion", &Object::update_motion);
	kaba::link_external_class_func("Model.update_mass", &Object::update_mass);
	kaba::link_external_class_func("Model.get_vertex", &Model::get_vertex);
//	kaba::link_external_class_func("Model.set_bone_model", &Model::set_bone_model);
	kaba::link_external_class_func("Model.reset_animation", &Model::reset_animation);
	kaba::link_external_class_func("Model.animate", &Model::animate);
	kaba::link_external_class_func("Model.animate_x", &Model::animate_x);
	kaba::link_external_class_func("Model.is_animation_done", &Model::is_animation_done);
	kaba::link_external_class_func("Model.begin_edit_animation", &Model::begin_edit_animation);

	kaba::link_external_class_func("Model.add_force", &Object::add_force);
	kaba::link_external_class_func("Model.add_impulse", &Object::add_impulse);
	kaba::link_external_class_func("Model.add_torque", &Object::add_torque);
	kaba::link_external_class_func("Model.add_torque_impulse", &Object::add_torque_impulse);
	kaba::link_external_virtual("Model.on_init", &Model::on_init, &model);
	kaba::link_external_virtual("Model.on_delete", &Model::on_delete, &model);
	kaba::link_external_virtual("Model.on_collide", &Model::on_collide, &model);
	kaba::link_external_virtual("Model.on_iterate", &Model::on_iterate, &model);


	kaba::declare_class_size("Terrain", sizeof(Terrain));
	kaba::declare_class_element("Terrain.pos", &Terrain::pos);
	kaba::link_external_class_func("Terrain.get_height", &Terrain::gimme_height);

	kaba::declare_class_size("CollisionData", sizeof(CollisionData));
	kaba::declare_class_element("CollisionData.m", &CollisionData::m);
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
	kaba::link_external_class_func("World.create_object", &_create_object);
	kaba::link_external_class_func("World.create_object_multi", &_create_object_multi);
	kaba::link_external_class_func("World.create_terrain", &World::create_terrain);
	kaba::link_external_class_func("World.set_active_physics", &World::set_active_physics);
	kaba::link_external_class_func("World.add_light", &World::add_light);
	kaba::link_external_class_func("World.add_particle", &World::add_particle);
	kaba::link_external_class_func("World.add_sound", &World::add_sound);
	kaba::link_external_class_func("World.shift_all", &World::shift_all);
	kaba::link_external_class_func("World.get_g", &World::get_g);
	kaba::link_external_class_func("World.trace", &World::trace);
	kaba::link_external_class_func("World.delete", &World::_delete);
	kaba::link_external_class_func("World.unregister", &World::unregister);

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

	Light light(v_0, v_0, Black, 0, 0);
	kaba::declare_class_size("Light", sizeof(Light));
	kaba::declare_class_element("Light.pos", _OFFSET(light, light.pos));
	kaba::declare_class_element("Light.dir", _OFFSET(light, light.dir));
	kaba::declare_class_element("Light.color", _OFFSET(light, light.col));
	kaba::declare_class_element("Light.radius", _OFFSET(light, light.radius));
	kaba::declare_class_element("Light.theta", _OFFSET(light, light.theta));
	kaba::declare_class_element("Light.harshness", _OFFSET(light, light.harshness));
	kaba::declare_class_element("Light.enabled", &Light::enabled);
	kaba::declare_class_element("Light.allow_shadow", &Light::allow_shadow);
	kaba::declare_class_element("Light.user_shadow_control", &Light::user_shadow_control);
	kaba::declare_class_element("Light.user_shadow_theta", &Light::user_shadow_theta);

	kaba::link_external_class_func("Light.Parallel.__init__", &Light::__init_parallel__);
	kaba::link_external_class_func("Light.Spherical.__init__", &Light::__init_spherical__);
	kaba::link_external_class_func("Light.Cone.__init__", &Light::__init_cone__);

	kaba::declare_class_size("Link", sizeof(Link));
	kaba::declare_class_element("Link.a", &Link::a);
	kaba::declare_class_element("Link.b", &Link::b);
	kaba::link_external_class_func("Link.set_motor", &Link::set_motor);
	kaba::link_external_class_func("Link.set_frame", &Link::set_frame);
	//kaba::link_external_class_func("Link.set_axis", &Link::set_axis);

	Entity entity(Entity::Type::NONE);
	kaba::declare_class_size("Entity", sizeof(Entity));
//	kaba::link_external_class_func("Entity.__init__", &Entity::__init__);
	kaba::link_external_virtual("Entity.__delete__", &Entity::__delete__, &entity);
	kaba::link_external_virtual("Entity.on_init", &Entity::on_init, &entity);
	kaba::link_external_virtual("Entity.on_delete", &Entity::on_delete, &entity);
	kaba::link_external_virtual("Entity.on_iterate", &Entity::on_iterate, &entity);
	kaba::link_external_class_func("Entity.__del_override__", &global_delete);

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
	kaba::declare_class_element("ui.Node.x", &gui::Node::x);
	kaba::declare_class_element("ui.Node.y", &gui::Node::y);
	kaba::declare_class_element("ui.Node.width", &gui::Node::width);
	kaba::declare_class_element("ui.Node.height", &gui::Node::height);
	kaba::declare_class_element("ui.Node._eff_area", &gui::Node::eff_area);
	kaba::declare_class_element("ui.Node.margin", &gui::Node::margin);
	kaba::declare_class_element("ui.Node.align", &gui::Node::align);
	kaba::declare_class_element("ui.Node.dz", &gui::Node::dz);
	kaba::declare_class_element("ui.Node.color", &gui::Node::col);
	kaba::declare_class_element("ui.Node.visible", &gui::Node::visible);
	kaba::declare_class_element("ui.Node.children", &gui::Node::children);
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

	gui::Text text(":::fake:::", 0, 0, 0);
	kaba::declare_class_size("ui.Text", sizeof(gui::Text));
	kaba::declare_class_element("ui.Text.font_size", &gui::Text::font_size);
	kaba::declare_class_element("ui.Text.text", &gui::Text::text);
	kaba::link_external_class_func("ui.Text.__init__:Text:string:float", &gui::Text::__init2__);
	kaba::link_external_class_func("ui.Text.__init__:Text:string:float:float:float", &gui::Text::__init4__);
	kaba::link_external_virtual("ui.Text.__delete__", &gui::Text::__delete__, &text);
	kaba::link_external_class_func("ui.Text.set_text", &gui::Text::set_text);

	kaba::link_external_class_func("ui.HBox.__init__", &gui::HBox::__init__);
	kaba::link_external_class_func("ui.VBox.__init__", &gui::VBox::__init__);

	kaba::link_external("ui.key", (void*)&InputManager::get_key);
	kaba::link_external("ui.key_down", (void*)&InputManager::get_key_down);
	kaba::link_external("ui.key_up", (void*)&InputManager::get_key_up);
	kaba::link_external("ui.toplevel", &gui::toplevel);
	kaba::link_external("ui.mouse", &InputManager::mouse);
	kaba::link_external("ui.dmouse", &InputManager::dmouse);
	kaba::link_external("ui.scroll", &InputManager::scroll);

	kaba::declare_class_size("NetworkManager", sizeof(NetworkManager));
	kaba::declare_class_element("NewtorkManager.cur_con", &NetworkManager::cur_con);
	kaba::link_external_class_func("NetworkManager.connect_to_host", &NetworkManager::connect_to_host);
	kaba::link_external_class_func("NetworkManager.event", &NetworkManager::event_kaba);


	kaba::declare_class_size("NetworkManager.Connection", sizeof(NetworkManager::Connection));
	kaba::declare_class_element("NetworkManager.Connection.s", &NetworkManager::Connection::s);
	kaba::declare_class_element("NetworkManager.Connection.buffer", &NetworkManager::Connection::buffer);
	kaba::link_external_class_func("NetworkManager.Connection.start_block", &NetworkManager::Connection::start_block);
	kaba::link_external_class_func("NetworkManager.Connection.end_block", &NetworkManager::Connection::end_block);
	kaba::link_external_class_func("NetworkManager.Connection.send", &NetworkManager::Connection::send);

	kaba::link_external("network", &network_manager);

	kaba::declare_class_size("PerformanceMonitor", sizeof(PerformanceMonitor));
	kaba::declare_class_element("PerformanceMonitor.avg", &PerformanceMonitor::avg);
	kaba::declare_class_element("PerformanceMonitor.frames", &PerformanceMonitor::frames);
	//kaba::declare_class_element("PerformanceMonitor.location", &PerformanceMonitor::avg.location);
	kaba::link_external("perf_mon", &global_perf_mon);

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
	kaba::declare_class_element("EngineData.renderer", &EngineData::renderer);
	kaba::link_external_class_func("EngineData.exit", &global_exit);

	kaba::declare_class_size("RenderPath", sizeof(RenderPathGL));
	kaba::declare_class_element("RenderPath.depth_buffer", &RenderPathGL::depth_buffer);
	kaba::declare_class_element("RenderPath.cube_map", &RenderPathGL::cube_map);
	kaba::declare_class_element("RenderPath.fb_main", &RenderPathGL::fb_main);
	kaba::declare_class_element("RenderPath.fb2", &RenderPathGL::fb2);
	kaba::declare_class_element("RenderPath.fb3", &RenderPathGL::fb3);
	kaba::declare_class_element("RenderPath.fb_small1", &RenderPathGL::fb_small1);
	kaba::declare_class_element("RenderPath.fb_small2", &RenderPathGL::fb_small2);
	kaba::link_external_virtual("RenderPath.render_into_texture", &RenderPathGLForward::render_into_texture, engine.renderer);
	kaba::link_external_class_func("RenderPath.render_into_cubemap", &RenderPathGLForward::render_into_cubemap);
	kaba::link_external_class_func("RenderPath.next_fb", &RenderPathGL::next_fb);
	kaba::link_external_class_func("RenderPath.process", &RenderPathGL::process);
	kaba::link_external_class_func("RenderPath.add_post_processor", &RenderPathGL::kaba_add_post_processor);

	kaba::link_external("tex_white", &_tex_white);
	kaba::link_external("world", &world);
	kaba::link_external("cam", &cam);
	kaba::link_external("engine", &engine);
	kaba::link_external("load_model", (void*)&ModelManager::load);
	kaba::link_external("load_shader", (void*)&ResourceManager::load_shader);
	kaba::link_external("load_texture", (void*)&ResourceManager::load_texture);
}

void PluginManager::reset() {
	controllers.clear();
}

void assign_variables(char *p, const kaba::Class *c, const Array<TemplateDataScriptVariable> &variables) {
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

void *PluginManager::create_instance(const Path &filename, const string &base_class, const Array<TemplateDataScriptVariable> &variables) {
	//msg_write(format("INSTANCE  %s:   %s", filename, base_class));
	try {
		auto s = kaba::load(filename);
		for (auto c: s->classes()) {
			if (c->is_derived_from_s(base_class)) {
				msg_write(format("creating instance  %s", c->long_name()));
				void *p = c->create_instance();
				assign_variables((char*)p, c, variables);
				return p;
			}
		}
	} catch (kaba::Exception &e) {
		msg_error(e.message());
		throw Exception(e.message());
	}
	throw Exception(format("script does not contain a class derived from '%s'", base_class));
	return nullptr;
}

void PluginManager::add_controller(const Path &name, const Array<TemplateDataScriptVariable> &variables) {
	msg_write("add controller: " + name.str());
	auto *c = (Controller*)create_instance(name, "y.Controller", variables);

	controllers.add(c);
	c->on_init();
}

void PluginManager::handle_iterate(float dt) {
	for (auto *c: controllers)
		c->on_iterate(dt);
}

void PluginManager::handle_iterate_pre(float dt) {
	for (auto *c: controllers)
		c->on_iterate_pre(dt);
}

void PluginManager::handle_input() {
	for (auto *c: controllers)
		c->on_input();
}

void PluginManager::handle_draw_pre() {
	for (auto *c: controllers)
		c->on_draw_pre();
}

void PluginManager::handle_render_inject() {
	for (auto *c: controllers)
		c->on_render_inject();
}

void PluginManager::handle_render_inject2() {
	for (auto *c: controllers)
		c->on_render_inject2();
}

