#include "Events.hpp"

#include "../Compositor.hpp"
#include "../helpers/WLClasses.hpp"
#include "../managers/InputManager.hpp"
#include "../render/Renderer.hpp"

// ---------------------------------------------------- //
//   _____  ________      _______ _____ ______  _____   //
//  |  __ \|  ____\ \    / /_   _/ ____|  ____|/ ____|  //
//  | |  | | |__   \ \  / /  | || |    | |__  | (___    //
//  | |  | |  __|   \ \/ /   | || |    |  __|  \___ \   //
//  | |__| | |____   \  /   _| || |____| |____ ____) |  //
//  |_____/|______|   \/   |_____\_____|______|_____/   //
//                                                      //
// ---------------------------------------------------- //

void Events::listener_keyboardDestroy(void* owner, void* data) {
    SKeyboard* PKEYBOARD = (SKeyboard*)owner;
    g_pInputManager->destroyKeyboard(PKEYBOARD);

    Debug::log(LOG, "Destroyed keyboard %x", PKEYBOARD);
}

void Events::listener_keyboardKey(void* owner, void* data) {
    SKeyboard* PKEYBOARD = (SKeyboard*)owner;
    g_pInputManager->onKeyboardKey((wlr_keyboard_key_event*)data, PKEYBOARD);
}

void Events::listener_keyboardMod(void* owner, void* data) {
    SKeyboard* PKEYBOARD = (SKeyboard*)owner;
    g_pInputManager->onKeyboardMod(data, PKEYBOARD);
}

void Events::listener_mouseFrame(wl_listener* listener, void* data) {
    wlr_seat_pointer_notify_frame(g_pCompositor->m_sSeat.seat);
}

void Events::listener_mouseMove(wl_listener* listener, void* data) {
    g_pInputManager->onMouseMoved((wlr_pointer_motion_event*)data);
}

void Events::listener_mouseMoveAbsolute(wl_listener* listener, void* data) {
    g_pInputManager->onMouseWarp((wlr_pointer_motion_absolute_event*)data);
}

void Events::listener_mouseButton(wl_listener* listener, void* data) {
    g_pInputManager->onMouseButton((wlr_pointer_button_event*)data);
}

void Events::listener_mouseAxis(wl_listener* listener, void* data) {
    const auto E = (wlr_pointer_axis_event*)data;

    wlr_seat_pointer_notify_axis(g_pCompositor->m_sSeat.seat, E->time_msec, E->orientation, E->delta, E->delta_discrete, E->source);
}

void Events::listener_requestMouse(wl_listener* listener, void* data) {
    const auto EVENT = (wlr_seat_pointer_request_set_cursor_event*)data;
    
    if (EVENT->seat_client == g_pCompositor->m_sSeat.seat->pointer_state.focused_client)
        wlr_cursor_set_surface(g_pCompositor->m_sWLRCursor, EVENT->surface, EVENT->hotspot_x, EVENT->hotspot_y);
}

void Events::listener_newInput(wl_listener* listener, void* data) {
    const auto DEVICE = (wlr_input_device*)data;

    switch(DEVICE->type) {
        case WLR_INPUT_DEVICE_KEYBOARD:
            Debug::log(LOG, "Attached a keyboard with name %s", DEVICE->name);
            g_pInputManager->newKeyboard(DEVICE);
            break;
        case WLR_INPUT_DEVICE_POINTER:
            Debug::log(LOG, "Attached a mouse with name %s", DEVICE->name);
            g_pInputManager->newMouse(DEVICE);
            break;
        default:
            break;
    }

    uint32_t capabilities = WL_SEAT_CAPABILITY_POINTER | WL_SEAT_CAPABILITY_KEYBOARD;

    wlr_seat_set_capabilities(g_pCompositor->m_sSeat.seat, capabilities);
}