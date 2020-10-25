#include "runcfg.h"

#ifndef PROJECT_SOURCE_DIR
#error "PROJECT_SOURCE_DIR must be defined"
#endif

Runcfg::Runcfg() :
	initialized{ false }
{
	projectSourceDir = PROJECT_SOURCE_DIR;
	projectSourceDir.make_preferred();
}

Runcfg& Runcfg::Instance()
{
	static Runcfg instance;
	return instance;
}

void Runcfg::Init()
{
	if (initialized) throw std::runtime_error("Runcfg already initialized");

	initialized = true;

	auto runcfgPath = projectSourceDir / "runcfg.json";
	std::ifstream ifs(runcfgPath);
	rapidjson::IStreamWrapper isw(ifs);

	rapidjson::Document d;
	d.ParseStream(isw);

	currentRenderer = d["currentRenderer"].GetString();
	currentRendererName = d["renderers"][currentRenderer.c_str()]["name"].GetString();
	shadersDir = projectSourceDir / d["shadersDir"].GetString();
	texturesDir = projectSourceDir / d["texturesDir"].GetString();
}
