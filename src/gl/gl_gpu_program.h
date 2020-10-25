#pragma once

struct ShaderPath
{
	enum struct Type { VERT, FRAG, GEOM, COMP, TESC, TESE } type;
	std::string path;

	ShaderPath(Type type, std::string const& path);

	static unsigned GetGlType(Type type);
};

struct GpuProgram
{
	GLuint programHandle;
	std::vector<ShaderPath> shaderPathList;

	GpuProgram();
	virtual ~GpuProgram();

	void Create();
	void PrintActiveNames();

	virtual void Bind() const = 0;
	virtual void BindMaterial() const = 0;
	virtual std::string GetPrettyName() const = 0;

private:
	bool debugPrintActiveNames;

	static void GetErrorInfo(unsigned handle);
	static void CheckShader(unsigned shader, std::string const& message);
	static void CheckLinking(unsigned program);
	static GLuint CompileShader(GLenum shaderType, std::string const& path);
};
