// SPDX-License-Identifier: MIT
// usdl2 native module (MicroPython + CircuitPython unix/windows).

#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "usdl2.h"

#include <string.h>
#include <unistd.h>

// --- SDL_DEFINE_PIXELFORMAT --------------------------------------

static mp_obj_t usdl2_define_pixelformat(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    mp_int_t type = mp_obj_get_int(args[0]);
    mp_int_t order = mp_obj_get_int(args[1]);
    mp_int_t layout = mp_obj_get_int(args[2]);
    mp_int_t bits = mp_obj_get_int(args[3]);
    mp_int_t bytes = mp_obj_get_int(args[4]);
    return mp_obj_new_int(USDL2_DEFINE_PIXELFORMAT(type, order, layout, bits, bytes));
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_SDL_DEFINE_PIXELFORMAT_fun_obj, 5, 5, usdl2_define_pixelformat);

// --- Event type --------------------------------------------------

_Static_assert(sizeof(SDL_Event) == USDL2_EVENT_SIZE,
    "usdl2: SDL_Event size changed; update USDL2_EVENT_SIZE and event layout");

typedef struct {
    mp_obj_base_t base;
    uint8_t data[USDL2_EVENT_SIZE];
} usdl2_event_obj_t;

typedef struct {
    mp_obj_base_t base;
    mp_obj_t parent;
    uint16_t offset;
} usdl2_subview_obj_t;

static const mp_obj_type_t usdl2_subview_type;

const mp_obj_type_t usdl2_event_type;

static uint8_t *usdl2_parent_data(mp_obj_t parent) {
    usdl2_event_obj_t *self = MP_OBJ_TO_PTR(parent);
    return self->data;
}

static uint32_t usdl2_read_u32(const uint8_t *data, uint16_t offset) {
    uint32_t value;
    memcpy(&value, data + offset, sizeof(value));
    return value;
}

static int32_t usdl2_read_i32(const uint8_t *data, uint16_t offset) {
    int32_t value;
    memcpy(&value, data + offset, sizeof(value));
    return value;
}

static int16_t usdl2_read_i16(const uint8_t *data, uint16_t offset) {
    int16_t value;
    memcpy(&value, data + offset, sizeof(value));
    return value;
}

static bool usdl2_is_joystick_event(uint32_t type) {
    return type >= SDL_JOYAXISMOTION && type <= SDL_JOYDEVICEREMOVED;
}

static mp_obj_t usdl2_subview_make(mp_obj_t parent, uint16_t offset) {
    usdl2_subview_obj_t *view = mp_obj_malloc(usdl2_subview_obj_t, &usdl2_subview_type);
    view->parent = parent;
    view->offset = offset;
    return MP_OBJ_FROM_PTR(view);
}

