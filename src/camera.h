#pragma once

struct Camera
{
	float movementSpeed;
	std::unordered_map<std::string, bool> pressedKeys;

	Camera();
	void Init(bool isVulkan);
	void UpdatePosition(glm::vec3 newPosition);
	void UpdateDirection(glm::vec3 newDirection);
	glm::vec3 GetPosition() const;
	glm::vec3 GetDirection() const;
	void RegisterInputHandlers();

	glm::mat4 V() const;
	glm::mat4 P() const;

	void Update(float newTime);
	void Control();

	struct Parameters
	{
		glm::vec2 windowSize;
		struct { float horizontal, vertical; } fov;
		float aspectRatio;
		struct { float zNear, zFar; } clippingDistance;

		Parameters();
		void UpdateWindowSize(float width, float height);

	} parameters;

private:

	glm::vec3 const worldRight, worldUp, worldForward;
	glm::vec3 position;
	glm::vec3 direction, right, up;
	struct Drag { float speed; glm::vec2 startPos; bool moving; } drag;
	struct TimeInfo { float currentTime, lastTime, deltaTime; } time;
	glm::vec3 moveDirection;
	bool isVulkan;
};
