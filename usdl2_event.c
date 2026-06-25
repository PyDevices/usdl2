// SPDX-License-Identifier: MIT

#include "usdl2.h"

#include "py/binary.h"

_Static_assert(sizeof(SDL_Event) == USDL2_EVENT_SIZE,
    "usdl2: SDL_Event size changed; update USDL2_EVENT_SIZE and event layout");
#include "py/runtime.h"
#include <string.h>

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
