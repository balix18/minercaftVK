#include "input_manager.h"
#include <GLFW/glfw3.h>

namespace glfwim {
    void InputManager::initialize(GLFWwindow* window) {
        keyStates.resize(GLFW_KEY_LAST, 0);
        this->window = window;

        glfwSetWindowUserPointer(window, this);

        glfwSetKeyCallback(window, [](auto window, int key, int scancode, int action, int mods) {
            auto& inputManager = *(InputManager*)glfwGetWindowUserPointer(window);

            if (inputManager.isKeyboardCaptured() && !(inputManager.keyStates[key - 1] && action == GLFW_RELEASE)) return;

            if (key >= 1) {
                if (!inputManager.keyStates[key - 1] && action == GLFW_RELEASE) return;
                if (action == GLFW_PRESS) inputManager.keyStates[key - 1] = true;
                else if (action == GLFW_RELEASE) inputManager.keyStates[key - 1] = false;
            }

            auto tempKeyHandlers = backupContainer(inputManager.keyHandlers);
            auto tempUtf8KeyHandlers = backupContainer(inputManager.utf8KeyHandlers);

            for (auto& h : tempKeyHandlers) {
                if (h.enabled) h.handler(scancode, Modifier{ mods }, Action{ action });
            }

            if (!tempUtf8KeyHandlers.empty()) {
                const char* utf8key = glfwGetKeyName(key, scancode);
                if (utf8key) {
                    for (auto& h : tempUtf8KeyHandlers) {
                        if (h.enabled) h.handler(utf8key, Modifier{ mods }, Action{ action });
                    }
                }
            }

            restoreContainer(inputManager.keyHandlers, tempKeyHandlers);
            restoreContainer(inputManager.utf8KeyHandlers, tempUtf8KeyHandlers);
        });

        glfwSetMouseButtonCallback(window, [](auto window, int button, int action, int mods) {
            auto& inputManager = *(InputManager*)glfwGetWindowUserPointer(window);

            if (inputManager.isMouseCaptured()) return;

            auto tempHandlers = backupContainer(inputManager.mouseButtonHandlers);
 
            for (auto& h : tempHandlers) {
                if (h.enabled) h.handler(MouseButton{ button }, Modifier{ mods }, Action{ action });
            }

            restoreContainer(inputManager.mouseButtonHandlers, tempHandlers);
        });

        glfwSetScrollCallback(window, [](auto window, double x, double y) {
            auto& inputManager = *(InputManager*)glfwGetWindowUserPointer(window);

            if (inputManager.isMouseCaptured()) return;

            auto tempHandlers = backupContainer(inputManager.mouseScrollHandlers);

            for (auto& h : tempHandlers) {
                if (h.enabled) h.handler(x, y);
            }

            restoreContainer(inputManager.mouseScrollHandlers, tempHandlers);
        });

        glfwSetCursorEnterCallback(window, [](auto window, int entered) {
            auto& inputManager = *(InputManager*)glfwGetWindowUserPointer(window);
            auto movement = CursorMovement{ !!entered };

            if (inputManager.isMouseCaptured()) return;

            auto tempHandlers = backupContainer(inputManager.cursorMovementHandlers);

            for (auto& h : tempHandlers) {
                if (h.enabled) h.handler(movement);
            }

            restoreContainer(inputManager.cursorMovementHandlers, tempHandlers);
        });

        glfwSetCursorPosCallback(window, [](auto window, double x, double y) {
            auto& inputManager = *(InputManager*)glfwGetWindowUserPointer(window);

            if (inputManager.isMouseCaptured()) return;

            auto tempHandlers = backupContainer(inputManager.cursorPositionHandlers);

            for (auto& h : tempHandlers) {
                if (h.enabled) h.handler(x, y);
            }

            restoreContainer(inputManager.cursorPositionHandlers, tempHandlers);
        });

        glfwSetFramebufferSizeCallback(window, [](auto window, int x, int y) {
            auto& inputManager = *(InputManager*)glfwGetWindowUserPointer(window);

            auto tempHandlers = backupContainer(inputManager.windowResizeHandlers);

            for (auto& h : tempHandlers) {
                if (h.enabled) h.handler(x, y);
            }

            restoreContainer(inputManager.windowResizeHandlers, tempHandlers);
        });
    }

    void InputManager::pollEvents() {
        glfwPollEvents();
        elapsedTime();
        finishedInputHandling = true;
        while (paused);
    }

    void InputManager::waitUntilNextEventHandling() {
        finishedInputHandling = false;
        while (!finishedInputHandling);
    }

    void InputManager::waitUntilNextEventHandling(double timeout) {
        finishedInputHandling = false;
        double now = glfwGetTime() * 1000.0;
        while (!finishedInputHandling) {
            if ((glfwGetTime() * 1000.0 - now) >= timeout) {
                break;
            }
        }
    }

    void InputManager::pauseInputHandling() {
        paused = true;
        waitUntilNextEventHandling();
    }

    void InputManager::continueInputHandling() {
        paused = false;
    }