static void usdl2_subview_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    usdl2_subview_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const uint8_t *data = usdl2_parent_data(self->parent);
    const uint16_t base = self->offset;

    if (dest[0] != MP_OBJ_NULL) {
        return;
    }

    switch (attr) {
        case MP_QSTR_windowID:
            dest[0] = mp_obj_new_int_from_uint(usdl2_read_u32(data, base + 0));
            return;
        case MP_QSTR_which:
            if (usdl2_is_joystick_event(usdl2_read_u32(data, 0))) {
                dest[0] = mp_obj_new_int(usdl2_read_i32(data, base + 0));
            } else {
                dest[0] = mp_obj_new_int_from_uint(usdl2_read_u32(data, base + 4));
            }
            return;
        case MP_QSTR_state: {
            uint32_t type = usdl2_read_u32(data, 0);
            if (base == 8 && type == SDL_MOUSEMOTION) {
                dest[0] = mp_obj_new_int_from_uint(usdl2_read_u32(data, base + 8));
            } else if (type == SDL_JOYBUTTONDOWN || type == SDL_JOYBUTTONUP) {
                dest[0] = mp_obj_new_int(data[base + 5]);
            } else {
                dest[0] = mp_obj_new_int_from_uint(data[base + 4]);
            }
            return;
        }
        case MP_QSTR_x:
            dest[0] = mp_obj_new_int(usdl2_read_i32(data, base + 12));
            return;
        case MP_QSTR_y:
            dest[0] = mp_obj_new_int(usdl2_read_i32(data, base + 16));
            return;
        case MP_QSTR_xrel: {
            uint32_t type = usdl2_read_u32(data, 0);
            if (type == SDL_JOYBALLMOTION) {
                dest[0] = mp_obj_new_int(usdl2_read_i16(data, base + 8));
            } else {
                dest[0] = mp_obj_new_int(usdl2_read_i32(data, base + 20));
            }
            return;
        }
        case MP_QSTR_yrel: {
            uint32_t type = usdl2_read_u32(data, 0);
            if (type == SDL_JOYBALLMOTION) {
                dest[0] = mp_obj_new_int(usdl2_read_i16(data, base + 10));
            } else {
                dest[0] = mp_obj_new_int(usdl2_read_i32(data, base + 24));
            }
            return;
        }
        case MP_QSTR_button: {
            uint32_t type = usdl2_read_u32(data, 0);
            if (type == SDL_JOYBUTTONDOWN || type == SDL_JOYBUTTONUP) {
                dest[0] = mp_obj_new_int(data[base + 4]);
            } else {
                dest[0] = mp_obj_new_int(data[base + 8]);
            }
            return;
        }
        case MP_QSTR_clicks:
            dest[0] = mp_obj_new_int(data[base + 10]);
            return;
        case MP_QSTR_direction:
            dest[0] = mp_obj_new_int_from_uint(usdl2_read_u32(data, base + 16));
            return;
        case MP_QSTR_preciseX: {
            float value;
            memcpy(&value, data + base + 20, sizeof(value));
            dest[0] = mp_obj_new_float(value);
            return;
        }
        case MP_QSTR_preciseY: {
            float value;
            memcpy(&value, data + base + 24, sizeof(value));
            dest[0] = mp_obj_new_float(value);
            return;
        }
        case MP_QSTR_repeat:
            dest[0] = mp_obj_new_int(data[base + 5]);
            return;
        case MP_QSTR_keysym:
            dest[0] = usdl2_subview_make(self->parent, base + 8);
            return;
        case MP_QSTR_scancode:
            dest[0] = mp_obj_new_int(usdl2_read_i32(data, base + 0));
            return;
        case MP_QSTR_sym:
            dest[0] = mp_obj_new_int(usdl2_read_i32(data, base + 4));
            return;
        case MP_QSTR_mod:
            dest[0] = mp_obj_new_int_from_uint(usdl2_read_u32(data, base + 8) & 0xffff);
            return;
        case MP_QSTR_axis:
            dest[0] = mp_obj_new_int(data[base + 4]);
            return;
        case MP_QSTR_value: {
            uint32_t type = usdl2_read_u32(data, 0);
            if (type == SDL_JOYAXISMOTION) {
                dest[0] = mp_obj_new_int(usdl2_read_i16(data, base + 8));
            } else if (type == SDL_JOYHATMOTION) {
                dest[0] = mp_obj_new_int(data[base + 5]);
            } else {
                dest[0] = MP_OBJ_NULL;
            }
            return;
        }
        case MP_QSTR_ball:
            dest[0] = mp_obj_new_int(data[base + 4]);
            return;
        case MP_QSTR_hat:
            dest[0] = mp_obj_new_int(data[base + 4]);
            return;
        default:
            dest[0] = MP_OBJ_NULL;
            return;
    }
}

static MP_DEFINE_CONST_OBJ_TYPE(
    usdl2_subview_type,
    MP_QSTR_usdl2_subview,
    MP_TYPE_FLAG_NONE,
    attr, usdl2_subview_attr
    );

uint8_t *usdl2_event_buffer(mp_obj_t self_in) {
    usdl2_event_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return self->data;
}

mp_int_t usdl2_event_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    usdl2_event_obj_t *self = MP_OBJ_TO_PTR(self_in);
    bufinfo->buf = self->data;
    bufinfo->len = USDL2_EVENT_SIZE;
    bufinfo->typecode = 'B';
    return 0;
}

mp_obj_t usdl2_event_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    usdl2_event_obj_t *self = mp_obj_malloc(usdl2_event_obj_t, type);
    memset(self->data, 0, USDL2_EVENT_SIZE);
    if (n_args == 1) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[0], &bufinfo, MP_BUFFER_READ);
        if (bufinfo.len < USDL2_EVENT_SIZE) {
            mp_raise_ValueError(MP_ERROR_TEXT("event buffer too small"));
        }
        memcpy(self->data, bufinfo.buf, USDL2_EVENT_SIZE);
    }
    return MP_OBJ_FROM_PTR(self);
}

