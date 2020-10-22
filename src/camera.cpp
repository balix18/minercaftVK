#include "Camera.h"

Camera::Camera():
	worldRight{ 1, 0, 0 },
	worldUp{ 0, 1, 0 },
	worldForward{ 0, 0, -1 },
	drag{ 0.003f, {0, 0} },
	position{ 0, 0, 0 },
	movementSpeed{ 1.0f },
	time{ 0.0f, 0.0f, 0.0f },
	moveDirection{ 0, 0, 0 },
	isVulkan{ false }
{
	UpdateDirection(worldForward);
}

void Camera::Init(bool isVulkan)
{
	this->isVulkan = isVulkan;
	RegisterInputHandlers();
}

void Camera::UpdatePosition(glm::vec3 newPosition)
{
	position = newPosition;
}

void Camera::UpdateDirection(glm::vec3 newDirection)
{
	direction = newDirection;
	right = glm::cross(direction, worldUp);
	up = glm::cross(right, direction);
}

glm::vec3 Camera::GetPosition() const
{
	return position;
}

glm::vec3 Camera::GetDirection() const
{
	return direction;
}

void Camera::RegisterInputHandlers()
{
	pressedKeys["d"] = false;
	pressedKeys["a"] = false;
	pressedKeys["w"] = false;
	pressedKeys["s"] = false;
	pressedKeys["e"] = false;
	pressedKeys["q"] = false;

	theInputManager.registerUtf8KeyHandler("d", Modifier::None, [&](auto action) {
		if (action == Action::Press) {
			pressedKeys["d"] = true;
		}
		else if (action == Action::Release) {
			pressedKeys["d"] = false;
		}
	});

	theInputManager.registerUtf8KeyHandler("a", Modifier::None, [&](auto action) {
		if (action == Action::Press) {
			pressedKeys["a"] = true;
		}
		else if (action == Action::Release) {
			pressedKeys["a"] = false;
		}
	});

	theInputManager.registerUtf8KeyHandler("w", Modifier::None, [&](auto action) {
		if (action == Action::Press) {
			pressedKeys["w"] = true;
		}
		else if (action == Action::Release) {
			pressedKeys["w"] = false;
		}
	});

	theInputManager.registerUtf8KeyHandler("s", Modifier::None, [&](auto action) {
		if (action == Action::Press) {
			pressedKeys["s"] = true;
		}
		else if (action == Action::Release) {
			pressedKeys["s"] = false;
		}
	});

	theInputManager.registerUtf8KeyHandler("e", Modifier::None, [&](auto action) {
		if (action == Action::Press) {
			pressedKeys["e"] = true;
		}
		else if (action == Action::Release) {
			pressedKeys["e"] = false;
		}
	});

	theInputManager.registerUtf8KeyHandler("q", Modifier::None, [&](auto action) {
		if (action == Action::Press) {
			pressedKeys["q"] = true;
		}
		else if (action == Action::Release) {
			pressedKeys["q"] = false;
		}
	});

	theInputManager.registerUtf8KeyHandler(" ", Modifier::None, Action::Press, [&]() {
		theLogger.LogInfo("CameraPos: {}, CameraDir: {}", glm::to_string(position), glm::to_string(direction));
	});

	// nem kell a registerWindowResizeHandler-re kezzel itt is raakasztkodni, mert a createSwapChain beallitja a parametereket
}

glm::mat4 Camera::V() const
{
	return glm::lookAt(position, position + direction, up);
}

glm::mat4 Camera::P() const
{
	auto proj = glm::perspective(parameters.fov.horizontal, parameters.aspectRatio, parameters.clippingDistance.zNear, parameters.clippingDistance.zFar);
	if (isVulkan) {
		proj[1][1] *= -1;
	}

	return proj;
}

void Camera::StartDrag(glm::vec2 const& currentPoint)
{
	throw std::runtime_error("TODO");

	drag.startPos = currentPoint;
}

void Camera::Drag(glm::vec2 const& currentPoint)
{
	throw std::runtime_error("TODO");

	auto delta = currentPoint - drag.startPos;

	direction = glm::normalize(direction);
	right = glm::cross(direction, worldUp);
	up = glm::cross(right, direction);

	auto glmMidentity = glm::mat4(1.0f);
	auto glmMrotateX = glm::rotate(glmMidentity, delta.x * drag.speed, worldUp);
	auto glmMrotateY = glm::rotate(glmMidentity, delta.y * drag.speed, right);

	direction = glm::vec4(direction, 1) * glmMrotateX;
	direction = glm::vec4(direction, 1) * glmMrotateY;

	drag.startPos = currentPoint;
}

void Camera::Update(float newTime)
{
	time.deltaTime = newTime - time.lastTime;
	time.lastTime = time.currentTime;
	time.currentTime = newTime;
}

void Camera::Control()
{
	if (pressedKeys["w"]) position += direction * movementSpeed * time.deltaTime;
	if (pressedKeys["s"]) position -= direction * movementSpeed * time.deltaTime;
	if (pressedKeys["d"]) position += right * movementSpeed * time.deltaTime;
	if (pressedKeys["a"]) position -= right * movementSpeed * time.deltaTime;
	if (pressedKeys["e"]) position += up * movementSpeed * time.deltaTime;
	if (pressedKeys["q"]) position -= up * movementSpeed * time.deltaTime;
}

Camera::Parameters::Parameters():
	fov{ 60.0f,  60.0f },
	aspectRatio{ 1.0 },
	clippingDistance{ 1.0f, 10000.0f }
{
}

void Camera::Parameters::UpdateWindowSize(float width, float height)
{
	aspectRatio = width / height;
	fov.vertical = glm::radians(30.0f);
	fov.horizontal = 2.0f * atanf(tanf(fov.vertical / 2.0f) * aspectRatio);

	theLogger.LogInfo("Camera parameters updated. Aspect ratio: {}, Field of view: {}", aspectRatio, fov.horizontal);
}
