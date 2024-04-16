#include <hyprland/src/plugins/PluginSystem.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigValue.hpp>

inline HANDLE pHandle;

typedef void (*tStartSwipe)(CInputManager*, wlr_pointer_swipe_begin_event*);
typedef void (*tUpdateSwipe)(CInputManager*, wlr_pointer_swipe_update_event*);
typedef void (*tEndSwipe)(CInputManager*, wlr_pointer_swipe_end_event*);

tStartSwipe pStartSwipe;
tUpdateSwipe pUpdateSwipe;
tEndSwipe pEndSwipe;

int button = BTN_SIDE;
float sensitivity = 1.f;
bool lockCursor = true;

bool fakeSwipeStarted = false;
Vector2D lastCursorPos;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

void onMouseButton(void* thisptr, SCallbackInfo& info, std::any args) {

    const auto e = std::any_cast<wlr_pointer_button_event*>(args);
    if (!e) return;

    const auto pressed = e->state == WL_POINTER_BUTTON_STATE_PRESSED;

    if (e->button == button) {
        info.cancelled = true;
        static auto PSWIPEFINGERS = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_fingers");
        if (pressed) {
            wlr_pointer_swipe_begin_event fakeEvent;
            fakeEvent.fingers = *PSWIPEFINGERS;
            pStartSwipe(g_pInputManager.get(), &fakeEvent);
            lastCursorPos = g_pInputManager->getMouseCoordsInternal();
            fakeSwipeStarted = true;
        }
        else {
            fakeSwipeStarted = false;
            wlr_pointer_swipe_end_event fakeEvent;
            pEndSwipe(g_pInputManager.get(), &fakeEvent);
        }
    }

}

void onMouseMove(void* thisptr, SCallbackInfo& info, std::any args) {

    if (fakeSwipeStarted && g_pInputManager->m_sActiveSwipe.pWorkspaceBegin) {
        info.cancelled = true;
        const auto pos = std::any_cast<Vector2D>(args);
        const auto d = pos - lastCursorPos;
        wlr_pointer_swipe_update_event fakeEvent;
        static auto PSWIPEFINGERS = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_fingers");
        static auto PSWIPEDIST             = CConfigValue<Hyprlang::INT>("gestures:workspace_swipe_distance");
        const auto pMonitor = g_pCompositor->getMonitorFromCursor();
        const float curSwipeRatio = *PSWIPEDIST / (pMonitor->vecSize.x * pMonitor->scale);
        fakeEvent.fingers = *PSWIPEFINGERS;
        fakeEvent.dx = d.x * curSwipeRatio * sensitivity;
        fakeEvent.dy = d.y * curSwipeRatio * sensitivity;
        pUpdateSwipe(g_pInputManager.get(), &fakeEvent);
        if (lockCursor)
            wlr_cursor_warp(g_pCompositor->m_sWLRCursor, NULL, lastCursorPos.x, lastCursorPos.y);
        else
            lastCursorPos = pos;
    }

}

void onMouseAxis(void* thisptr, SCallbackInfo& info, std::any args) {

    const auto e = std::any_cast<wlr_pointer_axis_event*>(std::any_cast<std::unordered_map<std::string, std::any>>(args)["event"]);
    if (!e) return;

    const auto pMonitor = g_pCompositor->getMonitorFromCursor();
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

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE inHandle) {
    pHandle = inHandle;

    HyprlandAPI::addConfigValue(pHandle, "plugin:hyswipe:button", Hyprlang::INT{BTN_SIDE});
    HyprlandAPI::addConfigValue(pHandle, "plugin:hyswipe:sensitivity", Hyprlang::FLOAT{1});
    HyprlandAPI::addConfigValue(pHandle, "plugin:hyswipe:lockCursor", Hyprlang::INT{1});

    HyprlandAPI::registerCallbackDynamic(pHandle, "configReloaded", [&] (void* thisptr, SCallbackInfo& info, std::any data) { reloadConfig(); });
    HyprlandAPI::reloadConfig();

    pStartSwipe = (tStartSwipe)findFunctionBySymbol(pHandle, "onSwipeBegin", "onSwipeBegin");
    pUpdateSwipe = (tUpdateSwipe)findFunctionBySymbol(pHandle, "onSwipeUpdate", "onSwipeUpdate");
    pEndSwipe = (tEndSwipe)findFunctionBySymbol(pHandle, "onSwipeEnd", "onSwipeEnd");

    HyprlandAPI::registerCallbackDynamic(pHandle, "mouseButton", onMouseButton);
    HyprlandAPI::registerCallbackDynamic(pHandle, "mouseAxis", onMouseAxis);
    HyprlandAPI::registerCallbackDynamic(pHandle, "mouseMove", onMouseMove);

    return {"Hyprswipe", "Gesture simulation", "KZdkm", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
}