static void usdl2_event_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest) {
    usdl2_event_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if (dest[0] != MP_OBJ_NULL) {
        return;
    }

    switch (attr) {
        case MP_QSTR_type:
            dest[0] = mp_obj_new_int_from_uint(usdl2_read_u32(self->data, 0));
            return;
        case MP_QSTR_timestamp:
            dest[0] = mp_obj_new_int_from_uint(usdl2_read_u32(self->data, 4));
            return;
        case MP_QSTR_motion:
        case MP_QSTR_key:
        case MP_QSTR_button:
        case MP_QSTR_wheel:
        case MP_QSTR_jaxis:
        case MP_QSTR_jball:
        case MP_QSTR_jhat:
        case MP_QSTR_jbutton:
            dest[0] = usdl2_subview_make(self_in, 8);
            return;
        default:
            dest[0] = MP_OBJ_NULL;
            return;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    usdl2_event_type,
    MP_QSTR_Event,
    MP_TYPE_FLAG_NONE,
    make_new, usdl2_event_make_new,
    attr, usdl2_event_attr,
    buffer, usdl2_event_get_buffer
    );

// --- SDL bindings ------------------------------------------------

void *usdl2_ptr_from_obj(mp_obj_t obj) {
    return (void *)(uintptr_t)mp_obj_get_int_truncated(obj);
}

mp_obj_t usdl2_ptr_obj(void *ptr) {
    return mp_obj_new_int_from_uint((uintptr_t)ptr);
}

bool usdl2_obj_is_none_or_null(mp_obj_t obj) {
    if (obj == mp_const_none) {
        return true;
    }
    if (mp_obj_is_int(obj)) {
        return mp_obj_get_int_truncated(obj) == 0;
    }
    return false;
}

void usdl2_rect_from_bytes(mp_obj_t obj, SDL_Rect *rect) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(obj, &bufinfo, MP_BUFFER_READ);
    if (bufinfo.len < (mp_uint_t)sizeof(SDL_Rect)) {
        mp_raise_ValueError(MP_ERROR_TEXT("rect buffer too small"));
    }
    const int32_t *values = (const int32_t *)bufinfo.buf;
    rect->x = values[0];
    rect->y = values[1];
    rect->w = values[2];
    rect->h = values[3];
}

SDL_Rect *usdl2_optional_rect(mp_obj_t obj, SDL_Rect *storage) {
    if (usdl2_obj_is_none_or_null(obj)) {
        return NULL;
    }
    usdl2_rect_from_bytes(obj, storage);
    return storage;
}

mp_obj_t usdl2_init(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    int rc = SDL_Init((Uint32)mp_obj_get_int(args[0]));
    return mp_obj_new_int(rc);
}

mp_obj_t usdl2_init_subsystem(mp_obj_t flags_in) {
    int rc = SDL_InitSubSystem((Uint32)mp_obj_get_int(flags_in));
    return mp_obj_new_int(rc);
}

mp_obj_t usdl2_quit(void) {
    SDL_Quit();
    return mp_const_none;
}

mp_obj_t usdl2_process_exit(mp_obj_t code_in) {
    _exit((int)mp_obj_get_int(code_in));
    return mp_const_none;
}

mp_obj_t usdl2_get_error(void) {
    const char *err = SDL_GetError();
    if (err == NULL || err[0] == '\0') {
        return mp_obj_new_str("", 0);
    }
    return mp_obj_new_str(err, strlen(err));
}

