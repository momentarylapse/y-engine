/*
 * PluginManager.cpp
 *
 *  Created on: 02.01.2020
 *      Author: michi
 */

#include "PluginManager.h"
#include "Controller.h"
#include "../lib/kaba/kaba.h"
#include "../world/world.h"
#include "../world/model.h"
#include "../world/ModelManager.h"
#include "../world/terrain.h"
#include "../world/camera.h"
#include "../fx/Light.h"
#include "../fx/Particle.h"
#include "../fx/Beam.h"
#include "../gui/gui.h"
#include "../gui/Node.h"
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../helper/InputManager.h"
#include "../helper/PerformanceMonitor.h"


PluginManager plugin_manager;
PerformanceMonitor *global_perf_mon;


extern nix::Texture *_tex_white;


void PluginManager::link_kaba() {

	Camera _cam(v_0, quaternion::ID, rect::ID);
	Kaba::declare_class_size("Camera", sizeof(Camera));
	Kaba::declare_class_element("Camera.pos", &Camera::pos);
	Kaba::declare_class_element("Camera.ang", &Camera::ang);
	Kaba::declare_class_element("Camera.dest", &Camera::dest);
	Kaba::declare_class_element("Camera.fov", &Camera::fov);
	Kaba::declare_class_element("Camera.exposure", &Camera::exposure);
	Kaba::declare_class_element("Camera.bloom_radius", &Camera::bloom_radius);
	Kaba::declare_class_element("Camera.bloom_factor", &Camera::bloom_factor);
	Kaba::declare_class_element("Camera.focus_enabled", &Camera::focus_enabled);
	Kaba::declare_class_element("Camera.focal_length", &Camera::focal_length);
	Kaba::declare_class_element("Camera.focal_blur", &Camera::focal_blur);
	Kaba::declare_class_element("Camera.enabled", &Camera::enabled);
	Kaba::declare_class_element("Camera.show", &Camera::show);
	Kaba::declare_class_element("Camera.min_depth", &Camera::min_depth);
	Kaba::declare_class_element("Camera.max_depth", &Camera::max_depth);
	Kaba::declare_class_element("Camera.m_view", &Camera::m_view);
	Kaba::link_external_class_func("Camera.update_matrices", &Camera::update_matrices);
	Kaba::link_external_class_func("Camera.project", &Camera::project);
	Kaba::link_external_class_func("Camera.unproject", &Camera::unproject);
	Kaba::link_external_virtual("Camera.__delete__", &Camera::__delete__, &_cam);
	Kaba::link_external_virtual("Camera.on_init", &Camera::on_init, &_cam);
	Kaba::link_external_virtual("Camera.on_delete", &Camera::on_delete, &_cam);
	Kaba::link_external_virtual("Camera.on_iterate", &Camera::on_iterate, &_cam);


	Kaba::declare_class_size("Model.Mesh", sizeof(Mesh));
	Kaba::declare_class_element("Model.Mesh.bone_index", &Mesh::bone_index);
	Kaba::declare_class_element("Model.Mesh.vertex", &Mesh::vertex);
	Kaba::declare_class_element("Model.Mesh.sub", &Mesh::sub);

	Kaba::declare_class_size("Model.Mesh.Sub", sizeof(SubMesh));
	Kaba::declare_class_element("Model.Mesh.Sub.num_triangles", &SubMesh::num_triangles);
	Kaba::declare_class_element("Model.Mesh.Sub.triangle_index", &SubMesh::triangle_index);
	Kaba::declare_class_element("Model.Mesh.Sub.skin_vertex", &SubMesh::skin_vertex);
	Kaba::declare_class_element("Model.Mesh.Sub.normal", &SubMesh::normal);

	Kaba::declare_class_size("Model.Bone", sizeof(Bone));
	Kaba::declare_class_element("Model.Bone.parent", &Bone::parent);
	Kaba::declare_class_element("Model.Bone.pos", &Bone::pos);
	Kaba::declare_class_element("Model.Bone.model", &Bone::model);
	Kaba::declare_class_element("Model.Bone.dmatrix", &Bone::dmatrix);
	Kaba::declare_class_element("Model.Bone.cur_ang", &Bone::cur_ang);
	Kaba::declare_class_element("Model.Bone.cur_pos", &Bone::cur_pos);

	Model model;
	Kaba::declare_class_size("Model", sizeof(Model));
	Kaba::declare_class_element("Model.pos", &Model::pos);
	Kaba::declare_class_element("Model.vel", &Model::vel);
	Kaba::declare_class_element("Model.ang", &Model::ang);
	Kaba::declare_class_element("Model.rot", &Model::rot);
	Kaba::declare_class_element("Model.mesh", &Model::mesh);
	Kaba::declare_class_element("Model.materials", &Model::material);
	Kaba::declare_class_element("Model.bones", &Model::bone);
	Kaba::declare_class_element("Model.matrix", &Model::_matrix);
	Kaba::declare_class_element("Model.var", (char*)&model.script_data.var - (char*)&model);
	Kaba::declare_class_element("Model.var_i", (char*)&model.script_data.var - (char*)&model);
	Kaba::declare_class_element("Model.mass", (char*)&model.physics_data.mass - (char*)&model);
	Kaba::declare_class_element("Model.parent", &Model::parent);
	Kaba::link_external_class_func("Model.make_editable", &Model::make_editable);
	Kaba::link_external_class_func("Model.begin_edit", &Model::begin_edit);
	Kaba::link_external_class_func("Model.end_edit", &Model::end_edit);
	Kaba::link_external_class_func("Model.edit_motion", &Model::edit_motion);
	Kaba::link_external_class_func("Model.__init__", &Model::__init__);
	Kaba::link_external_virtual("Model.__delete__", &Model::__delete__, &model);
	Kaba::link_external_virtual("Model.on_init", &Model::on_init, &model);
	Kaba::link_external_virtual("Model.on_delete", &Model::on_delete, &model);
	Kaba::link_external_virtual("Model.on_collide_m", &Model::on_collide_m, &model);
	Kaba::link_external_virtual("Model.on_collide_t", &Model::on_collide_t, &model);
	Kaba::link_external_virtual("Model.on_iterate", &Model::on_iterate, &model);


	Kaba::declare_class_element("Material.textures", &Material::textures);
	Kaba::declare_class_element("Material.shader", &Material::shader);
	Kaba::declare_class_element("Material.ambient", &Material::ambient);
	Kaba::declare_class_element("Material.diffuse", &Material::diffuse);
	Kaba::declare_class_element("Material.specular", &Material::specular);
	Kaba::declare_class_element("Material.shininess", &Material::shininess);
	Kaba::declare_class_element("Material.emission", &Material::emission);


	Kaba::declare_class_element("World.objects", &World::objects);
	Kaba::declare_class_element("World.terrains", &World::terrains);
	Kaba::declare_class_element("World.background", &World::background);
	Kaba::declare_class_element("World.skyboxes", &World::skybox);
	Kaba::declare_class_element("World.lights", &World::lights);
	Kaba::declare_class_element("World.links", &World::links);
	Kaba::declare_class_element("World.ego", &World::ego);
	Kaba::declare_class_element("World.fog", &World::fog);
	Kaba::declare_class_element("World.gravity", &World::gravity);
	Kaba::declare_class_element("World.physics_mode", &World::physics_mode);
	Kaba::link_external_class_func("World.create_object", &World::create_object);
	Kaba::link_external_class_func("World.create_terrain", &World::create_terrain);
	Kaba::link_external_class_func("World.add_light", &World::add_light);
	Kaba::link_external_class_func("World.add_particle", &World::add_particle);
	Kaba::link_external_class_func("World.shift_all", &World::shift_all);
	Kaba::link_external_class_func("World.get_g", &World::get_g);

	Kaba::declare_class_element("Fog.color", &Fog::_color);
	Kaba::declare_class_element("Fog.enabled", &Fog::enabled);
	Kaba::declare_class_element("Fog.distance", &Fog::distance);


	Controller con;
	Kaba::declare_class_size("Controller", sizeof(Controller));
	Kaba::link_external_class_func("Controller.__init__", &Controller::__init__);
	Kaba::link_external_virtual("Controller.__delete__", &Controller::__delete__, &con);
	Kaba::link_external_virtual("Controller.on_init", &Controller::on_init, &con);
	Kaba::link_external_virtual("Controller.on_delete", &Controller::on_delete, &con);
	Kaba::link_external_virtual("Controller.on_iterate", &Controller::on_iterate, &con);
	Kaba::link_external_virtual("Controller.on_iterate_pre", &Controller::on_iterate_pre, &con);
	Kaba::link_external_virtual("Controller.on_draw_pre", &Controller::on_draw_pre, &con);
	Kaba::link_external_virtual("Controller.on_input", &Controller::on_input, &con);
	Kaba::link_external_virtual("Controller.on_key", &Controller::on_key, &con);
	Kaba::link_external_virtual("Controller.on_key_down", &Controller::on_key_down, &con);
	Kaba::link_external_virtual("Controller.on_key_up", &Controller::on_key_up, &con);
	Kaba::link_external_virtual("Controller.on_left_button_down", &Controller::on_left_button_down, &con);
	Kaba::link_external_virtual("Controller.on_left_button_up", &Controller::on_left_button_up, &con);
	Kaba::link_external_virtual("Controller.on_middle_button_down", &Controller::on_middle_button_down, &con);
	Kaba::link_external_virtual("Controller.on_middle_button_up", &Controller::on_middle_button_up, &con);
	Kaba::link_external_virtual("Controller.on_right_button_down", &Controller::on_right_button_down, &con);
	Kaba::link_external_virtual("Controller.on_right_button_up", &Controller::on_right_button_up, &con);


	Kaba::declare_class_size("Light", sizeof(Light));
	Kaba::declare_class_element("Light.pos", &Light::pos);
	Kaba::declare_class_element("Light.dir", &Light::dir);
	Kaba::declare_class_element("Light.color", &Light::col);
	Kaba::declare_class_element("Light.radius", &Light::radius);
	Kaba::declare_class_element("Light.theta", &Light::theta);
	Kaba::declare_class_element("Light.harshness", &Light::harshness);
	Kaba::declare_class_element("Light.enabled", &Light::enabled);

	Kaba::link_external_class_func("Light.Parallel.__init__", &Light::__init_parallel__);
	Kaba::link_external_class_func("Light.Spherical.__init__", &Light::__init_spherical__);
	Kaba::link_external_class_func("Light.Cone.__init__", &Light::__init_cone__);


	Particle particle(vector::ZERO, 0, nullptr, -1);
	Kaba::declare_class_size("Particle", sizeof(Particle));
	Kaba::declare_class_element("Particle.pos", &Particle::pos);
	Kaba::declare_class_element("Particle.vel", &Particle::vel);
	Kaba::declare_class_element("Particle.radius", &Particle::radius);
	Kaba::declare_class_element("Particle.time_to_live", &Particle::time_to_live);
	Kaba::declare_class_element("Particle.suicidal", &Particle::suicidal);
	Kaba::declare_class_element("Particle.texture", &Particle::texture);
	Kaba::declare_class_element("Particle.color", &Particle::col);
	Kaba::declare_class_element("Particle.source", &Particle::source);
	Kaba::link_external_class_func("Particle.__init__", &Particle::__init__);
	Kaba::link_external_virtual("Particle.__delete__", &Particle::__delete__, &particle);
	Kaba::link_external_virtual("Particle.on_iterate", &Particle::on_iterate, &particle);

	Kaba::declare_class_size("Beam", sizeof(Beam));
	Kaba::declare_class_element("Beam.length", &Beam::length);
	Kaba::link_external_class_func("Beam.__init__", &Beam::__init_beam__);

	gui::Node node(rect::ID);
	Kaba::declare_class_size("ui.Node", sizeof(gui::Node));
	Kaba::declare_class_element("ui.Node.area", &gui::Node::area);
	Kaba::declare_class_element("ui.Node._eff_area", &gui::Node::eff_area);
	Kaba::declare_class_element("ui.Node.margin", &gui::Node::margin);
	Kaba::declare_class_element("ui.Node.align", &gui::Node::align);
	Kaba::declare_class_element("ui.Node.dz", &gui::Node::dz);
	Kaba::declare_class_element("ui.Node.color", &gui::Node::col);
	Kaba::declare_class_element("ui.Node.visible", &gui::Node::visible);
	Kaba::link_external_class_func("ui.Node.__init__", &gui::Picture::__init__);
	Kaba::link_external_virtual("ui.Node.__delete__", &gui::Picture::__delete__, &node);
	Kaba::link_external_class_func("ui.Node.add", &gui::Picture::add);
	Kaba::link_external_virtual("ui.Node.on_iterate", &gui::Picture::on_iterate, &node);
	Kaba::link_external_virtual("ui.Node.on_enter", &gui::Picture::on_enter, &node);
	Kaba::link_external_virtual("ui.Node.on_leave", &gui::Picture::on_leave, &node);
	Kaba::link_external_virtual("ui.Node.on_left_button_down", &gui::Picture::on_left_button_down, &node);
	Kaba::link_external_virtual("ui.Node.on_left_button_up", &gui::Picture::on_left_button_up, &node);
	Kaba::link_external_virtual("ui.Node.on_middle_button_down", &gui::Picture::on_middle_button_down, &node);
	Kaba::link_external_virtual("ui.Node.on_middle_button_up", &gui::Picture::on_middle_button_up, &node);
	Kaba::link_external_virtual("ui.Node.on_right_button_down", &gui::Picture::on_right_button_down, &node);
	Kaba::link_external_virtual("ui.Node.on_right_button_up", &gui::Picture::on_right_button_up, &node);

	gui::Picture picture(rect::ID, nullptr);
	Kaba::declare_class_size("ui.Picture", sizeof(gui::Picture));
	Kaba::declare_class_element("ui.Picture.source", &gui::Picture::source);
	Kaba::declare_class_element("ui.Picture.texture", &gui::Picture::texture);
	Kaba::declare_class_element("ui.Picture.blur", &gui::Picture::bg_blur);
	Kaba::link_external_class_func("ui.Picture.__init__:2", &gui::Picture::__init2__);
	Kaba::link_external_class_func("ui.Picture.__init__:3", &gui::Picture::__init3__);
	Kaba::link_external_virtual("ui.Picture.__delete__", &gui::Picture::__delete__, &picture);

	Kaba::link_external_class_func("ui.HBox.__init__", &gui::HBox::__init__);
	Kaba::link_external_class_func("ui.VBox.__init__", &gui::VBox::__init__);

	gui::Text text(":::fake:::", 0, 0, 0);
	Kaba::declare_class_size("ui.Text", sizeof(gui::Text));
	Kaba::declare_class_element("ui.Text.font_size", &gui::Text::font_size);
	Kaba::declare_class_element("ui.Text.text", &gui::Text::text);
	Kaba::link_external_class_func("ui.Text.__init__:2", &gui::Text::__init2__);
	Kaba::link_external_class_func("ui.Text.__init__:4", &gui::Text::__init4__);
	Kaba::link_external_virtual("ui.Text.__delete__", &gui::Text::__delete__, &text);
	Kaba::link_external_class_func("ui.Text.set_text", &gui::Text::set_text);

	Kaba::declare_class_size("Link", sizeof(Link));
	Kaba::declare_class_element("Link.a", &Link::a);
	Kaba::declare_class_element("Link.b", &Link::b);
	Kaba::link_external_class_func("Link.set_motor", &Link::set_motor);
	Kaba::link_external_class_func("Link.set_frame", &Link::set_frame);
	//Kaba::link_external_class_func("Link.set_axis", &Link::set_axis);

	Kaba::link_external("ui.key", (void*)&InputManager::get_key);
	Kaba::link_external("ui.key_down", (void*)&InputManager::get_key_down);
	Kaba::link_external("ui.key_up", (void*)&InputManager::get_key_up);
	Kaba::link_external("ui.toplevel", &gui::toplevel);
	Kaba::link_external("ui.mouse", &InputManager::mouse);
	Kaba::link_external("ui.dmouse", &InputManager::dmouse);
	Kaba::link_external("ui.scroll", &InputManager::scroll);

	Kaba::declare_class_size("PerformanceMonitor", sizeof(PerformanceMonitor));
	Kaba::declare_class_element("PerformanceMonitor.avg", &PerformanceMonitor::avg);
	Kaba::declare_class_element("PerformanceMonitor.frames", &PerformanceMonitor::frames);
	//Kaba::declare_class_element("PerformanceMonitor.location", &PerformanceMonitor::avg.location);
	Kaba::link_external("perf_mon", &global_perf_mon);

	Kaba::declare_class_size("EngineData", sizeof(EngineData));
	Kaba::declare_class_element("EngineData.app_name", &EngineData::app_name);
	Kaba::declare_class_element("EngineData.version", &EngineData::version);
	Kaba::declare_class_element("EngineData.physics_enabled", &EngineData::physics_enabled);
	Kaba::declare_class_element("EngineData.collisions_enabled", &EngineData::collisions_enabled);
	Kaba::declare_class_element("EngineData.elapsed", &EngineData::elapsed);
	Kaba::declare_class_element("EngineData.elapsed_rt", &EngineData::elapsed_rt);
	Kaba::declare_class_element("EngineData.time_scale", &EngineData::time_scale);
	Kaba::declare_class_element("EngineData.fps_min", &EngineData::fps_min);
	Kaba::declare_class_element("EngineData.fps_max", &EngineData::fps_max);
	Kaba::declare_class_element("EngineData.debug", &EngineData::debug);
	Kaba::declare_class_element("EngineData.console_enabled", &EngineData::console_enabled);
	Kaba::declare_class_element("EngineData.wire_mode", &EngineData::wire_mode);
	Kaba::declare_class_element("EngineData.show_timings", &EngineData::show_timings);
	Kaba::declare_class_element("EngineData.first_frame", &EngineData::first_frame);
	Kaba::declare_class_element("EngineData.resetting_game", &EngineData::resetting_game);
	Kaba::declare_class_element("EngineData.game_running", &EngineData::game_running);
	Kaba::declare_class_element("EngineData.default_font", &EngineData::default_font);
	Kaba::declare_class_element("EngineData.detail_level", &EngineData::detail_level);
	Kaba::declare_class_element("EngineData.initial_world_file", &EngineData::initial_world_file);
	Kaba::declare_class_element("EngineData.second_world_file", &EngineData::second_world_file);
	Kaba::declare_class_element("EngineData.physical_aspect_ratio", &EngineData::physical_aspect_ratio);

	Kaba::link_external("tex_white", &_tex_white);
	Kaba::link_external("world", &world);
	Kaba::link_external("cam", &cam);
	Kaba::link_external("engine", &engine);
	Kaba::link_external("load_model", (void*)&ModelManager::load);
}

