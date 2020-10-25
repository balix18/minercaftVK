#pragma once

struct Runcfg
{
	fs::path projectSourceDir;
	std::string currentRenderer;
	std::string currentRendererName;
	fs::path shadersDir;
	fs::path texturesDir;

	static Runcfg& Instance();
	void Init();

	Runcfg(Runcfg const&) = delete;
	Runcfg& operator=(Runcfg const&) = delete;
	Runcfg(Runcfg&&) = delete;
	Runcfg& operator=(Runcfg&&) = delete;

private:
	Runcfg();

	bool initialized;
};

inline Runcfg& theRuncfg = Runcfg::Instance();