mp_obj_t usdl2_window_title(mp_obj_t title_in) {
    if (mp_obj_is_str(title_in)) {
        return title_in;
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(title_in, &bufinfo, MP_BUFFER_READ);
    return mp_obj_new_str(bufinfo.buf, bufinfo.len);
}

mp_obj_t usdl2_create_window(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    const char *title = mp_obj_str_get_str(usdl2_window_title(args[0]));
    SDL_Window *window = SDL_CreateWindow(
        title,
        (int)mp_obj_get_int(args[1]),
        (int)mp_obj_get_int(args[2]),
        (int)mp_obj_get_int(args[3]),
        (int)mp_obj_get_int(args[4]),
        (Uint32)mp_obj_get_int(args[5]));
    return usdl2_ptr_obj(window);
}

mp_obj_t usdl2_destroy_window(mp_obj_t win_in) {
    SDL_DestroyWindow((SDL_Window *)usdl2_ptr_from_obj(win_in));
    return mp_const_none;
}

mp_obj_t usdl2_set_window_size(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    SDL_SetWindowSize(
        (SDL_Window *)usdl2_ptr_from_obj(args[0]),
        (int)mp_obj_get_int(args[1]),
        (int)mp_obj_get_int(args[2]));
    return mp_const_none;
}

mp_obj_t usdl2_create_renderer(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    SDL_Renderer *renderer = SDL_CreateRenderer(
        (SDL_Window *)usdl2_ptr_from_obj(args[0]),
        (int)mp_obj_get_int(args[1]),
        (Uint32)mp_obj_get_int(args[2]));
    return usdl2_ptr_obj(renderer);
}

mp_obj_t usdl2_destroy_renderer(mp_obj_t renderer_in) {
    SDL_DestroyRenderer((SDL_Renderer *)usdl2_ptr_from_obj(renderer_in));
    return mp_const_none;
}

mp_obj_t usdl2_set_render_draw_color(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    int rc = SDL_SetRenderDrawColor(
        (SDL_Renderer *)usdl2_ptr_from_obj(args[0]),
        (Uint8)mp_obj_get_int(args[1]),
        (Uint8)mp_obj_get_int(args[2]),
        (Uint8)mp_obj_get_int(args[3]),
        (Uint8)mp_obj_get_int(args[4]));
    return mp_obj_new_int(rc);
}

mp_obj_t usdl2_set_render_target(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    SDL_Texture *texture = NULL;
    if (!usdl2_obj_is_none_or_null(args[1])) {
        texture = (SDL_Texture *)usdl2_ptr_from_obj(args[1]);
    }
    int rc = SDL_SetRenderTarget((SDL_Renderer *)usdl2_ptr_from_obj(args[0]), texture);
    return mp_obj_new_int(rc);
}

mp_obj_t usdl2_render_clear(mp_obj_t renderer_in) {
    SDL_RenderClear((SDL_Renderer *)usdl2_ptr_from_obj(renderer_in));
    return mp_const_none;
}

mp_obj_t usdl2_render_copy(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    SDL_Rect src_storage;
    SDL_Rect dst_storage;
    SDL_Rect *src = usdl2_optional_rect(args[2], &src_storage);
    SDL_Rect *dst = usdl2_optional_rect(args[3], &dst_storage);
    SDL_RenderCopy(
        (SDL_Renderer *)usdl2_ptr_from_obj(args[0]),
        (SDL_Texture *)usdl2_ptr_from_obj(args[1]),
        src,
        dst);
    return mp_const_none;
}

mp_obj_t usdl2_render_copy_ex(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    SDL_Rect src_storage;
    SDL_Rect dst_storage;
    SDL_Rect *src = usdl2_optional_rect(args[2], &src_storage);
    SDL_Rect *dst = usdl2_optional_rect(args[3], &dst_storage);
    SDL_Point center_storage;
    SDL_Point *center = NULL;
    if (!usdl2_obj_is_none_or_null(args[5])) {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[5], &bufinfo, MP_BUFFER_READ);
        if (bufinfo.len < (mp_uint_t)sizeof(int32_t) * 2) {
            mp_raise_ValueError(MP_ERROR_TEXT("center buffer too small"));
        }
        const int32_t *values = (const int32_t *)bufinfo.buf;
        center_storage.x = values[0];
        center_storage.y = values[1];
        center = &center_storage;
    }
    SDL_RenderCopyEx(
        (SDL_Renderer *)usdl2_ptr_from_obj(args[0]),
        (SDL_Texture *)usdl2_ptr_from_obj(args[1]),
        src,
        dst,
        mp_obj_get_float(args[4]),
        center,
        (int)mp_obj_get_int(args[6]));
    return mp_const_none;
}

mp_obj_t usdl2_render_present(mp_obj_t renderer_in) {
    SDL_RenderPresent((SDL_Renderer *)usdl2_ptr_from_obj(renderer_in));
    return mp_const_none;
}

mp_obj_t usdl2_render_fill_rect(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    SDL_Rect rect;
    usdl2_rect_from_bytes(args[1], &rect);
    int rc = SDL_RenderFillRect((SDL_Renderer *)usdl2_ptr_from_obj(args[0]), &rect);
    return mp_obj_new_int(rc);
}

mp_obj_t usdl2_render_set_logical_size(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    int rc = SDL_RenderSetLogicalSize(
        (SDL_Renderer *)usdl2_ptr_from_obj(args[0]),
        (int)mp_obj_get_int(args[1]),
        (int)mp_obj_get_int(args[2]));
    return mp_obj_new_int(rc);
}

mp_obj_t usdl2_create_texture(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    SDL_Texture *texture = SDL_CreateTexture(
        (SDL_Renderer *)usdl2_ptr_from_obj(args[0]),
        (Uint32)mp_obj_get_int(args[1]),
        (int)mp_obj_get_int(args[2]),
        (int)mp_obj_get_int(args[3]),
        (int)mp_obj_get_int(args[4]));
    return usdl2_ptr_obj(texture);
}

mp_obj_t usdl2_destroy_texture(mp_obj_t texture_in) {
    SDL_DestroyTexture((SDL_Texture *)usdl2_ptr_from_obj(texture_in));
    return mp_const_none;
}

mp_obj_t usdl2_set_texture_blend_mode(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    int rc = SDL_SetTextureBlendMode(
        (SDL_Texture *)usdl2_ptr_from_obj(args[0]),
        (SDL_BlendMode)mp_obj_get_int(args[1]));
    return mp_obj_new_int(rc);
}

mp_obj_t usdl2_update_texture(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    SDL_Rect rect_storage;
    SDL_Rect *rect = usdl2_optional_rect(args[1], &rect_storage);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);
    int rc = SDL_UpdateTexture(
        (SDL_Texture *)usdl2_ptr_from_obj(args[0]),
        rect,
        bufinfo.buf,
        (int)mp_obj_get_int(args[3]));
    return mp_obj_new_int(rc);
}

