#include "StageBindings.h"

#include "core/util/AppPaths.h"
#include "ecs/MEcs.h"
#include "packs/rigPython/src/rigPython.h"
#include "CStageScript.h"
#include "StageScene.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <unordered_map>

#if defined(RIGPYTHON_HAS_EMBED)
#include <Python.h>
#endif

namespace stage {
namespace {

#if defined(RIGPYTHON_HAS_EMBED)

rigkit::MEcs* gEcs = nullptr;

bool requireEcs(const char* fn) {
	if (gEcs) {
		return true;
	}
	PyErr_Format(PyExc_RuntimeError, "stage.%s: no ECS", fn);
	return false;
}

entt::entity entityArg(PyObject* args, const char* fn) {
	unsigned long id = 0;
	if (!PyArg_ParseTuple(args, "k", &id)) {
		return entt::null;
	}
	auto entity = static_cast<entt::entity>(static_cast<std::uint32_t>(id));
	if (!gEcs->registry().valid(entity)) {
		PyErr_Format(PyExc_ValueError, "stage.%s: bad entity", fn);
		return entt::null;
	}
	return entity;
}

PyObject* py_find(PyObject*, PyObject* args) {
	if (!requireEcs("find")) {
		return nullptr;
	}
	const char* name = nullptr;
	if (!PyArg_ParseTuple(args, "s", &name)) {
		return nullptr;
	}
	auto entity = find(*gEcs, name);
	if (entity == entt::null) {
		Py_RETURN_NONE;
	}
	return PyLong_FromUnsignedLong(static_cast<unsigned long>(entity));
}

PyObject* py_create(PyObject*, PyObject* args) {
	if (!requireEcs("create")) {
		return nullptr;
	}
	const char* name = nullptr;
	if (!PyArg_ParseTuple(args, "s", &name)) {
		return nullptr;
	}
	auto entity = create(*gEcs, name);
	return PyLong_FromUnsignedLong(static_cast<unsigned long>(entity));
}

PyObject* py_destroy(PyObject*, PyObject* args) {
	if (!requireEcs("destroy")) {
		return nullptr;
	}
	auto entity = entityArg(args, "destroy");
	if (PyErr_Occurred()) {
		return nullptr;
	}
	destroy(*gEcs, entity);
	Py_RETURN_NONE;
}

PyObject* py_count(PyObject*, PyObject*) {
	if (!requireEcs("count")) {
		return nullptr;
	}
	return PyLong_FromSize_t(gEcs->getEntityCount());
}

PyObject* py_name(PyObject*, PyObject* args) {
	if (!requireEcs("name")) {
		return nullptr;
	}
	auto entity = entityArg(args, "name");
	if (PyErr_Occurred()) {
		return nullptr;
	}
	return PyUnicode_FromString(gEcs->entityName(entity).c_str());
}

PyObject* py_position(PyObject*, PyObject* args) {
	if (!requireEcs("position")) {
		return nullptr;
	}
	auto entity = entityArg(args, "position");
	if (PyErr_Occurred()) {
		return nullptr;
	}
	glm::vec3 position{0.f};
	if (!positionOf(*gEcs, entity, position)) {
		PyErr_SetString(PyExc_RuntimeError, "stage.position: no transform");
		return nullptr;
	}
	return Py_BuildValue("(fff)", position.x, position.y, position.z);
}

PyObject* py_set_position(PyObject*, PyObject* args) {
	if (!requireEcs("set_position")) {
		return nullptr;
	}
	unsigned long id = 0;
	double x = 0, y = 0, z = 0;
	if (!PyArg_ParseTuple(args, "kddd", &id, &x, &y, &z)) {
		return nullptr;
	}
	auto entity = static_cast<entt::entity>(static_cast<std::uint32_t>(id));
	if (!gEcs->registry().valid(entity)) {
		PyErr_SetString(PyExc_ValueError, "stage.set_position: bad entity");
		return nullptr;
	}
	if (simOwnsPose(*gEcs, entity)) {
		PyErr_SetString(PyExc_RuntimeError, "stage.set_position: sim owns pose");
		return nullptr;
	}
	if (!setPosition(*gEcs, entity,
					 {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)})) {
		PyErr_SetString(PyExc_RuntimeError, "stage.set_position: no transform");
		return nullptr;
	}
	Py_RETURN_NONE;
}

PyObject* py_velocity(PyObject*, PyObject* args) {
	if (!requireEcs("velocity")) {
		return nullptr;
	}
	auto entity = entityArg(args, "velocity");
	if (PyErr_Occurred()) {
		return nullptr;
	}
	glm::vec3 velocity{0.f};
	if (!velocityOf(*gEcs, entity, velocity)) {
		PyErr_SetString(PyExc_RuntimeError, "stage.velocity: no rigidbody");
		return nullptr;
	}
	return Py_BuildValue("(fff)", velocity.x, velocity.y, velocity.z);
}

PyObject* py_spawn(PyObject*, PyObject* args) {
	if (!requireEcs("spawn")) {
		return nullptr;
	}
	const char* prefab = nullptr;
	double x = 0, y = 0, z = 0, vx = 0, vy = 0, vz = 0;
	if (!PyArg_ParseTuple(args, "sddd|ddd", &prefab, &x, &y, &z, &vx, &vy, &vz)) {
		return nullptr;
	}
	auto entity =
		spawn(*gEcs, prefab, {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
			  {static_cast<float>(vx), static_cast<float>(vy), static_cast<float>(vz)});
	if (entity == entt::null) {
		PyErr_Format(PyExc_RuntimeError, "stage.spawn: no prefab '%s'", prefab);
		return nullptr;
	}
	return PyLong_FromUnsignedLong(static_cast<unsigned long>(entity));
}

PyObject* py_sim_owns(PyObject*, PyObject* args) {
	if (!requireEcs("sim_owns")) {
		return nullptr;
	}
	auto entity = entityArg(args, "sim_owns");
	if (PyErr_Occurred()) {
		return nullptr;
	}
	if (simOwnsPose(*gEcs, entity)) {
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyMethodDef kMethods[] = {
	{"find", py_find, METH_VARARGS, "find(name) -> id or None"},
	{"create", py_create, METH_VARARGS, "create(name) -> id"},
	{"destroy", py_destroy, METH_VARARGS, "destroy(id)"},
	{"count", py_count, METH_NOARGS, "count() -> int"},
	{"name", py_name, METH_VARARGS, "name(id) -> str"},
	{"position", py_position, METH_VARARGS, "position(id) -> (x, y, z)"},
	{"set_position", py_set_position, METH_VARARGS, "set_position(id, x, y, z)"},
	{"velocity", py_velocity, METH_VARARGS, "velocity(id) -> launch (x, y, z)"},
	{"spawn", py_spawn, METH_VARARGS, "spawn(prefab, x, y, z[, vx, vy, vz]) -> id"},
	{"sim_owns", py_sim_owns, METH_VARARGS, "sim_owns(id) -> bool"},
	{nullptr, nullptr, 0, nullptr},
};

PyModuleDef kModule = {
	PyModuleDef_HEAD_INIT,
	"stage",
	"Stage entities: find, create, destroy, spawn prefabs, pose on non-sim actors",
	-1,
	kMethods,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

bool injectModule() {
	if (!Py_IsInitialized()) {
		return false;
	}
	PyObject* name = PyUnicode_FromString("stage");
	PyObject* existing = name ? PyImport_GetModule(name) : nullptr;
	Py_XDECREF(name);
	if (existing) {
		Py_DECREF(existing);
		return true;
	}
	PyObject* mod = PyModule_Create(&kModule);
	if (!mod) {
		return false;
	}
	PyObject* sysModules = PyImport_GetModuleDict();
	if (!sysModules || PyDict_SetItemString(sysModules, "stage", mod) != 0) {
		Py_DECREF(mod);
		return false;
	}
	Py_DECREF(mod);
	return true;
}

struct ScriptLoad {
	std::filesystem::file_time_type mtime{};
	bool loaded = false;
};

std::unordered_map<std::uint32_t, ScriptLoad> gLoads;

std::string scriptPath(const CStageScript& script) {
	std::filesystem::path path(script.path);
	if (path.is_absolute()) {
		return script.path;
	}
	return AppPaths::joinPath(AppPaths::getDataDir(), script.path);
}

#endif

} // namespace

bool install(rigkit::rigPython& py, rigkit::MEcs* ecs, std::string* err) {
#if !defined(RIGPYTHON_HAS_EMBED)
	(void)py;
	(void)ecs;
	if (err) {
		*err = "CPython Development.Embed was not found at configure time";
	}
	return false;
#else
	if (!py.available()) {
		if (err) {
			*err = py.lastError().empty() ? "Python not available" : py.lastError();
		}
		return false;
	}
	if (!py.eval("pass")) {
		if (err) {
			*err = py.lastError();
		}
		return false;
	}
	gEcs = ecs;
	if (!injectModule()) {
		if (err) {
			*err = "failed to inject stage module";
		}
		return false;
	}
	if (!py.eval("import stage")) {
		if (err) {
			*err = py.lastError();
		}
		return false;
	}
	return true;
#endif
}

bool runScripts(rigkit::MEcs& ecs, rigkit::rigPython& py, float dt, std::string* err) {
#if !defined(RIGPYTHON_HAS_EMBED)
	(void)ecs;
	(void)py;
	(void)dt;
	if (err) {
		*err = "CPython Development.Embed was not found at configure time";
	}
	return false;
#else
	if (!py.available()) {
		if (err) {
			*err = "Python not available";
		}
		return false;
	}
	gEcs = &ecs;
	if (!std::isfinite(dt)) {
		dt = 0.f;
	}

	bool any = false;
	for (auto entity : ecs.view<CStageScript>()) {
		const auto& script = ecs.getComponent<CStageScript>(entity);
		if (!script.enabled || script.path.empty()) {
			continue;
		}
		any = true;
		const std::string path = scriptPath(script);
		std::error_code ec;
		const auto mtime = std::filesystem::last_write_time(path, ec);
		if (ec) {
			if (err) {
				*err = "script missing: " + path;
			}
			return false;
		}
		auto& load = gLoads[static_cast<std::uint32_t>(entity)];
		if (!load.loaded || mtime != load.mtime) {
			if (!py.runFile(path)) {
				if (err) {
					*err = py.lastError();
				}
				return false;
			}
			load.loaded = true;
			load.mtime = mtime;
		}
	}
	if (!any) {
		return true;
	}

	char call[128];
	std::snprintf(call, sizeof(call),
				  "if 'update' in globals() and callable(update):\n"
				  "    update(%.8f)\n",
				  static_cast<double>(dt));
	if (!py.eval(call)) {
		if (err) {
			*err = py.lastError();
		}
		return false;
	}
	return true;
#endif
}

} // namespace stage
