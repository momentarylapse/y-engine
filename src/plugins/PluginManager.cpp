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
#include "../gui/Picture.h"
#include "../gui/Text.h"
#include "../helper/InputManager.h"
#include "../helper/PerformanceMonitor.h"


PluginManager plugin_manager;
PerformanceMonitor *global_perf_mon;


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

	Model model;
	Kaba::declare_class_size("Model", sizeof(Model));
	Kaba::declare_class_element("Model.pos", &Model::pos);
	Kaba::declare_class_element("Model.vel", &Model::vel);
	Kaba::declare_class_element("Model.ang", &Model::ang);
	Kaba::declare_class_element("Model.rot", &Model::rot);
	Kaba::declare_class_element("Model.mesh", &Model::mesh);
	Kaba::declare_class_element("Model.materials", &Model::material);
	Kaba::declare_class_element("Model.bones", &Model::bone);
	//Kaba::declare_class_element("Model.mass", &Model::physics_data.mass);
	Kaba::link_external_class_func("Model.make_editable", &Model::make_editable);
	Kaba::link_external_class_func("Model.begin_edit", &Model::begin_edit);
	Kaba::link_external_class_func("Model.end_edit", &Model::end_edit);
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
	Kaba::link_external_class_func("World.create_object", &World::create_object);
	Kaba::link_external_class_func("World.create_terrain", &World::create_terrain);
	Kaba::link_external_class_func("World.add_light", &World::add_light);
	Kaba::link_external_class_func("World.add_particle", &World::add_particle);

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
	Kaba::link_external_virtual("Controller.before_draw", &Controller::before_draw, &con);


	Kaba::declare_class_size("Light", sizeof(Light));
	Kaba::declare_class_element("Light.pos", &Light::pos);
	Kaba::declare_class_element("Light.dir", &Light::dir);
	Kaba::declare_class_element("Light.color", &Light::col);
	Kaba::declare_class_element("Light.radius", &Light::radius);
	Kaba::declare_class_element("Light.theta", &Light::theta);
	Kaba::declare_class_element("Light.harshness", &Light::harshness);
	Kaba::declare_class_element("Light.enabled", &Light::enabled);

	Kaba::link_external_class_func("LightParallel.__init__", &Light::__init_parallel__);
	Kaba::link_external_class_func("LightSpherical.__init__", &Light::__init_spherical__);
	Kaba::link_external_class_func("LightCone.__init__", &Light::__init_cone__);


	Kaba::declare_class_size("Particle", sizeof(Particle));
	Kaba::declare_class_element("Particle.pos", &Particle::pos);
	Kaba::declare_class_element("Particle.radius", &Particle::radius);
	Kaba::declare_class_element("Particle.texture", &Particle::texture);
	Kaba::declare_class_element("Particle.color", &Particle::col);
	Kaba::declare_class_element("Particle.source", &Particle::source);
	Kaba::link_external_class_func("Particle.__init__", &Particle::__init__);


	Kaba::declare_class_size("Picture", sizeof(Picture));
	Kaba::declare_class_element("Picture.dest", &Picture::dest);
	Kaba::declare_class_element("Picture.source", &Picture::source);
	Kaba::declare_class_element("Picture.z", &Picture::z);
	Kaba::declare_class_element("Picture.texture", &Picture::texture);
	Kaba::declare_class_element("Picture.blur", &Picture::bg_blur);
	Kaba::declare_class_element("Picture.color", &Picture::col);
	Kaba::link_external_class_func("Picture.__init__", &Picture::__init__);
	Kaba::link_external_class_func("Picture.__delete__", &Picture::__delete__);

	Kaba::declare_class_size("Text", sizeof(Text));
	Kaba::declare_class_element("Text.font_size", &Text::font_size);
	Kaba::declare_class_element("Text.text", &Text::text);
	Kaba::link_external_class_func("Text.__init__", &Text::__init__);
	Kaba::link_external_class_func("Text.__delete__", &Text::__delete__);
	Kaba::link_external_class_func("Text.set_text", &Text::set_text);

	Kaba::declare_class_size("Link", sizeof(Link));
	Kaba::declare_class_element("Link.a", &Link::a);
	Kaba::declare_class_element("Link.b", &Link::b);
	Kaba::link_external_class_func("Link.set_motor", &Link::set_motor);
	Kaba::link_external_class_func("Link.set_frame", &Link::set_frame);
	//Kaba::link_external_class_func("Link.set_axis", &Link::set_axis);

	Kaba::link_external("get_key", (void*)&InputManager::get_key);
	Kaba::link_external("get_key_down", (void*)&InputManager::get_key_down);
	Kaba::link_external("get_key_up", (void*)&InputManager::get_key_up);

	Kaba::link_external("gui_add", (void*)&gui::add);

	Kaba::declare_class_size("PerformanceMonitor", sizeof(PerformanceMonitor));
	Kaba::declare_class_element("PerformanceMonitor.avg", &PerformanceMonitor::avg);
	Kaba::declare_class_element("PerformanceMonitor.frames", &PerformanceMonitor::frames);
	//Kaba::declare_class_element("PerformanceMonitor.location", &PerformanceMonitor::avg.location);
	Kaba::link_external("perf_mon", &global_perf_mon);

	Kaba::declare_class_size("EngineData", sizeof(EngineData));
	Kaba::declare_class_element("EngineData.physics_enabled", &EngineData::physics_enabled);

	Kaba::link_external("world", &world);
	Kaba::link_external("cam", &cam);
	Kaba::link_external("engine", &engine);
	Kaba::link_external("mouse", &InputManager::mouse);
	Kaba::link_external("dmouse", &InputManager::dmouse);
	Kaba::link_external("scroll", &InputManager::scroll);
	Kaba::link_external("load_model", (void*)&ModelManager::load);
}

void PluginManager::reset() {
	controllers.clear();
}

void *PluginManager::create_instance(const Path &filename, const string &base_class) {
	try {
		auto *s = Kaba::Load(filename);
		for (auto *c: s->classes()) {
			if (c->is_derived_from_s(base_class)) {
				return c->create_instance();
			}
		}
	} catch(Kaba::Exception &e) {
		msg_error(e.message());
		throw Exception(e.message());
	}
	throw Exception(("script does not contain a class derived from " + base_class));
	return nullptr;
}

void PluginManager::add_controller(const Path &name) {
	auto *c = (Controller*)create_instance(name, "Controller");
	controllers.add(c);
	c->on_init();
}