static uint8_t *usdl2_event_buf_from_obj(mp_obj_t event_in) {
    if (mp_obj_is_type(event_in, &usdl2_event_type)) {
        return usdl2_event_buffer(event_in);
    }
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(event_in, &bufinfo, MP_BUFFER_WRITE);
    if (bufinfo.len < USDL2_EVENT_SIZE) {
        mp_raise_ValueError(MP_ERROR_TEXT("event buffer too small"));
    }
    return (uint8_t *)bufinfo.buf;
}

mp_obj_t usdl2_poll_event(mp_obj_t event_in) {
    SDL_Event event;
    int rc = SDL_PollEvent(&event);
    if (rc) {
        uint8_t *buf = usdl2_event_buf_from_obj(event_in);
        memcpy(buf, &event, USDL2_EVENT_SIZE);
    }
    return mp_obj_new_bool(rc);
}

mp_obj_t usdl2_get_key_name(mp_obj_t sym_in) {
    const char *name = SDL_GetKeyName((SDL_Keycode)mp_obj_get_int(sym_in));
    if (name == NULL) {
        return mp_obj_new_str("", 0);
    }
    return mp_obj_new_str(name, strlen(name));
}

mp_obj_t usdl2_num_joysticks(void) {
    return mp_obj_new_int(SDL_NumJoysticks());
}

mp_obj_t usdl2_joystick_open(mp_obj_t index_in) {
    SDL_Joystick *joystick = SDL_JoystickOpen((int)mp_obj_get_int(index_in));
    return usdl2_ptr_obj(joystick);
}

mp_obj_t usdl2_joystick_close(mp_obj_t joystick_in) {
    SDL_JoystickClose((SDL_Joystick *)usdl2_ptr_from_obj(joystick_in));
    return mp_const_none;
}

mp_obj_t usdl2_joystick_instance_id(mp_obj_t joystick_in) {
    return mp_obj_new_int(SDL_JoystickInstanceID((SDL_Joystick *)usdl2_ptr_from_obj(joystick_in)));
}

mp_obj_t usdl2_rect_helper(size_t n_args, const mp_obj_t *args) {
    int32_t values[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < n_args && i < 4; i++) {
        values[i] = (int32_t)mp_obj_get_int(args[i]);
    }
    return mp_obj_new_bytes((const byte *)values, sizeof(values));
}

mp_obj_t usdl2_point_helper(size_t n_args, const mp_obj_t *args) {
    int32_t values[2] = {0, 0};
    for (size_t i = 0; i < n_args && i < 2; i++) {
        values[i] = (int32_t)mp_obj_get_int(args[i]);
    }
    return mp_obj_new_bytes((const byte *)values, sizeof(values));
}

mp_obj_t usdl2_event_helper(size_t n_args, const mp_obj_t *args) {
    if (n_args == 0) {
        return usdl2_event_make_new(&usdl2_event_type, 0, 0, NULL);
    }
    if (mp_obj_is_type(args[0], &usdl2_event_type)) {
        return args[0];
    }
    return usdl2_event_make_new(&usdl2_event_type, 1, 0, args);
}

// --- Timers ------------------------------------------------------

#define USDL2_TIMER_MAX 8

typedef struct {
    mp_obj_base_t base;
    mp_obj_t callback;
} usdl2_timer_cb_obj_t;

typedef struct usdl2_timer_entry {
    struct usdl2_timer_entry *next;
    SDL_TimerID id;
    mp_obj_t callback;
    mp_obj_t user_param;
    Uint32 interval;
    Uint32 last_interval;
    uint8_t slot;
} usdl2_timer_entry_t;

const mp_obj_type_t usdl2_timer_cb_type;

static usdl2_timer_entry_t *usdl2_timer_list;
static usdl2_timer_entry_t *usdl2_timer_by_slot[USDL2_TIMER_MAX];

static usdl2_timer_entry_t *usdl2_timer_find(SDL_TimerID id) {
    for (usdl2_timer_entry_t *entry = usdl2_timer_list; entry != NULL; entry = entry->next) {
        if (entry->id == id) {
            return entry;
        }
    }
    return NULL;
}

static int usdl2_timer_alloc_slot(usdl2_timer_entry_t *entry) {
    for (int i = 0; i < USDL2_TIMER_MAX; i++) {
        if (usdl2_timer_by_slot[i] == NULL) {
            usdl2_timer_by_slot[i] = entry;
            entry->slot = (uint8_t)i;
            return i;
        }
    }
    return -1;
}

static void usdl2_timer_free_slot(usdl2_timer_entry_t *entry) {
    if (entry->slot < USDL2_TIMER_MAX) {
        usdl2_timer_by_slot[entry->slot] = NULL;
    }
}

