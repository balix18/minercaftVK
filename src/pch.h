#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <optional>
#include <limits>
#include <chrono>
#include <fstream>
#include <filesystem>

#include <vulkan/vulkan.hpp>

#include <glad/glad.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <boost/functional/hash.hpp>

#include <tiny_obj_loader.h>
#include <stb_image.h>

#define LOGGER_USING_FMT
#include <EZPZLogger/logger.h>

#define INPUT_MANAGER_USE_AS_SINGLETON
#define INPUT_MANAGER_CUSTOM_SINGLETON_NAME theInputManager
#include <glfwim/input_manager.h>

#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

using namespace ezpz;
using namespace glfwim;

using uint = unsigned int;

namespace fs = std::filesystem;