void PluginManager::reset() {
	controllers.clear();
}

void assign_variables(char *p, const Kaba::Class *c, Array<TemplateDataScriptVariable> &variables) {
	for (auto &v: variables) {
		for (auto &e: c->elements)
			if (v.name == e.name.lower().replace("_", "")) {
				//msg_write("  " + e.type->long_name() + " " + e.name + " = " + v.value);
				if (e.type == Kaba::TypeInt)
					*(int*)(p + e.offset) = v.value._int();
				else if (e.type == Kaba::TypeFloat32)
					*(float*)(p + e.offset) = v.value._float();
				else if (e.type == Kaba::TypeBool)
					*(bool*)(p + e.offset) = v.value._bool();
				else if (e.type == Kaba::TypeString)
					*(string*)(p + e.offset) = v.value;
			}
	}
}

void *PluginManager::create_instance(const Path &filename, const string &base_class, Array<TemplateDataScriptVariable> &variables) {
	//msg_write(format("INSTANCE  %s:   %s", filename, base_class));
	try {
		auto *s = Kaba::Load(filename);
		for (auto *c: s->classes()) {
			if (c->is_derived_from_s(base_class)) {
				void *p = c->create_instance();
				assign_variables((char*)p, c, variables);
				return p;
			}
		}
	} catch(Kaba::Exception &e) {
		msg_error(e.message());
		throw Exception(e.message());
	}
	throw Exception(format("script does not contain a class derived from '%s'", base_class));
	return nullptr;
}

void PluginManager::add_controller(const Path &name, Array<TemplateDataScriptVariable> &variables) {
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