static void usdl2_timer_unlink(usdl2_timer_entry_t *entry) {
    usdl2_timer_entry_t **prev = &usdl2_timer_list;
    while (*prev != NULL) {
        if (*prev == entry) {
            *prev = entry->next;
            usdl2_timer_free_slot(entry);
            m_del(usdl2_timer_entry_t, entry, 1);
            return;
        }
        prev = &(*prev)->next;
    }
}

static mp_obj_t usdl2_timer_dispatch(mp_obj_t slot_in) {
    int slot = MP_OBJ_SMALL_INT_VALUE(slot_in);
    if (slot < 0 || slot >= USDL2_TIMER_MAX) {
        return mp_const_none;
    }
    usdl2_timer_entry_t *entry = usdl2_timer_by_slot[slot];
    if (entry == NULL) {
        return mp_const_none;
    }
    mp_obj_t args[2] = {
        mp_obj_new_int_from_uint(entry->last_interval),
        entry->user_param,
    };
    mp_call_function_n_kw(entry->callback, 2, 0, args);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_timer_dispatch_obj, usdl2_timer_dispatch);

static Uint32 SDLCALL usdl2_timer_trampoline(Uint32 interval, void *param) {
    usdl2_timer_entry_t *entry = (usdl2_timer_entry_t *)param;
    if (entry == NULL) {
        return 0;
    }

    entry->last_interval = interval;

    #if MICROPY_ENABLE_SCHEDULER
    if (!mp_sched_schedule(
        MP_OBJ_FROM_PTR(&usdl2_timer_dispatch_obj),
        MP_OBJ_NEW_SMALL_INT(entry->slot))) {
        mp_printf(MICROPY_ERROR_PRINTER, "SDL timer schedule queue full\n");
    }
    #else
    (void)interval;
    #endif

    return entry->interval;
}

mp_obj_t usdl2_timer_callback(mp_obj_t callback_in) {
    if (!mp_obj_is_callable(callback_in)) {
        mp_raise_TypeError(MP_ERROR_TEXT("callback must be callable"));
    }
    usdl2_timer_cb_obj_t *self = mp_obj_malloc(usdl2_timer_cb_obj_t, &usdl2_timer_cb_type);
    self->callback = callback_in;
    return MP_OBJ_FROM_PTR(self);
}

mp_obj_t usdl2_add_timer(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    if (!mp_obj_is_type(args[1], &usdl2_timer_cb_type)) {
        mp_raise_TypeError(MP_ERROR_TEXT("callback must be from SDL_TimerCallback()"));
    }

    usdl2_timer_cb_obj_t *cb = MP_OBJ_TO_PTR(args[1]);
    mp_obj_t user_param = mp_const_none;
    if (!usdl2_obj_is_none_or_null(args[2])) {
        user_param = args[2];
    }

    usdl2_timer_entry_t *entry = m_new(usdl2_timer_entry_t, 1);
    entry->next = usdl2_timer_list;
    entry->callback = cb->callback;
    entry->user_param = user_param;
    entry->interval = (Uint32)mp_obj_get_int(args[0]);
    entry->last_interval = entry->interval;
    entry->id = 0;

    if (usdl2_timer_alloc_slot(entry) < 0) {
        m_del(usdl2_timer_entry_t, entry, 1);
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("too many SDL timers"));
    }

    entry->id = SDL_AddTimer(entry->interval, usdl2_timer_trampoline, entry);
    if (entry->id == 0) {
        usdl2_timer_free_slot(entry);
        m_del(usdl2_timer_entry_t, entry, 1);
        return mp_obj_new_int(0);
    }

    usdl2_timer_list = entry;
    return usdl2_ptr_obj((void *)(uintptr_t)entry->id);
}

mp_obj_t usdl2_remove_timer(mp_obj_t timer_in) {
    SDL_TimerID id = (SDL_TimerID)(uintptr_t)usdl2_ptr_from_obj(timer_in);
    usdl2_timer_entry_t *entry = usdl2_timer_find(id);
    if (entry == NULL) {
        return mp_obj_new_int(0);
    }

    SDL_bool removed = SDL_RemoveTimer(id);
    if (removed) {
        usdl2_timer_unlink(entry);
    }
    return mp_obj_new_int(removed ? 1 : 0);
}

MP_DEFINE_CONST_OBJ_TYPE(
    usdl2_timer_cb_type,
    MP_QSTR_usdl2_timer_cb,
    MP_TYPE_FLAG_NONE
    );

MP_REGISTER_ROOT_POINTER(struct usdl2_timer_entry *usdl2_timer_list);

// --- Module registration -----------------------------------------

//| """Desktop SDL2 subset for pydisplay (linked against libSDL2)."""

static mp_obj_t SDL_Init_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_init(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_Init_fun_obj, 1, 1, SDL_Init_obj);