    void InputManager::elapsedTime() {
        if (isMouseCaptured()) return;

        auto tempHandlers = backupContainer(cursorHoldHandlers);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        for (auto& it : tempHandlers) {
            if (!it.enabled) continue;

            if (it.handler.startTime < 0) {
                it.handler.startTime = glfwGetTime() * 1000.0;
                it.handler.x = x;
                it.handler.y = y;
                continue;
            }

            double dx = x - it.handler.x, dy = y - it.handler.y;
            double dv2 = dx * dx + dy * dy;
            if (dv2 <= it.handler.threshold2) {
                double elapsedTime = glfwGetTime() * 1000.0 - it.handler.startTime;
                if (elapsedTime >= it.handler.timeToTrigger) it.handler.handler(it.handler.x, it.handler.y);
            } else {
                it.handler.startTime = glfwGetTime() * 1000.0;
                it.handler.x = x;
                it.handler.y = y;
            }
        }

        restoreContainer(cursorHoldHandlers, tempHandlers);
    }

    InputManager::CallbackHandler InputManager::registerKeyHandler(std::function<void(int, Modifier, Action)> handler) {
        keyHandlers.emplace_back(std::move(handler));
        return CallbackHandler{this, CallbackType::Key, keyHandlers.size() - 1};
    }

    InputManager::CallbackHandler InputManager::registerUtf8KeyHandler(std::function<void(const char*, Modifier, Action)> handler) {
        utf8KeyHandlers.emplace_back(std::move(handler));
        return CallbackHandler{this, CallbackType::Utf8Key, utf8KeyHandlers.size() - 1};
    }

    InputManager::CallbackHandler InputManager::registerMouseButtonHandler(std::function<void(MouseButton, Modifier, Action)> handler) {
        mouseButtonHandlers.emplace_back(std::move(handler));
        return CallbackHandler{this, CallbackType::MouseButton, mouseButtonHandlers.size() - 1};
    }

    InputManager::CallbackHandler InputManager::registerMouseScrollHandler(std::function<void(double, double)> handler) {
        mouseScrollHandlers.emplace_back(std::move(handler));
        return CallbackHandler{this, CallbackType::MouseScroll, mouseScrollHandlers.size() - 1};
    }

    InputManager::CallbackHandler InputManager::registerCursorMovementHandler(std::function<void(CursorMovement)> handler) {
        cursorMovementHandlers.emplace_back(std::move(handler));
        return CallbackHandler{this, CallbackType::CursorMovement, cursorMovementHandlers.size() - 1};
    }

    InputManager::CallbackHandler InputManager::registerCursorPositionHandler(std::function<void(double, double)> handler) {
        cursorPositionHandlers.emplace_back(std::move(handler));
        return CallbackHandler{this, CallbackType::CursorPosition, cursorPositionHandlers.size() - 1};
    }

    InputManager::CallbackHandler InputManager::registerCursorHoldHandler(double triggerTimeInMs, double threshold, std::function<void(double, double)> handler) {
        CursorHoldData data;
        data.handler = std::move(handler);
        data.threshold2 = threshold * threshold;
        data.timeToTrigger = triggerTimeInMs;
        data.x = data.y = 0;
        data.startTime = -1;
        cursorHoldHandlers.emplace_back(std::move(data));
        return CallbackHandler{this, CallbackType::CursorHold, cursorHoldHandlers.size() - 1};
    }

    InputManager::CallbackHandler InputManager::registerWindowResizeHandler(std::function<void(int, int)> handler) {
        windowResizeHandlers.emplace_back(std::move(handler));
        return CallbackHandler{this, CallbackType::WindowResize, windowResizeHandlers.size() - 1};
    }

    void InputManager::setMouseMode(MouseMode mouseMode) {
        int mod;
        switch (mouseMode) {
        case MouseMode::Disabled: mod = GLFW_CURSOR_DISABLED; break;
        case MouseMode::Enabled: mod = GLFW_CURSOR_NORMAL; break;
        }
        glfwSetInputMode(window, GLFW_CURSOR, mod);
    }

    bool InputManager::isKeyboardCaptured()
    {
        return false;
    }

    bool InputManager::isMouseCaptured() 
    {
        return false;
    }

    int InputManager::getSpaceScanCode()
    {
        return glfwGetKeyScancode(GLFW_KEY_SPACE);
    }

    int InputManager::getEnterScanCode()
    {
        return glfwGetKeyScancode(GLFW_KEY_ENTER);
    }

    int InputManager::getRightArrowScanCode()
    {
        return glfwGetKeyScancode(GLFW_KEY_RIGHT);
    }

    int InputManager::getLeftArrowScanCode()
    {
        return glfwGetKeyScancode(GLFW_KEY_LEFT);
    }

    void InputManager::CallbackHandler::enable_impl(bool enable)
    {
#ifdef INPUT_MANAGER_USE_AS_SINGLETON
        auto* pInputManager = &INPUT_MANAGER_SINGLETON_NAME;
#else
        auto* pInputManager = this->pInputManager;
#endif

        switch (type)
        {
        case CallbackType::Key:
            pInputManager->keyHandlers[index].enabled = enable;
            break;
        case CallbackType::Utf8Key:
            pInputManager->utf8KeyHandlers[index].enabled = enable;
            break;
        case CallbackType::MouseButton:
            pInputManager->mouseButtonHandlers[index].enabled = enable;
            break;
        case CallbackType::MouseScroll:
            pInputManager->mouseScrollHandlers[index].enabled = enable;
            break;
        case CallbackType::CursorMovement:
            pInputManager->cursorMovementHandlers[index].enabled = enable;
            break;
        case CallbackType::CursorPosition:
            pInputManager->cursorPositionHandlers[index].enabled = enable;
            break;
        case CallbackType::WindowResize:
            pInputManager->windowResizeHandlers[index].enabled = enable;
            break;
        case CallbackType::CursorHold:
            pInputManager->cursorHoldHandlers[index].enabled = enable;
            break;
        }
    }
}
