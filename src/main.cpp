#include <hyprland/src/plugins/PluginSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>

inline HANDLE pHandle;

typedef void (*tStartSwipe)(void*, IPointer::SSwipeBeginEvent);
typedef void (*tUpdateSwipe)(void*, IPointer::SSwipeUpdateEvent);
typedef void (*tEndSwipe)(void*, IPointer::SSwipeEndEvent);

tStartSwipe pStartSwipe;
tUpdateSwipe pUpdateSwipe;
tEndSwipe pEndSwipe;

int button = BTN_SIDE;
float sensitivity = 1.f;
bool lockCursor = true;

bool fakeSwipeStarted = false;
Vector2D lastCursorPos;
PHLWORKSPACE lastWorkspace;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

void startSwipe() {
    static auto PSWIPEFINGERS = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_fingers");
    IPointer::SSwipeBeginEvent fakeEvent;
    fakeEvent.fingers = *PSWIPEFINGERS;
    pStartSwipe(g_pInputManager.get(), fakeEvent);
    lastCursorPos = g_pInputManager->getMouseCoordsInternal();
    lastWorkspace = g_pCompositor->getMonitorFromCursor()->activeWorkspace;
    fakeSwipeStarted = true;
}

void endSwipe() {
    fakeSwipeStarted = false;
    IPointer::SSwipeEndEvent fakeEvent;
    pEndSwipe(g_pInputManager.get(), fakeEvent);
}

void onMouseButton(void* thisptr, SCallbackInfo& info, std::any args) {

    const auto e = std::any_cast<IPointer::SButtonEvent>(args);

    const auto pressed = e.state == WL_POINTER_BUTTON_STATE_PRESSED;

    if (e.button == button) {
        info.cancelled = true;
        if (pressed)
            startSwipe();
        else
            endSwipe();         
    }

}

void onMouseMove(void* thisptr, SCallbackInfo& info, std::any args) {

    if (fakeSwipeStarted) {
        info.cancelled = lockCursor;
        static auto PSWIPEDIST = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_distance");
        const auto SWIPEDISTANCE = std::clamp(*PSWIPEDIST, (int64_t)1LL, (int64_t)UINT32_MAX);
        if (abs(g_pInputManager->m_sActiveSwipe.delta) >= SWIPEDISTANCE) return;
        const auto pos = std::any_cast<Vector2D>(args);
        const auto d = pos - lastCursorPos;
        IPointer::SSwipeUpdateEvent fakeEvent;
        const auto pMonitor = g_pCompositor->getMonitorFromCursor();
        const float curSwipeRatio = SWIPEDISTANCE / (pMonitor->vecSize.x * pMonitor->scale);
        fakeEvent.delta.x = d.x * curSwipeRatio * sensitivity;
        fakeEvent.delta.y = d.y * curSwipeRatio * sensitivity;
        pUpdateSwipe(g_pInputManager.get(), fakeEvent);
        if (!lockCursor)
            lastCursorPos = pos;
    }

}

void* findFunctionBySymbol(HANDLE inHandle, const std::string func, const std::string sym) {
    // should return all functions
    auto funcSearch = HyprlandAPI::findFunctionsByName(inHandle, func);
    for (auto f : funcSearch) {
        if (f.demangled.contains(sym))
            return f.address;
    }
    return nullptr;
}

void reloadConfig() {
    sensitivity = std::any_cast<Hyprlang::FLOAT>(HyprlandAPI::getConfigValue(pHandle, "plugin:hyswipe:sensitivity")->getValue());
    button = std::any_cast<Hyprlang::INT>(HyprlandAPI::getConfigValue(pHandle, "plugin:hyswipe:button")->getValue());
    lockCursor = std::any_cast<Hyprlang::INT>(HyprlandAPI::getConfigValue(pHandle, "plugin:hyswipe:lockCursor")->getValue());
}

Hyprutils::Memory::CSharedPointer<HOOK_CALLBACK_FN> configReloadHook;
Hyprutils::Memory::CSharedPointer<HOOK_CALLBACK_FN> mouseButtonHook;
Hyprutils::Memory::CSharedPointer<HOOK_CALLBACK_FN> mouseMoveHook;

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE inHandle) {
    pHandle = inHandle;

    HyprlandAPI::addConfigValue(pHandle, "plugin:hyswipe:button", Hyprlang::INT{BTN_SIDE});
    HyprlandAPI::addConfigValue(pHandle, "plugin:hyswipe:sensitivity", Hyprlang::FLOAT{1});
    HyprlandAPI::addConfigValue(pHandle, "plugin:hyswipe:lockCursor", Hyprlang::INT{1});

    configReloadHook = HyprlandAPI::registerCallbackDynamic(pHandle, "configReloaded", [&] (void* thisptr, SCallbackInfo& info, std::any data) { reloadConfig(); });
    HyprlandAPI::reloadConfig();

    pStartSwipe = (tStartSwipe)findFunctionBySymbol(pHandle, "onSwipeBegin", "onSwipeBegin");
    pUpdateSwipe = (tUpdateSwipe)findFunctionBySymbol(pHandle, "onSwipeUpdate", "onSwipeUpdate");
    pEndSwipe = (tEndSwipe)findFunctionBySymbol(pHandle, "onSwipeEnd", "onSwipeEnd");

    mouseButtonHook = HyprlandAPI::registerCallbackDynamic(pHandle, "mouseButton", onMouseButton);
    mouseMoveHook = HyprlandAPI::registerCallbackDynamic(pHandle, "mouseMove", onMouseMove);

    return {"Hyprswipe", "Gesture simulation", "KZdkm", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
}