static mp_obj_t SDL_InitSubSystem_obj(mp_obj_t flags_in) {
    return usdl2_init_subsystem(flags_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_InitSubSystem_fun_obj, SDL_InitSubSystem_obj);

static mp_obj_t SDL_Quit_obj(void) {
    return usdl2_quit();
}
static MP_DEFINE_CONST_FUN_OBJ_0(SDL_Quit_fun_obj, SDL_Quit_obj);

static mp_obj_t process_exit_obj(mp_obj_t code_in) {
    return usdl2_process_exit(code_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(process_exit_fun_obj, process_exit_obj);

static mp_obj_t SDL_GetError_obj(void) {
    return usdl2_get_error();
}
static MP_DEFINE_CONST_FUN_OBJ_0(SDL_GetError_fun_obj, SDL_GetError_obj);

static mp_obj_t SDL_CreateWindow_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_create_window(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_CreateWindow_fun_obj, 6, 6, SDL_CreateWindow_obj);

static mp_obj_t SDL_DestroyWindow_obj(mp_obj_t win_in) {
    return usdl2_destroy_window(win_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_DestroyWindow_fun_obj, SDL_DestroyWindow_obj);

static mp_obj_t SDL_SetWindowSize_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_set_window_size(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_SetWindowSize_fun_obj, 3, 3, SDL_SetWindowSize_obj);

static mp_obj_t SDL_CreateRenderer_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_create_renderer(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_CreateRenderer_fun_obj, 3, 3, SDL_CreateRenderer_obj);

static mp_obj_t SDL_DestroyRenderer_obj(mp_obj_t renderer_in) {
    return usdl2_destroy_renderer(renderer_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_DestroyRenderer_fun_obj, SDL_DestroyRenderer_obj);

static mp_obj_t SDL_SetRenderDrawColor_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_set_render_draw_color(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_SetRenderDrawColor_fun_obj, 5, 5, SDL_SetRenderDrawColor_obj);

static mp_obj_t SDL_SetRenderTarget_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_set_render_target(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_SetRenderTarget_fun_obj, 2, 2, SDL_SetRenderTarget_obj);

static mp_obj_t SDL_RenderClear_obj(mp_obj_t renderer_in) {
    return usdl2_render_clear(renderer_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_RenderClear_fun_obj, SDL_RenderClear_obj);

static mp_obj_t SDL_RenderCopy_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_render_copy(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_RenderCopy_fun_obj, 4, 4, SDL_RenderCopy_obj);

static mp_obj_t SDL_RenderCopyEx_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_render_copy_ex(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_RenderCopyEx_fun_obj, 7, 7, SDL_RenderCopyEx_obj);

static mp_obj_t SDL_RenderPresent_obj(mp_obj_t renderer_in) {
    return usdl2_render_present(renderer_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_RenderPresent_fun_obj, SDL_RenderPresent_obj);

static mp_obj_t SDL_RenderFillRect_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_render_fill_rect(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_RenderFillRect_fun_obj, 2, 2, SDL_RenderFillRect_obj);

static mp_obj_t SDL_RenderSetLogicalSize_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_render_set_logical_size(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_RenderSetLogicalSize_fun_obj, 3, 3, SDL_RenderSetLogicalSize_obj);

static mp_obj_t SDL_CreateTexture_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_create_texture(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_CreateTexture_fun_obj, 5, 5, SDL_CreateTexture_obj);

static mp_obj_t SDL_DestroyTexture_obj(mp_obj_t texture_in) {
    return usdl2_destroy_texture(texture_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_DestroyTexture_fun_obj, SDL_DestroyTexture_obj);

static mp_obj_t SDL_SetTextureBlendMode_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_set_texture_blend_mode(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_SetTextureBlendMode_fun_obj, 2, 2, SDL_SetTextureBlendMode_obj);

static mp_obj_t SDL_UpdateTexture_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_update_texture(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_UpdateTexture_fun_obj, 4, 4, SDL_UpdateTexture_obj);

static mp_obj_t SDL_PollEvent_obj(mp_obj_t event_in) {
    return usdl2_poll_event(event_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_PollEvent_fun_obj, SDL_PollEvent_obj);

static mp_obj_t SDL_GetKeyName_obj(mp_obj_t sym_in) {
    return usdl2_get_key_name(sym_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_GetKeyName_fun_obj, SDL_GetKeyName_obj);

static mp_obj_t SDL_NumJoysticks_obj(void) {
    return usdl2_num_joysticks();
}
static MP_DEFINE_CONST_FUN_OBJ_0(SDL_NumJoysticks_fun_obj, SDL_NumJoysticks_obj);

static mp_obj_t SDL_JoystickOpen_obj(mp_obj_t index_in) {
    return usdl2_joystick_open(index_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_JoystickOpen_fun_obj, SDL_JoystickOpen_obj);

static mp_obj_t SDL_JoystickClose_obj(mp_obj_t joystick_in) {
    return usdl2_joystick_close(joystick_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_JoystickClose_fun_obj, SDL_JoystickClose_obj);

static mp_obj_t SDL_JoystickInstanceID_obj(mp_obj_t joystick_in) {
    return usdl2_joystick_instance_id(joystick_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_JoystickInstanceID_fun_obj, SDL_JoystickInstanceID_obj);

static mp_obj_t SDL_Rect_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_rect_helper(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_Rect_fun_obj, 0, 4, SDL_Rect_obj);

static mp_obj_t SDL_Point_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_point_helper(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_Point_fun_obj, 0, 2, SDL_Point_obj);

static mp_obj_t SDL_Event_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_event_helper(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_Event_fun_obj, 0, 1, SDL_Event_obj);

static mp_obj_t SDL_TimerCallback_obj(mp_obj_t callback_in) {
    return usdl2_timer_callback(callback_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_TimerCallback_fun_obj, SDL_TimerCallback_obj);

static mp_obj_t SDL_AddTimer_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_add_timer(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(SDL_AddTimer_fun_obj, 3, 3, SDL_AddTimer_obj);

static mp_obj_t SDL_RemoveTimer_obj(mp_obj_t timer_in) {
    return usdl2_remove_timer(timer_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(SDL_RemoveTimer_fun_obj, SDL_RemoveTimer_obj);

static const mp_rom_map_elem_t usdl2_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_usdl2) },
    { MP_ROM_QSTR(MP_QSTR_SDL_Init), MP_ROM_PTR(&SDL_Init_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_InitSubSystem), MP_ROM_PTR(&SDL_InitSubSystem_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_Quit), MP_ROM_PTR(&SDL_Quit_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_process_exit), MP_ROM_PTR(&process_exit_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_GetError), MP_ROM_PTR(&SDL_GetError_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_CreateWindow), MP_ROM_PTR(&SDL_CreateWindow_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_DestroyWindow), MP_ROM_PTR(&SDL_DestroyWindow_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_SetWindowSize), MP_ROM_PTR(&SDL_SetWindowSize_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_CreateRenderer), MP_ROM_PTR(&SDL_CreateRenderer_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_DestroyRenderer), MP_ROM_PTR(&SDL_DestroyRenderer_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_SetRenderDrawColor), MP_ROM_PTR(&SDL_SetRenderDrawColor_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_SetRenderTarget), MP_ROM_PTR(&SDL_SetRenderTarget_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_RenderClear), MP_ROM_PTR(&SDL_RenderClear_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_RenderCopy), MP_ROM_PTR(&SDL_RenderCopy_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_RenderCopyEx), MP_ROM_PTR(&SDL_RenderCopyEx_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_RenderPresent), MP_ROM_PTR(&SDL_RenderPresent_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_RenderFillRect), MP_ROM_PTR(&SDL_RenderFillRect_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_RenderSetLogicalSize), MP_ROM_PTR(&SDL_RenderSetLogicalSize_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_CreateTexture), MP_ROM_PTR(&SDL_CreateTexture_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_DestroyTexture), MP_ROM_PTR(&SDL_DestroyTexture_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_SetTextureBlendMode), MP_ROM_PTR(&SDL_SetTextureBlendMode_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_UpdateTexture), MP_ROM_PTR(&SDL_UpdateTexture_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_PollEvent), MP_ROM_PTR(&SDL_PollEvent_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_GetKeyName), MP_ROM_PTR(&SDL_GetKeyName_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_NumJoysticks), MP_ROM_PTR(&SDL_NumJoysticks_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_JoystickOpen), MP_ROM_PTR(&SDL_JoystickOpen_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_JoystickClose), MP_ROM_PTR(&SDL_JoystickClose_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_JoystickInstanceID), MP_ROM_PTR(&SDL_JoystickInstanceID_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_Rect), MP_ROM_PTR(&SDL_Rect_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_Point), MP_ROM_PTR(&SDL_Point_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_Event), MP_ROM_PTR(&SDL_Event_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_TimerCallback), MP_ROM_PTR(&SDL_TimerCallback_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_AddTimer), MP_ROM_PTR(&SDL_AddTimer_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_SDL_RemoveTimer), MP_ROM_PTR(&SDL_RemoveTimer_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_Event), MP_ROM_PTR(&usdl2_event_type) },
    { MP_ROM_QSTR(MP_QSTR_SDL_DEFINE_PIXELFORMAT), MP_ROM_PTR(&usdl2_SDL_DEFINE_PIXELFORMAT_fun_obj) },
    USDL2_CONSTANTS_TABLE
};

static MP_DEFINE_CONST_DICT(usdl2_module_globals, usdl2_module_globals_table);

const mp_obj_module_t usdl2_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&usdl2_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_usdl2, usdl2_module);

