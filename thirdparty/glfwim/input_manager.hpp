#ifndef INPUT_MANAGER_HPP
#define INPUT_MANAGER_HPP

#include <functional>
#include <vector>
#include <cstring>

struct GLFWwindow;
namespace glfwim {
    enum class Modifier {
        None = 0, Shift = 1, Control = 2, Alt = 4, Super = 8
    };

    enum class Action {
        Release = 0, Press = 1, Repeat = 2
    };

    enum class MouseButton {
        Left = 0, Right = 1, Middle = 2
    };

    enum class CursorMovement {
        Leave = 0, Enter = 1
    };

    enum class MouseMode {
        Disabled = 0, Enabled = 1
    };

    class InputManager {
    private:
        struct CursorHoldData {
            std::function<void(double, double)> handler;
            double x, y;
            double threshold2, timeToTrigger, startTime;
        };

#ifdef INPUT_MANAGER_USE_AS_SINGLETON
    private:
        InputManager() = default;

    public:
        static InputManager& Instance() {
            static InputManager instance;
            return instance;
        }
#endif
    public:
        void initialize(GLFWwindow* window);
        void pollEvents();
        void setMouseMode(MouseMode mouseMode);
        void waitUntilNextEventHandling();
        void waitUntilNextEventHandling(double timeout);
        void pauseInputHandling();
        void continueInputHandling();

        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;

    public:
        enum class CallbackType { Key, Utf8Key, MouseButton, MouseScroll, CursorMovement, CursorPosition, WindowResize, CursorHold };

        class CallbackHandler {
        public:
            CallbackHandler(InputManager* inputManager, CallbackType type, size_t index) : 
#ifndef INPUT_MANAGER_USE_AS_SINGLETON
                pInputManager{inputManager}, 
#endif
                type{type}, index{index} {}

            void enable() { enable_impl(true); }
            void disable() { enable_impl(false); }

        private:
            void enable_impl(bool enable);

        private:
#ifndef INPUT_MANAGER_USE_AS_SINGLETON
            InputManager* pInputManager;
#endif
            CallbackType type;
            size_t index;
        };

        friend class CallbackHandler;

    public:
        CallbackHandler registerKeyHandler(std::function<void(int, Modifier, Action)> handler);

        template <typename H>
        CallbackHandler registerKeyHandler(int scancode, H handler) {
            return registerKeyHandler([h = std::move(handler), s = scancode](int scancode, Modifier modifier, Action action){
                if (scancode == s) h(modifier, action);
            });
        }

        template <typename H>
        CallbackHandler registerKeyHandler(int scancode, Modifier modifier, H handler) {
            return registerKeyHandler([h = std::move(handler), s = scancode, m = modifier](int scancode, Modifier modifier, Action action){
                if (scancode == s && modifier == m) h(action);
            });
        }

        template <typename H>
        CallbackHandler registerKeyHandler(int scancode, Modifier modifier, Action action, H handler) {
            return registerKeyHandler([h = std::move(handler), s = scancode, m = modifier, a = action](int scancode, Modifier modifier, Action action){
                if (scancode == s && modifier == m && action == a) h();
            });
        }

        CallbackHandler registerUtf8KeyHandler(std::function<void(const char*, Modifier, Action)> handler);

        template <typename H>
        CallbackHandler registerUtf8KeyHandler(const char* utf8code, H handler) {
            if (!strcmp(utf8code, " ")) {
                return registerKeyHandler(getSpaceScanCode(), handler);
            }
            else if (!strcmp(utf8code, "\n")) {
                return registerKeyHandler(getEnterScanCode(), handler);
            }
            else if (!strcmp(utf8code, "->")) {
                return registerKeyHandler(getRightArrowScanCode(), handler);
            }
            else if (!strcmp(utf8code, "<-")) {
                return registerKeyHandler(getLeftArrowScanCode(), handler);
            }
            else {
                return registerUtf8KeyHandler([h = std::move(handler), u = utf8code](const char* utf8code, Modifier modifier, Action action){
                    if (!strcmp(u, utf8code)) h(modifier, action);
                });
            }
        }

        template <typename H>
        CallbackHandler registerUtf8KeyHandler(const char* utf8code, Modifier modifier, H handler) {
            if (!strcmp(utf8code, " ")) {
                return registerKeyHandler(getSpaceScanCode(), modifier, handler);
            }
            else if (!strcmp(utf8code, "\n")) {
                return registerKeyHandler(getEnterScanCode(), modifier, handler);
            }
            else if (!strcmp(utf8code, "->")) {
                return registerKeyHandler(getRightArrowScanCode(), modifier, handler);
            }
            else if (!strcmp(utf8code, "<-")) {
                return registerKeyHandler(getLeftArrowScanCode(), modifier, handler);
            }
            else {
                return registerUtf8KeyHandler([h = std::move(handler), u = utf8code, m = modifier](const char* utf8code, Modifier modifier, Action action){
                    if (!strcmp(u, utf8code) && m == modifier) h(action);
                });
            }
        }

        template <typename H>
        CallbackHandler registerUtf8KeyHandler(const char* utf8code, Modifier modifier, Action action, H handler) {
            if (!strcmp(utf8code, " ")) {
                return registerKeyHandler(getSpaceScanCode(), modifier, action, handler);
            }
            else if (!strcmp(utf8code, "\n")) {
                return registerKeyHandler(getEnterScanCode(), modifier, action, handler);
            }
            else if (!strcmp(utf8code, "->")) {
                return registerKeyHandler(getRightArrowScanCode(), modifier, action, handler);
            }
            else if (!strcmp(utf8code, "<-")) {
                return registerKeyHandler(getLeftArrowScanCode(), modifier, action, handler);
            }
            else {
                return registerUtf8KeyHandler([h = std::move(handler), u = utf8code, m = modifier, a = action](const char* utf8code, Modifier modifier, Action action){
                    if (!strcmp(u, utf8code) && m == modifier && a == action) h();
                });
            }
        }

        CallbackHandler registerMouseButtonHandler(std::function<void(MouseButton, Modifier, Action)> handler);

        template <typename H>
        CallbackHandler registerMouseButtonHandler(MouseButton mouseButton, H handler) {
            return registerMouseButtonHandler([h = std::move(handler), mb = mouseButton](MouseButton mouseButton, Modifier modifier, Action action){
                if (mb == mouseButton) h(modifier, action);
            });
        }

        template <typename H>
        CallbackHandler registerMouseButtonHandler(MouseButton mouseButton, Modifier modifier, H handler) {
            return registerMouseButtonHandler([h = std::move(handler), mb = mouseButton, m = modifier](MouseButton mouseButton, Modifier modifier, Action action){
                if (mb == mouseButton && m == modifier) h(action);
            });
        }

        template <typename H>
        CallbackHandler registerMouseButtonHandler(MouseButton mouseButton, Modifier modifier, Action action, H handler) {
            return registerMouseButtonHandler([h = std::move(handler), mb = mouseButton, m = modifier, a = action](MouseButton mouseButton, Modifier modifier, Action action){
                if (mb == mouseButton && m == modifier && a == action) h();
            });
        }

        CallbackHandler registerMouseScrollHandler(std::function<void(double, double)> handler);

        CallbackHandler registerCursorMovementHandler(std::function<void(CursorMovement)> handler);

        template <typename H>
        CallbackHandler registerCursorMovementHandler(CursorMovement movement, H handler) {
            return registerCursorMovementHandler([h = std::move(handler), m = movement](CursorMovement movement){
                if (m == movement) h();
            });
        }

        CallbackHandler registerCursorPositionHandler(std::function<void(double, double)> handler);

        CallbackHandler registerCursorHoldHandler(double triggerTimeInMs, double threshold, std::function<void(double, double)> handler);

        CallbackHandler registerWindowResizeHandler(std::function<void(int, int)> handler);

    private:
        void elapsedTime();
        bool isKeyboardCaptured();
        bool isMouseCaptured();
        static int getSpaceScanCode();
        static int getEnterScanCode();
        static int getRightArrowScanCode();
        static int getLeftArrowScanCode();

        template <typename Container>
        static Container backupContainer(Container& container)
        {
            auto ret = std::move(container);
            container.clear();
            return ret;
        }

        template <typename Container>
        static void restoreContainer(Container& container, Container& backup)
        {
            std::swap(container, backup);
            if (!backup.empty()) {
                container.insert(std::end(container), std::make_move_iterator(std::begin(backup)), std::make_move_iterator(std::end(backup)));
            }
        }

        template <typename T>
        struct HandlerHolder {
            HandlerHolder(T handler) : handler{std::move(handler)} {}

            T handler;
            volatile bool enabled = true;
        };

    private:
        GLFWwindow* window;
        volatile bool finishedInputHandling = false;
        volatile bool paused = false;

    private:
        std::vector<HandlerHolder<std::function<void(int, Modifier, Action)>>> keyHandlers;
        std::vector<HandlerHolder<std::function<void(const char*, Modifier, Action)>>> utf8KeyHandlers;
        std::vector<HandlerHolder<std::function<void(MouseButton, Modifier, Action)>>> mouseButtonHandlers;
        std::vector<HandlerHolder<std::function<void(double, double)>>> mouseScrollHandlers;
        std::vector<HandlerHolder<std::function<void(CursorMovement)>>> cursorMovementHandlers;
        std::vector<HandlerHolder<std::function<void(double, double)>>> cursorPositionHandlers;
        std::vector<HandlerHolder<std::function<void(int, int)>>> windowResizeHandlers;
        std::vector<HandlerHolder<CursorHoldData>> cursorHoldHandlers;
        std::vector<char> keyStates;
    };

#ifdef INPUT_MANAGER_USE_AS_SINGLETON
    inline InputManager& INPUT_MANAGER_SINGLETON_NAME = InputManager::Instance();
#endif
}
#endif
