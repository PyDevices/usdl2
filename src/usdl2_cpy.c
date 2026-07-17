/*
 * usdl2 — CPython native extension module.
 *
 * Full-parity binding surface matching the MicroPython/CircuitPython native
 * module (usdl2_mp.c / usdl2_module_globals.inc). Opaque SDL handles
 * (windows, renderers, textures, joysticks, timers) are plain Python ints,
 * matching the MicroPython binding's mp_obj_new_int_from_uint() convention.
 *
 * This file is standalone: it does not include usdl2.h/usdl2_mp.c (those
 * depend on mp_obj_t / the MicroPython runtime) and links directly against
 * libSDL2. When the native module is unavailable, pydisplay falls back to
 * src/add_ons/usdl2.py.
 *
 * SPDX-License-Identifier: MIT
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <SDL.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define USDL2_EVENT_SIZE 56
#define USDL2_TIMER_MAX 8

static_assert(sizeof(SDL_Event) == USDL2_EVENT_SIZE,
    "usdl2: SDL_Event size changed; update USDL2_EVENT_SIZE and event layout");

/* ------------------------------------------------------------------------- */
/* Pointer / rect / buffer helpers                                          */
/* ------------------------------------------------------------------------- */

/* PyArg_ParseTuple "O&" converter: None or any int -> void* (0/None -> NULL).
 * Mirrors usdl2_ptr_from_obj()/usdl2_obj_is_none_or_null() in usdl2.c: opaque
 * handles are plain ints, and None or integer 0 both mean "no handle". */
static int ptr_converter(PyObject *obj, void *out) {
    void **result = (void **)out;
    if (obj == Py_None) {
        *result = NULL;
        return 1;
    }
    PyObject *index = PyNumber_Index(obj);
    if (index == NULL) {
        return 0;
    }
    unsigned long long v = PyLong_AsUnsignedLongLong(index);
    Py_DECREF(index);
    if (v == (unsigned long long)-1 && PyErr_Occurred()) {
        return 0;
    }
    *result = (void *)(uintptr_t)v;
    return 1;
}

static PyObject *ptr_to_obj(void *ptr) {
    return PyLong_FromUnsignedLongLong((unsigned long long)(uintptr_t)ptr);
}

/* obj is None, or an int whose value is falsy (0). Used for rect/point/center
 * "optional" arguments, matching usdl2_obj_is_none_or_null() in usdl2.c. */
static int obj_is_none_or_null(PyObject *obj) {
    if (obj == Py_None) {
        return 1;
    }
    if (PyLong_Check(obj)) {
        int truth = PyObject_IsTrue(obj);
        if (truth < 0) {
            PyErr_Clear();
            return 0;
        }
        return truth == 0;
    }
    return 0;
}

static int rect_from_obj(PyObject *obj, SDL_Rect *out) {
    Py_buffer view;
    if (PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE) < 0) {
        return -1;
    }
    if (view.len < (Py_ssize_t)sizeof(int32_t) * 4) {
        PyBuffer_Release(&view);
        PyErr_SetString(PyExc_ValueError, "rect buffer too small");
        return -1;
    }
    int32_t values[4];
    memcpy(values, view.buf, sizeof(values));
    PyBuffer_Release(&view);
    out->x = values[0];
    out->y = values[1];
    out->w = values[2];
    out->h = values[3];
    return 0;
}

static int rect_to_obj(const SDL_Rect *rect, PyObject *obj) {
    Py_buffer view;
    if (PyObject_GetBuffer(obj, &view, PyBUF_WRITABLE) < 0) {
        return -1;
    }
    if (view.len < (Py_ssize_t)sizeof(int32_t) * 4) {
        PyBuffer_Release(&view);
        PyErr_SetString(PyExc_ValueError, "rect buffer too small");
        return -1;
    }
    int32_t values[4] = { rect->x, rect->y, rect->w, rect->h };
    memcpy(view.buf, values, sizeof(values));
    PyBuffer_Release(&view);
    return 0;
}

/* Optional SDL_Rect* argument: None/0 -> *out = NULL; bytes-like -> filled
 * into *storage and *out = storage. Returns -1 with an exception set on a
 * buffer error. Mirrors usdl2_optional_rect() in usdl2.c. */
static int optional_rect(PyObject *obj, SDL_Rect *storage, SDL_Rect **out) {
    if (obj_is_none_or_null(obj)) {
        *out = NULL;
        return 0;
    }
    if (rect_from_obj(obj, storage) < 0) {
        return -1;
    }
    *out = storage;
    return 0;
}

static int display_mode_to_obj(const SDL_DisplayMode *mode, PyObject *obj) {
    Py_buffer view;
    if (PyObject_GetBuffer(obj, &view, PyBUF_WRITABLE) < 0) {
        return -1;
    }
    if (view.len < (Py_ssize_t)sizeof(int32_t) * 4) {
        PyBuffer_Release(&view);
        PyErr_SetString(PyExc_ValueError, "display mode buffer too small");
        return -1;
    }
    uint32_t format = (uint32_t)mode->format;
    int32_t rest[3] = { mode->w, mode->h, mode->refresh_rate };
    memcpy(view.buf, &format, sizeof(format));
    memcpy((char *)view.buf + sizeof(format), rest, sizeof(rest));
    PyBuffer_Release(&view);
    return 0;
}

/* Window title: accept str directly, or decode a bytes-like buffer as UTF-8
 * (mirrors usdl2_window_title() in usdl2.c). *tmp_out receives an owned
 * reference that must stay alive (and be released) while the returned
 * pointer is used; it is only set for the buffer-decoding path. */
static const char *title_to_cstring(PyObject *title, PyObject **tmp_out) {
    *tmp_out = NULL;
    if (PyUnicode_Check(title)) {
        return PyUnicode_AsUTF8(title);
    }
    Py_buffer view;
    if (PyObject_GetBuffer(title, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }
    PyObject *s = PyUnicode_FromStringAndSize((const char *)view.buf, view.len);
    PyBuffer_Release(&view);
    if (s == NULL) {
        return NULL;
    }
    const char *cstr = PyUnicode_AsUTF8(s);
    if (cstr == NULL) {
        Py_DECREF(s);
        return NULL;
    }
    *tmp_out = s;
    return cstr;
}

/* ------------------------------------------------------------------------- */
/* Event / subview types                                                    */
/* ------------------------------------------------------------------------- */

typedef struct {
    PyObject_HEAD
    uint8_t data[USDL2_EVENT_SIZE];
} UsdlEventObject;

typedef struct {
    PyObject_HEAD
    PyObject *parent; /* strong ref to the owning UsdlEventObject */
    uint16_t base;
} UsdlSubviewObject;

static PyTypeObject UsdlEventType;
static PyTypeObject UsdlSubviewType;

static uint32_t read_u32(const uint8_t *data, uint16_t off) {
    uint32_t v;
    memcpy(&v, data + off, sizeof(v));
    return v;
}

static int32_t read_i32(const uint8_t *data, uint16_t off) {
    int32_t v;
    memcpy(&v, data + off, sizeof(v));
    return v;
}

static int16_t read_i16(const uint8_t *data, uint16_t off) {
    int16_t v;
    memcpy(&v, data + off, sizeof(v));
    return v;
}

static int is_joystick_event(uint32_t type) {
    return type >= SDL_JOYAXISMOTION && type <= SDL_JOYDEVICEREMOVED;
}

static PyObject *subview_new(PyObject *parent, uint16_t base) {
    UsdlSubviewObject *self = PyObject_New(UsdlSubviewObject, &UsdlSubviewType);
    if (self == NULL) {
        return NULL;
    }
    Py_INCREF(parent);
    self->parent = parent;
    self->base = base;
    return (PyObject *)self;
}

static void subview_dealloc(UsdlSubviewObject *self) {
    Py_CLEAR(self->parent);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static const uint8_t *subview_data(UsdlSubviewObject *self) {
    return ((UsdlEventObject *)self->parent)->data;
}

static PyObject *subview_get_windowID(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromUnsignedLong(read_u32(subview_data(self), self->base + 0));
}

static PyObject *subview_get_which(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    const uint8_t *data = subview_data(self);
    if (is_joystick_event(read_u32(data, 0))) {
        return PyLong_FromLong(read_i32(data, self->base + 0));
    }
    return PyLong_FromUnsignedLong(read_u32(data, self->base + 4));
}

static PyObject *subview_get_state(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    const uint8_t *data = subview_data(self);
    uint32_t type = read_u32(data, 0);
    if (self->base == 8 && type == SDL_MOUSEMOTION) {
        return PyLong_FromUnsignedLong(read_u32(data, self->base + 8));
    }
    if (type == SDL_JOYBUTTONDOWN || type == SDL_JOYBUTTONUP) {
        return PyLong_FromLong(data[self->base + 5]);
    }
    return PyLong_FromUnsignedLong(data[self->base + 4]);
}

static PyObject *subview_get_x(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(read_i32(subview_data(self), self->base + 12));
}

static PyObject *subview_get_y(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(read_i32(subview_data(self), self->base + 16));
}

static PyObject *subview_get_xrel(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    const uint8_t *data = subview_data(self);
    if (read_u32(data, 0) == SDL_JOYBALLMOTION) {
        return PyLong_FromLong(read_i16(data, self->base + 8));
    }
    return PyLong_FromLong(read_i32(data, self->base + 20));
}

static PyObject *subview_get_yrel(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    const uint8_t *data = subview_data(self);
    if (read_u32(data, 0) == SDL_JOYBALLMOTION) {
        return PyLong_FromLong(read_i16(data, self->base + 10));
    }
    return PyLong_FromLong(read_i32(data, self->base + 24));
}

static PyObject *subview_get_button(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    const uint8_t *data = subview_data(self);
    uint32_t type = read_u32(data, 0);
    if (type == SDL_JOYBUTTONDOWN || type == SDL_JOYBUTTONUP) {
        return PyLong_FromLong(data[self->base + 4]);
    }
    return PyLong_FromLong(data[self->base + 8]);
}

static PyObject *subview_get_clicks(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(subview_data(self)[self->base + 10]);
}

static PyObject *subview_get_direction(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromUnsignedLong(read_u32(subview_data(self), self->base + 16));
}

static PyObject *subview_get_preciseX(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    float value;
    memcpy(&value, subview_data(self) + self->base + 0, sizeof(value));
    return PyFloat_FromDouble((double)value);
}

static PyObject *subview_get_preciseY(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    float value;
    memcpy(&value, subview_data(self) + self->base + 4, sizeof(value));
    return PyFloat_FromDouble((double)value);
}

static PyObject *subview_get_repeat(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(subview_data(self)[self->base + 5]);
}

static PyObject *subview_get_keysym(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return subview_new(self->parent, self->base + 8);
}

static PyObject *subview_get_scancode(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(read_i32(subview_data(self), self->base + 0));
}

static PyObject *subview_get_sym(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(read_i32(subview_data(self), self->base + 4));
}

static PyObject *subview_get_mod(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromUnsignedLong(read_u32(subview_data(self), self->base + 8) & 0xffffu);
}

static PyObject *subview_get_axis(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(subview_data(self)[self->base + 4]);
}

static PyObject *subview_get_value(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    const uint8_t *data = subview_data(self);
    uint32_t type = read_u32(data, 0);
    if (type == SDL_JOYAXISMOTION) {
        return PyLong_FromLong(read_i16(data, self->base + 8));
    }
    if (type == SDL_JOYHATMOTION) {
        return PyLong_FromLong(data[self->base + 5]);
    }
    PyErr_SetString(PyExc_AttributeError, "value");
    return NULL;
}

static PyObject *subview_get_ball(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(subview_data(self)[self->base + 4]);
}

static PyObject *subview_get_hat(UsdlSubviewObject *self, void *closure) {
    (void)closure;
    return PyLong_FromLong(subview_data(self)[self->base + 4]);
}

static PyGetSetDef subview_getset[] = {
    {"windowID", (getter)subview_get_windowID, NULL, NULL, NULL},
    {"which", (getter)subview_get_which, NULL, NULL, NULL},
    {"state", (getter)subview_get_state, NULL, NULL, NULL},
    {"x", (getter)subview_get_x, NULL, NULL, NULL},
    {"y", (getter)subview_get_y, NULL, NULL, NULL},
    {"xrel", (getter)subview_get_xrel, NULL, NULL, NULL},
    {"yrel", (getter)subview_get_yrel, NULL, NULL, NULL},
    {"button", (getter)subview_get_button, NULL, NULL, NULL},
    {"clicks", (getter)subview_get_clicks, NULL, NULL, NULL},
    {"direction", (getter)subview_get_direction, NULL, NULL, NULL},
    {"preciseX", (getter)subview_get_preciseX, NULL, NULL, NULL},
    {"preciseY", (getter)subview_get_preciseY, NULL, NULL, NULL},
    {"repeat", (getter)subview_get_repeat, NULL, NULL, NULL},
    {"keysym", (getter)subview_get_keysym, NULL, NULL, NULL},
    {"scancode", (getter)subview_get_scancode, NULL, NULL, NULL},
    {"sym", (getter)subview_get_sym, NULL, NULL, NULL},
    {"mod", (getter)subview_get_mod, NULL, NULL, NULL},
    {"axis", (getter)subview_get_axis, NULL, NULL, NULL},
    {"value", (getter)subview_get_value, NULL, NULL, NULL},
    {"ball", (getter)subview_get_ball, NULL, NULL, NULL},
    {"hat", (getter)subview_get_hat, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL, NULL},
};

static PyTypeObject UsdlSubviewType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "usdl2._SubView",
    .tp_basicsize = sizeof(UsdlSubviewObject),
    .tp_dealloc = (destructor)subview_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_getset = subview_getset,
};

/* Event: 56-byte SDL_Event buffer with pydisplay-compatible subviews. All of
 * motion/key/button/wheel/jaxis/jball/jhat/jbutton view the union at offset 8
 * (immediately after type+timestamp), matching real SDL_Event layout and
 * usdl2_event_attr()/usdl2_subview_attr() in usdl2.c. */

static PyObject *event_construct(PyObject *arg) {
    UsdlEventObject *self = (UsdlEventObject *)UsdlEventType.tp_alloc(&UsdlEventType, 0);
    if (self == NULL) {
        return NULL;
    }
    memset(self->data, 0, USDL2_EVENT_SIZE);
    if (arg != NULL) {
        Py_buffer view;
        if (PyObject_GetBuffer(arg, &view, PyBUF_SIMPLE) < 0) {
            Py_DECREF(self);
            return NULL;
        }
        if (view.len < USDL2_EVENT_SIZE) {
            PyBuffer_Release(&view);
            Py_DECREF(self);
            PyErr_SetString(PyExc_ValueError, "event buffer too small");
            return NULL;
        }
        memcpy(self->data, view.buf, USDL2_EVENT_SIZE);
        PyBuffer_Release(&view);
    }
    return (PyObject *)self;
}

static PyObject *event_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    (void)type;
    (void)kwds;
    PyObject *arg = NULL;
    if (!PyArg_ParseTuple(args, "|O", &arg)) {
        return NULL;
    }
    // SDL_Event(None) or SDL_Event() -> new zero-filled event.
    // SDL_Event(<existing SDL_Event>) -> the same instance (identity).
    // SDL_Event(<bytes-like>) -> a new event copied from the buffer.
    if (arg == NULL || arg == Py_None) {
        return event_construct(NULL);
    }
    if (PyObject_TypeCheck(arg, &UsdlEventType)) {
        Py_INCREF(arg);
        return arg;
    }
    return event_construct(arg);
}

static void event_dealloc(UsdlEventObject *self) {
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static Py_ssize_t event_length(UsdlEventObject *self) {
    (void)self;
    return USDL2_EVENT_SIZE;
}

static PySequenceMethods event_as_sequence = {
    .sq_length = (lenfunc)event_length,
};

static int event_getbuffer(UsdlEventObject *self, Py_buffer *view, int flags) {
    return PyBuffer_FillInfo(view, (PyObject *)self, self->data, USDL2_EVENT_SIZE, 0, flags);
}

static PyBufferProcs event_as_buffer = {
    .bf_getbuffer = (getbufferproc)event_getbuffer,
    .bf_releasebuffer = NULL,
};

static PyObject *event_get_type(UsdlEventObject *self, void *closure) {
    (void)closure;
    return PyLong_FromUnsignedLong(read_u32(self->data, 0));
}

static int event_set_type(UsdlEventObject *self, PyObject *value, void *closure) {
    (void)closure;
    if (value == NULL) {
        PyErr_SetString(PyExc_AttributeError, "cannot delete type");
        return -1;
    }
    unsigned long v = PyLong_AsUnsignedLong(value);
    if (v == (unsigned long)-1 && PyErr_Occurred()) {
        return -1;
    }
    uint32_t u = (uint32_t)v;
    memcpy(self->data, &u, sizeof(u));
    return 0;
}

static PyObject *event_get_timestamp(UsdlEventObject *self, void *closure) {
    (void)closure;
    return PyLong_FromUnsignedLong(read_u32(self->data, 4));
}

static PyObject *event_get_subview8(UsdlEventObject *self, void *closure) {
    (void)closure;
    return subview_new((PyObject *)self, 8);
}

static PyGetSetDef event_getset[] = {
    {"type", (getter)event_get_type, (setter)event_set_type, NULL, NULL},
    {"timestamp", (getter)event_get_timestamp, NULL, NULL, NULL},
    {"motion", (getter)event_get_subview8, NULL, NULL, NULL},
    {"key", (getter)event_get_subview8, NULL, NULL, NULL},
    {"button", (getter)event_get_subview8, NULL, NULL, NULL},
    {"wheel", (getter)event_get_subview8, NULL, NULL, NULL},
    {"jaxis", (getter)event_get_subview8, NULL, NULL, NULL},
    {"jball", (getter)event_get_subview8, NULL, NULL, NULL},
    {"jhat", (getter)event_get_subview8, NULL, NULL, NULL},
    {"jbutton", (getter)event_get_subview8, NULL, NULL, NULL},
    {NULL, NULL, NULL, NULL, NULL},
};

static PyTypeObject UsdlEventType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "usdl2.SDL_Event",
    .tp_basicsize = sizeof(UsdlEventObject),
    .tp_dealloc = (destructor)event_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_getset = event_getset,
    .tp_as_sequence = &event_as_sequence,
    .tp_as_buffer = &event_as_buffer,
    .tp_new = event_new,
};

/* ------------------------------------------------------------------------- */
/* Timer callback wrapper + SDL timer machinery                            */
/* ------------------------------------------------------------------------- */

/* Not exported at module scope (mirrors usdl2_timer_cb_type in usdl2.c, which
 * is only reachable via the SDL_TimerCallback() factory function). */
typedef struct {
    PyObject_HEAD
    PyObject *callback;
} UsdlTimerCbObject;

static PyTypeObject UsdlTimerCbType;

static void timer_cb_dealloc(UsdlTimerCbObject *self) {
    Py_CLEAR(self->callback);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyTypeObject UsdlTimerCbType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "usdl2._TimerCallback",
    .tp_basicsize = sizeof(UsdlTimerCbObject),
    .tp_dealloc = (destructor)timer_cb_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
};

/* SDL timer callbacks fire on an SDL-owned thread, not the Python thread that
 * called SDL_AddTimer(). We look the entry up through a slot table (not a raw
 * pointer) under a mutex, matching the ctypes reference's slot-indexed dict,
 * so a concurrent SDL_RemoveTimer() cannot dereference freed memory: it marks
 * the slot NULL and the entry "removed" before releasing the mutex, and the
 * entry is only freed once no trampoline invocation is still using it. */
typedef struct usdl2_timer_entry {
    struct usdl2_timer_entry *next;
    SDL_TimerID id;
    PyObject *callback;
    PyObject *user_param;
    Uint32 interval;
    int slot;
    int busy;
    int removed;
} usdl2_timer_entry_t;

static SDL_mutex *g_timer_mutex = NULL;
static usdl2_timer_entry_t *g_timer_list = NULL;
static usdl2_timer_entry_t *g_timer_slots[USDL2_TIMER_MAX];

/* Caller must hold the GIL (for the Py_DECREFs); entry must already be
 * unlinked from g_timer_list/g_timer_slots and have busy == 0. */
static void timer_entry_release(usdl2_timer_entry_t *entry) {
    Py_XDECREF(entry->callback);
    Py_XDECREF(entry->user_param);
    free(entry);
}

static Uint32 SDLCALL timer_trampoline(Uint32 interval, void *param) {
    intptr_t slot = (intptr_t)param;

    SDL_LockMutex(g_timer_mutex);
    usdl2_timer_entry_t *entry =
        (slot >= 0 && slot < USDL2_TIMER_MAX) ? g_timer_slots[slot] : NULL;
    if (entry == NULL) {
        SDL_UnlockMutex(g_timer_mutex);
        return 0;
    }
    entry->busy++;
    Uint32 ret_interval = entry->interval;
    PyObject *callback = entry->callback;
    PyObject *user_param = entry->user_param;
    SDL_UnlockMutex(g_timer_mutex);

    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject *args = Py_BuildValue("kO", (unsigned long)interval, user_param);
    if (args != NULL) {
        PyObject *result = PyObject_CallObject(callback, args);
        Py_DECREF(args);
        if (result == NULL) {
            PyErr_Print();
        } else {
            Py_DECREF(result);
        }
    } else {
        PyErr_Clear();
    }
    PyGILState_Release(gstate);

    int should_free = 0;
    SDL_LockMutex(g_timer_mutex);
    entry->busy--;
    if (entry->removed && entry->busy == 0) {
        should_free = 1;
    }
    SDL_UnlockMutex(g_timer_mutex);

    if (should_free) {
        PyGILState_STATE gstate2 = PyGILState_Ensure();
        timer_entry_release(entry);
        PyGILState_Release(gstate2);
    }

    /* The next interval is always the one the timer was created with (not
     * the callback's return value) -- matches usdl2_timer_trampoline() in
     * usdl2.c and the ctypes _timer_trampoline() in _api.py. */
    return ret_interval;
}

static PyObject *py_SDL_TimerCallback(PyObject *module, PyObject *callback) {
    (void)module;
    if (!PyCallable_Check(callback)) {
        PyErr_SetString(PyExc_TypeError, "callback must be callable");
        return NULL;
    }
    UsdlTimerCbObject *self = PyObject_New(UsdlTimerCbObject, &UsdlTimerCbType);
    if (self == NULL) {
        return NULL;
    }
    Py_INCREF(callback);
    self->callback = callback;
    return (PyObject *)self;
}

static PyObject *py_SDL_AddTimer(PyObject *module, PyObject *args) {
    (void)module;
    unsigned long interval_arg;
    PyObject *cb_obj;
    PyObject *user_param;
    if (!PyArg_ParseTuple(args, "kOO", &interval_arg, &cb_obj, &user_param)) {
        return NULL;
    }
    if (!PyObject_TypeCheck(cb_obj, &UsdlTimerCbType)) {
        PyErr_SetString(PyExc_TypeError, "callback must be from SDL_TimerCallback()");
        return NULL;
    }
    if (g_timer_mutex == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "usdl2 timer subsystem not initialized");
        return NULL;
    }

    SDL_LockMutex(g_timer_mutex);
    int slot = -1;
    for (int i = 0; i < USDL2_TIMER_MAX; i++) {
        if (g_timer_slots[i] == NULL) {
            slot = i;
            break;
        }
    }
    SDL_UnlockMutex(g_timer_mutex);
    if (slot < 0) {
        PyErr_SetString(PyExc_RuntimeError, "too many SDL timers");
        return NULL;
    }

    usdl2_timer_entry_t *entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    entry->callback = ((UsdlTimerCbObject *)cb_obj)->callback;
    Py_INCREF(entry->callback);
    entry->user_param = user_param;
    Py_INCREF(entry->user_param);
    entry->interval = (Uint32)interval_arg;
    entry->slot = slot;

    SDL_LockMutex(g_timer_mutex);
    g_timer_slots[slot] = entry;
    entry->next = g_timer_list;
    g_timer_list = entry;
    SDL_UnlockMutex(g_timer_mutex);

    SDL_TimerID id = SDL_AddTimer(entry->interval, timer_trampoline, (void *)(intptr_t)slot);
    if (id == 0) {
        SDL_LockMutex(g_timer_mutex);
        g_timer_slots[slot] = NULL;
        g_timer_list = entry->next;
        SDL_UnlockMutex(g_timer_mutex);
        timer_entry_release(entry);
        return PyLong_FromLong(0);
    }
    entry->id = id;
    return PyLong_FromLong((long)id);
}

static PyObject *py_SDL_RemoveTimer(PyObject *module, PyObject *timer_obj) {
    (void)module;
    void *ptr;
    if (!ptr_converter(timer_obj, &ptr)) {
        return NULL;
    }
    SDL_TimerID id = (SDL_TimerID)(intptr_t)ptr;

    SDL_LockMutex(g_timer_mutex);
    usdl2_timer_entry_t **prev = &g_timer_list;
    usdl2_timer_entry_t *entry = NULL;
    while (*prev != NULL) {
        if ((*prev)->id == id) {
            entry = *prev;
            *prev = entry->next;
            break;
        }
        prev = &(*prev)->next;
    }
    if (entry != NULL) {
        g_timer_slots[entry->slot] = NULL;
        entry->removed = 1;
    }
    SDL_UnlockMutex(g_timer_mutex);

    if (entry == NULL) {
        return PyLong_FromLong(0);
    }

    SDL_bool removed = SDL_RemoveTimer(id);

    int should_free = 0;
    SDL_LockMutex(g_timer_mutex);
    if (entry->busy == 0) {
        should_free = 1;
    }
    SDL_UnlockMutex(g_timer_mutex);
    if (should_free) {
        timer_entry_release(entry);
    }

    return PyLong_FromLong(removed ? 1 : 0);
}

/* ------------------------------------------------------------------------- */
/* Module-level SDL function bindings                                       */
/* ------------------------------------------------------------------------- */

static PyObject *py_SDL_Init(PyObject *module, PyObject *flags_obj) {
    (void)module;
    unsigned long flags = PyLong_AsUnsignedLong(flags_obj);
    if (flags == (unsigned long)-1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromLong(SDL_Init((Uint32)flags));
}

static PyObject *py_SDL_InitSubSystem(PyObject *module, PyObject *flags_obj) {
    (void)module;
    unsigned long flags = PyLong_AsUnsignedLong(flags_obj);
    if (flags == (unsigned long)-1 && PyErr_Occurred()) {
        return NULL;
    }
    return PyLong_FromLong(SDL_InitSubSystem((Uint32)flags));
}

static PyObject *py_SDL_Quit(PyObject *module, PyObject *noargs) {
    (void)module;
    (void)noargs;
    SDL_Quit();
    Py_RETURN_NONE;
}

static PyObject *py_SDL_GetError(PyObject *module, PyObject *noargs) {
    (void)module;
    (void)noargs;
    const char *err = SDL_GetError();
    return PyUnicode_FromString(err ? err : "");
}

static PyObject *py_SDL_CreateWindow(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *title_obj;
    int x, y, w, h;
    unsigned int flags;
    if (!PyArg_ParseTuple(args, "OiiiiI", &title_obj, &x, &y, &w, &h, &flags)) {
        return NULL;
    }
    PyObject *tmp = NULL;
    const char *title = title_to_cstring(title_obj, &tmp);
    if (title == NULL) {
        return NULL;
    }
    SDL_Window *win = SDL_CreateWindow(title, x, y, w, h, (Uint32)flags);
    Py_XDECREF(tmp);
    return ptr_to_obj(win);
}

static PyObject *py_SDL_DestroyWindow(PyObject *module, PyObject *win_obj) {
    (void)module;
    void *ptr;
    if (!ptr_converter(win_obj, &ptr)) {
        return NULL;
    }
    SDL_DestroyWindow((SDL_Window *)ptr);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_SetWindowSize(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *win_obj;
    int w, h;
    if (!PyArg_ParseTuple(args, "Oii", &win_obj, &w, &h)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(win_obj, &ptr)) {
        return NULL;
    }
    SDL_SetWindowSize((SDL_Window *)ptr, w, h);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_SetWindowResizable(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *win_obj;
    int resizable;
    if (!PyArg_ParseTuple(args, "Oi", &win_obj, &resizable)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(win_obj, &ptr)) {
        return NULL;
    }
    SDL_SetWindowResizable((SDL_Window *)ptr, resizable ? SDL_TRUE : SDL_FALSE);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_SetWindowMinimumSize(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *win_obj;
    int w, h;
    if (!PyArg_ParseTuple(args, "Oii", &win_obj, &w, &h)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(win_obj, &ptr)) {
        return NULL;
    }
    SDL_SetWindowMinimumSize((SDL_Window *)ptr, w, h);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_SetWindowMaximumSize(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *win_obj;
    int w, h;
    if (!PyArg_ParseTuple(args, "Oii", &win_obj, &w, &h)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(win_obj, &ptr)) {
        return NULL;
    }
    SDL_SetWindowMaximumSize((SDL_Window *)ptr, w, h);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_CreateRenderer(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *win_obj;
    int index;
    unsigned int flags;
    if (!PyArg_ParseTuple(args, "OiI", &win_obj, &index, &flags)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(win_obj, &ptr)) {
        return NULL;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer((SDL_Window *)ptr, index, (Uint32)flags);
    return ptr_to_obj(renderer);
}

static PyObject *py_SDL_DestroyRenderer(PyObject *module, PyObject *renderer_obj) {
    (void)module;
    void *ptr;
    if (!ptr_converter(renderer_obj, &ptr)) {
        return NULL;
    }
    SDL_DestroyRenderer((SDL_Renderer *)ptr);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_SetRenderDrawColor(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *renderer_obj;
    int r, g, b, a;
    if (!PyArg_ParseTuple(args, "Oiiii", &renderer_obj, &r, &g, &b, &a)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(renderer_obj, &ptr)) {
        return NULL;
    }
    int rc = SDL_SetRenderDrawColor((SDL_Renderer *)ptr, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a);
    return PyLong_FromLong(rc);
}

static PyObject *py_SDL_SetRenderTarget(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *renderer_obj, *texture_obj;
    if (!PyArg_ParseTuple(args, "OO", &renderer_obj, &texture_obj)) {
        return NULL;
    }
    void *renderer_ptr, *texture_ptr;
    if (!ptr_converter(renderer_obj, &renderer_ptr)) {
        return NULL;
    }
    if (!ptr_converter(texture_obj, &texture_ptr)) {
        return NULL;
    }
    int rc = SDL_SetRenderTarget((SDL_Renderer *)renderer_ptr, (SDL_Texture *)texture_ptr);
    return PyLong_FromLong(rc);
}

static PyObject *py_SDL_RenderClear(PyObject *module, PyObject *renderer_obj) {
    (void)module;
    void *ptr;
    if (!ptr_converter(renderer_obj, &ptr)) {
        return NULL;
    }
    SDL_RenderClear((SDL_Renderer *)ptr);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_RenderCopy(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *renderer_obj, *texture_obj, *src_obj, *dst_obj;
    if (!PyArg_ParseTuple(args, "OOOO", &renderer_obj, &texture_obj, &src_obj, &dst_obj)) {
        return NULL;
    }
    void *renderer_ptr, *texture_ptr;
    if (!ptr_converter(renderer_obj, &renderer_ptr)) {
        return NULL;
    }
    if (!ptr_converter(texture_obj, &texture_ptr)) {
        return NULL;
    }
    SDL_Rect src_storage, dst_storage;
    SDL_Rect *src, *dst;
    if (optional_rect(src_obj, &src_storage, &src) < 0) {
        return NULL;
    }
    if (optional_rect(dst_obj, &dst_storage, &dst) < 0) {
        return NULL;
    }
    SDL_RenderCopy((SDL_Renderer *)renderer_ptr, (SDL_Texture *)texture_ptr, src, dst);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_RenderCopyEx(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *renderer_obj, *texture_obj, *src_obj, *dst_obj, *center_obj;
    double angle;
    int flip;
    if (!PyArg_ParseTuple(args, "OOOOdOi", &renderer_obj, &texture_obj, &src_obj, &dst_obj,
        &angle, &center_obj, &flip)) {
        return NULL;
    }
    void *renderer_ptr, *texture_ptr;
    if (!ptr_converter(renderer_obj, &renderer_ptr)) {
        return NULL;
    }
    if (!ptr_converter(texture_obj, &texture_ptr)) {
        return NULL;
    }
    SDL_Rect src_storage, dst_storage;
    SDL_Rect *src, *dst;
    if (optional_rect(src_obj, &src_storage, &src) < 0) {
        return NULL;
    }
    if (optional_rect(dst_obj, &dst_storage, &dst) < 0) {
        return NULL;
    }
    SDL_Point center_storage;
    SDL_Point *center = NULL;
    if (!obj_is_none_or_null(center_obj)) {
        Py_buffer view;
        if (PyObject_GetBuffer(center_obj, &view, PyBUF_SIMPLE) < 0) {
            return NULL;
        }
        if (view.len < (Py_ssize_t)sizeof(int32_t) * 2) {
            PyBuffer_Release(&view);
            PyErr_SetString(PyExc_ValueError, "center buffer too small");
            return NULL;
        }
        int32_t values[2];
        memcpy(values, view.buf, sizeof(values));
        PyBuffer_Release(&view);
        center_storage.x = values[0];
        center_storage.y = values[1];
        center = &center_storage;
    }
    SDL_RenderCopyEx((SDL_Renderer *)renderer_ptr, (SDL_Texture *)texture_ptr, src, dst,
        angle, center, flip);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_RenderPresent(PyObject *module, PyObject *renderer_obj) {
    (void)module;
    void *ptr;
    if (!ptr_converter(renderer_obj, &ptr)) {
        return NULL;
    }
    SDL_RenderPresent((SDL_Renderer *)ptr);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_RenderFillRect(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *renderer_obj, *rect_obj;
    if (!PyArg_ParseTuple(args, "OO", &renderer_obj, &rect_obj)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(renderer_obj, &ptr)) {
        return NULL;
    }
    SDL_Rect rect;
    if (rect_from_obj(rect_obj, &rect) < 0) {
        return NULL;
    }
    int rc = SDL_RenderFillRect((SDL_Renderer *)ptr, &rect);
    return PyLong_FromLong(rc);
}

static PyObject *py_SDL_RenderSetLogicalSize(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *renderer_obj;
    int w, h;
    if (!PyArg_ParseTuple(args, "Oii", &renderer_obj, &w, &h)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(renderer_obj, &ptr)) {
        return NULL;
    }
    int rc = SDL_RenderSetLogicalSize((SDL_Renderer *)ptr, w, h);
    return PyLong_FromLong(rc);
}

static PyObject *py_SDL_CreateTexture(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *renderer_obj;
    unsigned int format;
    int access, w, h;
    if (!PyArg_ParseTuple(args, "OIiii", &renderer_obj, &format, &access, &w, &h)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(renderer_obj, &ptr)) {
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTexture((SDL_Renderer *)ptr, (Uint32)format, access, w, h);
    return ptr_to_obj(texture);
}

static PyObject *py_SDL_DestroyTexture(PyObject *module, PyObject *texture_obj) {
    (void)module;
    void *ptr;
    if (!ptr_converter(texture_obj, &ptr)) {
        return NULL;
    }
    SDL_DestroyTexture((SDL_Texture *)ptr);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_SetTextureBlendMode(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *texture_obj;
    int mode;
    if (!PyArg_ParseTuple(args, "Oi", &texture_obj, &mode)) {
        return NULL;
    }
    void *ptr;
    if (!ptr_converter(texture_obj, &ptr)) {
        return NULL;
    }
    int rc = SDL_SetTextureBlendMode((SDL_Texture *)ptr, (SDL_BlendMode)mode);
    return PyLong_FromLong(rc);
}

static PyObject *py_SDL_UpdateTexture(PyObject *module, PyObject *args) {
    (void)module;
    PyObject *texture_obj, *rect_obj, *pixels_obj;
    int pitch;
    if (!PyArg_ParseTuple(args, "OOOi", &texture_obj, &rect_obj, &pixels_obj, &pitch)) {
        return NULL;
    }
    void *texture_ptr;
    if (!ptr_converter(texture_obj, &texture_ptr)) {
        return NULL;
    }
    SDL_Rect rect_storage;
    SDL_Rect *rect_ptr;
    if (optional_rect(rect_obj, &rect_storage, &rect_ptr) < 0) {
        return NULL;
    }
    Py_buffer view;
    if (PyObject_GetBuffer(pixels_obj, &view, PyBUF_SIMPLE) < 0) {
        return NULL;
    }
    int rc = SDL_UpdateTexture((SDL_Texture *)texture_ptr, rect_ptr, view.buf, pitch);
    PyBuffer_Release(&view);
    return PyLong_FromLong(rc);
}

static PyObject *py_SDL_PollEvent(PyObject *module, PyObject *event_obj) {
    (void)module;
    SDL_Event event;
    int rc = SDL_PollEvent(&event);
    if (rc) {
        if (PyObject_TypeCheck(event_obj, &UsdlEventType)) {
            memcpy(((UsdlEventObject *)event_obj)->data, &event, USDL2_EVENT_SIZE);
        } else {
            Py_buffer view;
            if (PyObject_GetBuffer(event_obj, &view, PyBUF_WRITABLE) < 0) {
                return NULL;
            }
            if (view.len < USDL2_EVENT_SIZE) {
                PyBuffer_Release(&view);
                PyErr_SetString(PyExc_ValueError, "event buffer too small");
                return NULL;
            }
            memcpy(view.buf, &event, USDL2_EVENT_SIZE);
            PyBuffer_Release(&view);
        }
    }
    return PyBool_FromLong(rc);
}

static PyObject *py_SDL_GetKeyName(PyObject *module, PyObject *sym_obj) {
    (void)module;
    long sym = PyLong_AsLong(sym_obj);
    if (sym == -1 && PyErr_Occurred()) {
        return NULL;
    }
    const char *name = SDL_GetKeyName((SDL_Keycode)sym);
    return PyUnicode_FromString(name ? name : "");
}

static PyObject *py_SDL_NumJoysticks(PyObject *module, PyObject *noargs) {
    (void)module;
    (void)noargs;
    return PyLong_FromLong(SDL_NumJoysticks());
}

static PyObject *py_SDL_JoystickOpen(PyObject *module, PyObject *index_obj) {
    (void)module;
    long index = PyLong_AsLong(index_obj);
    if (index == -1 && PyErr_Occurred()) {
        return NULL;
    }
    SDL_Joystick *joystick = SDL_JoystickOpen((int)index);
    return ptr_to_obj(joystick);
}

static PyObject *py_SDL_JoystickClose(PyObject *module, PyObject *joystick_obj) {
    (void)module;
    void *ptr;
    if (!ptr_converter(joystick_obj, &ptr)) {
        return NULL;
    }
    SDL_JoystickClose((SDL_Joystick *)ptr);
    Py_RETURN_NONE;
}

static PyObject *py_SDL_JoystickInstanceID(PyObject *module, PyObject *joystick_obj) {
    (void)module;
    void *ptr;
    if (!ptr_converter(joystick_obj, &ptr)) {
        return NULL;
    }
    return PyLong_FromLong(SDL_JoystickInstanceID((SDL_Joystick *)ptr));
}

static PyObject *py_SDL_Rect(PyObject *module, PyObject *args) {
    (void)module;
    Py_ssize_t n = PyTuple_GET_SIZE(args);
    if (n > 4) {
        PyErr_SetString(PyExc_TypeError, "SDL_Rect takes at most 4 arguments");
        return NULL;
    }
    int32_t values[4] = { 0, 0, 0, 0 };
    for (Py_ssize_t i = 0; i < n; i++) {
        long v = PyLong_AsLong(PyTuple_GET_ITEM(args, i));
        if (v == -1 && PyErr_Occurred()) {
            return NULL;
        }
        values[i] = (int32_t)v;
    }
    return PyBytes_FromStringAndSize((const char *)values, sizeof(values));
}

static PyObject *py_SDL_Point(PyObject *module, PyObject *args) {
    (void)module;
    Py_ssize_t n = PyTuple_GET_SIZE(args);
    if (n > 2) {
        PyErr_SetString(PyExc_TypeError, "SDL_Point takes at most 2 arguments");
        return NULL;
    }
    int32_t values[2] = { 0, 0 };
    for (Py_ssize_t i = 0; i < n; i++) {
        long v = PyLong_AsLong(PyTuple_GET_ITEM(args, i));
        if (v == -1 && PyErr_Occurred()) {
            return NULL;
        }
        values[i] = (int32_t)v;
    }
    return PyBytes_FromStringAndSize((const char *)values, sizeof(values));
}

static PyObject *py_SDL_PumpEvents(PyObject *module, PyObject *noargs) {
    (void)module;
    (void)noargs;
    SDL_PumpEvents();
    Py_RETURN_NONE;
}

static PyObject *py_SDL_GetDisplayUsableBounds(PyObject *module, PyObject *args) {
    (void)module;
    int display_index;
    PyObject *rect_obj = NULL;
    if (!PyArg_ParseTuple(args, "i|O", &display_index, &rect_obj)) {
        return NULL;
    }
    SDL_Rect rect;
    int rc = SDL_GetDisplayUsableBounds(display_index, &rect);
    if (rc == 0 && rect_obj != NULL && rect_obj != Py_None) {
        if (rect_to_obj(&rect, rect_obj) < 0) {
            return NULL;
        }
    }
    return PyLong_FromLong(rc);
}

static PyObject *py_SDL_GetDesktopDisplayMode(PyObject *module, PyObject *args) {
    (void)module;
    int display_index;
    PyObject *mode_obj = NULL;
    if (!PyArg_ParseTuple(args, "i|O", &display_index, &mode_obj)) {
        return NULL;
    }
    SDL_DisplayMode mode;
    int rc = SDL_GetDesktopDisplayMode(display_index, &mode);
    if (rc == 0 && mode_obj != NULL && mode_obj != Py_None) {
        if (display_mode_to_obj(&mode, mode_obj) < 0) {
            return NULL;
        }
    }
    return PyLong_FromLong(rc);
}

static PyObject *py_SDL_DEFINE_PIXELFORMAT(PyObject *module, PyObject *args) {
    (void)module;
    int type, order, layout, bits, bytes;
    if (!PyArg_ParseTuple(args, "iiiii", &type, &order, &layout, &bits, &bytes)) {
        return NULL;
    }
    return PyLong_FromLong(SDL_DEFINE_PIXELFORMAT(type, order, layout, bits, bytes));
}

/* ------------------------------------------------------------------------- */
/* Module definition                                                        */
/* ------------------------------------------------------------------------- */

static PyMethodDef usdl2_methods[] = {
    {"SDL_Init", py_SDL_Init, METH_O, NULL},
    {"SDL_InitSubSystem", py_SDL_InitSubSystem, METH_O, NULL},
    {"SDL_Quit", py_SDL_Quit, METH_NOARGS, NULL},
    {"SDL_GetError", py_SDL_GetError, METH_NOARGS, NULL},
    {"SDL_CreateWindow", py_SDL_CreateWindow, METH_VARARGS, NULL},
    {"SDL_DestroyWindow", py_SDL_DestroyWindow, METH_O, NULL},
    {"SDL_SetWindowSize", py_SDL_SetWindowSize, METH_VARARGS, NULL},
    {"SDL_SetWindowResizable", py_SDL_SetWindowResizable, METH_VARARGS, NULL},
    {"SDL_SetWindowMinimumSize", py_SDL_SetWindowMinimumSize, METH_VARARGS, NULL},
    {"SDL_SetWindowMaximumSize", py_SDL_SetWindowMaximumSize, METH_VARARGS, NULL},
    {"SDL_CreateRenderer", py_SDL_CreateRenderer, METH_VARARGS, NULL},
    {"SDL_DestroyRenderer", py_SDL_DestroyRenderer, METH_O, NULL},
    {"SDL_SetRenderDrawColor", py_SDL_SetRenderDrawColor, METH_VARARGS, NULL},
    {"SDL_SetRenderTarget", py_SDL_SetRenderTarget, METH_VARARGS, NULL},
    {"SDL_RenderClear", py_SDL_RenderClear, METH_O, NULL},
    {"SDL_RenderCopy", py_SDL_RenderCopy, METH_VARARGS, NULL},
    {"SDL_RenderCopyEx", py_SDL_RenderCopyEx, METH_VARARGS, NULL},
    {"SDL_RenderPresent", py_SDL_RenderPresent, METH_O, NULL},
    {"SDL_RenderFillRect", py_SDL_RenderFillRect, METH_VARARGS, NULL},
    {"SDL_RenderSetLogicalSize", py_SDL_RenderSetLogicalSize, METH_VARARGS, NULL},
    {"SDL_CreateTexture", py_SDL_CreateTexture, METH_VARARGS, NULL},
    {"SDL_DestroyTexture", py_SDL_DestroyTexture, METH_O, NULL},
    {"SDL_SetTextureBlendMode", py_SDL_SetTextureBlendMode, METH_VARARGS, NULL},
    {"SDL_UpdateTexture", py_SDL_UpdateTexture, METH_VARARGS, NULL},
    {"SDL_PollEvent", py_SDL_PollEvent, METH_O, NULL},
    {"SDL_GetKeyName", py_SDL_GetKeyName, METH_O, NULL},
    {"SDL_NumJoysticks", py_SDL_NumJoysticks, METH_NOARGS, NULL},
    {"SDL_JoystickOpen", py_SDL_JoystickOpen, METH_O, NULL},
    {"SDL_JoystickClose", py_SDL_JoystickClose, METH_O, NULL},
    {"SDL_JoystickInstanceID", py_SDL_JoystickInstanceID, METH_O, NULL},
    {"SDL_Rect", py_SDL_Rect, METH_VARARGS, NULL},
    {"SDL_Point", py_SDL_Point, METH_VARARGS, NULL},
    {"SDL_TimerCallback", py_SDL_TimerCallback, METH_O, NULL},
    {"SDL_AddTimer", py_SDL_AddTimer, METH_VARARGS, NULL},
    {"SDL_RemoveTimer", py_SDL_RemoveTimer, METH_O, NULL},
    {"SDL_PumpEvents", py_SDL_PumpEvents, METH_NOARGS, NULL},
    {"SDL_GetDisplayUsableBounds", py_SDL_GetDisplayUsableBounds, METH_VARARGS, NULL},
    {"SDL_GetDesktopDisplayMode", py_SDL_GetDesktopDisplayMode, METH_VARARGS, NULL},
    {"SDL_DEFINE_PIXELFORMAT", py_SDL_DEFINE_PIXELFORMAT, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef usdl2_moduledef = {
    PyModuleDef_HEAD_INIT,
    .m_name = "usdl2",
    .m_doc = "Native desktop SDL2 subset for pydisplay (CPython extension).",
    .m_size = -1,
    .m_methods = usdl2_methods,
};

PyMODINIT_FUNC PyInit_usdl2(void) {
    if (PyType_Ready(&UsdlEventType) < 0) {
        return NULL;
    }
    if (PyType_Ready(&UsdlSubviewType) < 0) {
        return NULL;
    }
    if (PyType_Ready(&UsdlTimerCbType) < 0) {
        return NULL;
    }

    PyObject *m = PyModule_Create(&usdl2_moduledef);
    if (m == NULL) {
        return NULL;
    }

    Py_INCREF(&UsdlEventType);
    if (PyModule_AddObject(m, "SDL_Event", (PyObject *)&UsdlEventType) < 0) {
        Py_DECREF(&UsdlEventType);
        Py_DECREF(m);
        return NULL;
    }

#define ADD_INT(name, val) \
    do { \
        if (PyModule_AddIntConstant(m, name, (long)(val)) < 0) { \
            Py_DECREF(m); \
            return NULL; \
        } \
    } while (0)

    ADD_INT("SDL_ARRAYORDER_ABGR", SDL_ARRAYORDER_ABGR);
    ADD_INT("SDL_ARRAYORDER_ARGB", SDL_ARRAYORDER_ARGB);
    ADD_INT("SDL_ARRAYORDER_BGR", SDL_ARRAYORDER_BGR);
    ADD_INT("SDL_ARRAYORDER_BGRA", SDL_ARRAYORDER_BGRA);
    ADD_INT("SDL_ARRAYORDER_NONE", SDL_ARRAYORDER_NONE);
    ADD_INT("SDL_ARRAYORDER_RGB", SDL_ARRAYORDER_RGB);
    ADD_INT("SDL_ARRAYORDER_RGBA", SDL_ARRAYORDER_RGBA);
    ADD_INT("SDL_BITMAPORDER_1234", SDL_BITMAPORDER_1234);
    ADD_INT("SDL_BITMAPORDER_4321", SDL_BITMAPORDER_4321);
    ADD_INT("SDL_BITMAPORDER_NONE", SDL_BITMAPORDER_NONE);
    ADD_INT("SDL_BLENDMODE_ADD", SDL_BLENDMODE_ADD);
    ADD_INT("SDL_BLENDMODE_BLEND", SDL_BLENDMODE_BLEND);
    ADD_INT("SDL_BLENDMODE_MOD", SDL_BLENDMODE_MOD);
    ADD_INT("SDL_BLENDMODE_MUL", SDL_BLENDMODE_MUL);
    ADD_INT("SDL_BLENDMODE_NONE", SDL_BLENDMODE_NONE);
    ADD_INT("SDL_BUTTON_LMASK", SDL_BUTTON_LMASK);
    ADD_INT("SDL_BUTTON_MMASK", SDL_BUTTON_MMASK);
    ADD_INT("SDL_BUTTON_RMASK", SDL_BUTTON_RMASK);
    ADD_INT("SDL_HAT_CENTERED", SDL_HAT_CENTERED);
    ADD_INT("SDL_HAT_DOWN", SDL_HAT_DOWN);
    ADD_INT("SDL_HAT_LEFT", SDL_HAT_LEFT);
    ADD_INT("SDL_HAT_RIGHT", SDL_HAT_RIGHT);
    ADD_INT("SDL_HAT_UP", SDL_HAT_UP);
    ADD_INT("SDL_INIT_AUDIO", SDL_INIT_AUDIO);
    ADD_INT("SDL_INIT_EVENTS", SDL_INIT_EVENTS);
    ADD_INT("SDL_INIT_EVERYTHING", SDL_INIT_EVERYTHING);
    ADD_INT("SDL_INIT_GAMECONTROLLER", SDL_INIT_GAMECONTROLLER);
    ADD_INT("SDL_INIT_HAPTIC", SDL_INIT_HAPTIC);
    ADD_INT("SDL_INIT_JOYSTICK", SDL_INIT_JOYSTICK);
    ADD_INT("SDL_INIT_NOPARACHUTE", SDL_INIT_NOPARACHUTE);
    ADD_INT("SDL_INIT_TIMER", SDL_INIT_TIMER);
    ADD_INT("SDL_INIT_VIDEO", SDL_INIT_VIDEO);
    ADD_INT("SDL_JOYAXISMOTION", SDL_JOYAXISMOTION);
    ADD_INT("SDL_JOYBALLMOTION", SDL_JOYBALLMOTION);
    ADD_INT("SDL_JOYBUTTONDOWN", SDL_JOYBUTTONDOWN);
    ADD_INT("SDL_JOYBUTTONUP", SDL_JOYBUTTONUP);
    ADD_INT("SDL_JOYDEVICEADDED", SDL_JOYDEVICEADDED);
    ADD_INT("SDL_JOYDEVICEREMOVED", SDL_JOYDEVICEREMOVED);
    ADD_INT("SDL_JOYHATMOTION", SDL_JOYHATMOTION);
    ADD_INT("SDL_KEYDOWN", SDL_KEYDOWN);
    ADD_INT("SDL_KEYUP", SDL_KEYUP);
    ADD_INT("SDL_MOUSEBUTTONDOWN", SDL_MOUSEBUTTONDOWN);
    ADD_INT("SDL_MOUSEBUTTONUP", SDL_MOUSEBUTTONUP);
    ADD_INT("SDL_MOUSEMOTION", SDL_MOUSEMOTION);
    ADD_INT("SDL_MOUSEWHEEL", SDL_MOUSEWHEEL);
    ADD_INT("SDL_PACKEDLAYOUT_1010102", SDL_PACKEDLAYOUT_1010102);
    ADD_INT("SDL_PACKEDLAYOUT_1555", SDL_PACKEDLAYOUT_1555);
    ADD_INT("SDL_PACKEDLAYOUT_2101010", SDL_PACKEDLAYOUT_2101010);
    ADD_INT("SDL_PACKEDLAYOUT_332", SDL_PACKEDLAYOUT_332);
    ADD_INT("SDL_PACKEDLAYOUT_4444", SDL_PACKEDLAYOUT_4444);
    ADD_INT("SDL_PACKEDLAYOUT_5551", SDL_PACKEDLAYOUT_5551);
    ADD_INT("SDL_PACKEDLAYOUT_565", SDL_PACKEDLAYOUT_565);
    ADD_INT("SDL_PACKEDLAYOUT_8888", SDL_PACKEDLAYOUT_8888);
    ADD_INT("SDL_PACKEDLAYOUT_NONE", SDL_PACKEDLAYOUT_NONE);
    ADD_INT("SDL_PACKEDORDER_ABGR", SDL_PACKEDORDER_ABGR);
    ADD_INT("SDL_PACKEDORDER_ARGB", SDL_PACKEDORDER_ARGB);
    ADD_INT("SDL_PACKEDORDER_BGRA", SDL_PACKEDORDER_BGRA);
    ADD_INT("SDL_PACKEDORDER_BGRX", SDL_PACKEDORDER_BGRX);
    ADD_INT("SDL_PACKEDORDER_NONE", SDL_PACKEDORDER_NONE);
    ADD_INT("SDL_PACKEDORDER_RGBA", SDL_PACKEDORDER_RGBA);
    ADD_INT("SDL_PACKEDORDER_RGBX", SDL_PACKEDORDER_RGBX);
    ADD_INT("SDL_PACKEDORDER_XBGR", SDL_PACKEDORDER_XBGR);
    ADD_INT("SDL_PACKEDORDER_XRGB", SDL_PACKEDORDER_XRGB);
    ADD_INT("SDL_PIXELFORMAT_ABGR1555", SDL_PIXELFORMAT_ABGR1555);
    ADD_INT("SDL_PIXELFORMAT_ABGR4444", SDL_PIXELFORMAT_ABGR4444);
    ADD_INT("SDL_PIXELFORMAT_ABGR8888", SDL_PIXELFORMAT_ABGR8888);
    ADD_INT("SDL_PIXELFORMAT_ARGB1555", SDL_PIXELFORMAT_ARGB1555);
    ADD_INT("SDL_PIXELFORMAT_ARGB2101010", SDL_PIXELFORMAT_ARGB2101010);
    ADD_INT("SDL_PIXELFORMAT_ARGB4444", SDL_PIXELFORMAT_ARGB4444);
    ADD_INT("SDL_PIXELFORMAT_ARGB8888", SDL_PIXELFORMAT_ARGB8888);
    ADD_INT("SDL_PIXELFORMAT_BGR24", SDL_PIXELFORMAT_BGR24);
    ADD_INT("SDL_PIXELFORMAT_BGR444", SDL_PIXELFORMAT_BGR444);
    ADD_INT("SDL_PIXELFORMAT_BGR555", SDL_PIXELFORMAT_BGR555);
    ADD_INT("SDL_PIXELFORMAT_BGR565", SDL_PIXELFORMAT_BGR565);
    ADD_INT("SDL_PIXELFORMAT_BGR888", SDL_PIXELFORMAT_BGR888);
    ADD_INT("SDL_PIXELFORMAT_BGRA4444", SDL_PIXELFORMAT_BGRA4444);
    ADD_INT("SDL_PIXELFORMAT_BGRA5551", SDL_PIXELFORMAT_BGRA5551);
    ADD_INT("SDL_PIXELFORMAT_BGRA8888", SDL_PIXELFORMAT_BGRA8888);
    ADD_INT("SDL_PIXELFORMAT_BGRX8888", SDL_PIXELFORMAT_BGRX8888);
    ADD_INT("SDL_PIXELFORMAT_INDEX1LSB", SDL_PIXELFORMAT_INDEX1LSB);
    ADD_INT("SDL_PIXELFORMAT_INDEX1MSB", SDL_PIXELFORMAT_INDEX1MSB);
    ADD_INT("SDL_PIXELFORMAT_INDEX4LSB", SDL_PIXELFORMAT_INDEX4LSB);
    ADD_INT("SDL_PIXELFORMAT_INDEX4MSB", SDL_PIXELFORMAT_INDEX4MSB);
    ADD_INT("SDL_PIXELFORMAT_INDEX8", SDL_PIXELFORMAT_INDEX8);
    ADD_INT("SDL_PIXELFORMAT_RGB24", SDL_PIXELFORMAT_RGB24);
    ADD_INT("SDL_PIXELFORMAT_RGB332", SDL_PIXELFORMAT_RGB332);
    ADD_INT("SDL_PIXELFORMAT_RGB444", SDL_PIXELFORMAT_RGB444);
    ADD_INT("SDL_PIXELFORMAT_RGB555", SDL_PIXELFORMAT_RGB555);
    ADD_INT("SDL_PIXELFORMAT_RGB565", SDL_PIXELFORMAT_RGB565);
    ADD_INT("SDL_PIXELFORMAT_RGB888", SDL_PIXELFORMAT_RGB888);
    ADD_INT("SDL_PIXELFORMAT_RGBA4444", SDL_PIXELFORMAT_RGBA4444);
    ADD_INT("SDL_PIXELFORMAT_RGBA5551", SDL_PIXELFORMAT_RGBA5551);
    ADD_INT("SDL_PIXELFORMAT_RGBA8888", SDL_PIXELFORMAT_RGBA8888);
    ADD_INT("SDL_PIXELFORMAT_RGBX8888", SDL_PIXELFORMAT_RGBX8888);
    ADD_INT("SDL_PIXELFORMAT_UNKNOWN", SDL_PIXELFORMAT_UNKNOWN);
    ADD_INT("SDL_PIXELFORMAT_XBGR1555", SDL_PIXELFORMAT_XBGR1555);
    ADD_INT("SDL_PIXELFORMAT_XBGR4444", SDL_PIXELFORMAT_XBGR4444);
    ADD_INT("SDL_PIXELFORMAT_XBGR8888", SDL_PIXELFORMAT_XBGR8888);
    ADD_INT("SDL_PIXELFORMAT_XRGB1555", SDL_PIXELFORMAT_XRGB1555);
    ADD_INT("SDL_PIXELFORMAT_XRGB4444", SDL_PIXELFORMAT_XRGB4444);
    ADD_INT("SDL_PIXELFORMAT_XRGB8888", SDL_PIXELFORMAT_XRGB8888);
    ADD_INT("SDL_PIXELTYPE_ARRAYF16", SDL_PIXELTYPE_ARRAYF16);
    ADD_INT("SDL_PIXELTYPE_ARRAYF32", SDL_PIXELTYPE_ARRAYF32);
    ADD_INT("SDL_PIXELTYPE_ARRAYU16", SDL_PIXELTYPE_ARRAYU16);
    ADD_INT("SDL_PIXELTYPE_ARRAYU32", SDL_PIXELTYPE_ARRAYU32);
    ADD_INT("SDL_PIXELTYPE_ARRAYU8", SDL_PIXELTYPE_ARRAYU8);
    ADD_INT("SDL_PIXELTYPE_INDEX1", SDL_PIXELTYPE_INDEX1);
    ADD_INT("SDL_PIXELTYPE_INDEX4", SDL_PIXELTYPE_INDEX4);
    ADD_INT("SDL_PIXELTYPE_INDEX8", SDL_PIXELTYPE_INDEX8);
    ADD_INT("SDL_PIXELTYPE_PACKED16", SDL_PIXELTYPE_PACKED16);
    ADD_INT("SDL_PIXELTYPE_PACKED32", SDL_PIXELTYPE_PACKED32);
    ADD_INT("SDL_PIXELTYPE_PACKED8", SDL_PIXELTYPE_PACKED8);
    ADD_INT("SDL_PIXELTYPE_UNKNOWN", SDL_PIXELTYPE_UNKNOWN);
    ADD_INT("SDL_POLLSENTINEL", SDL_POLLSENTINEL);
    ADD_INT("SDL_QUIT", SDL_QUIT);
    ADD_INT("SDL_RENDERER_ACCELERATED", SDL_RENDERER_ACCELERATED);
    ADD_INT("SDL_RENDERER_PRESENTVSYNC", SDL_RENDERER_PRESENTVSYNC);
    ADD_INT("SDL_RENDERER_SOFTWARE", SDL_RENDERER_SOFTWARE);
    ADD_INT("SDL_RENDERER_TARGETTEXTURE", SDL_RENDERER_TARGETTEXTURE);
    ADD_INT("SDL_TEXTUREACCESS_STATIC", SDL_TEXTUREACCESS_STATIC);
    ADD_INT("SDL_TEXTUREACCESS_STREAMING", SDL_TEXTUREACCESS_STREAMING);
    ADD_INT("SDL_TEXTUREACCESS_TARGET", SDL_TEXTUREACCESS_TARGET);
    ADD_INT("SDL_WINDOWPOS_CENTERED", SDL_WINDOWPOS_CENTERED);
    ADD_INT("SDL_WINDOWPOS_UNDEFINED", SDL_WINDOWPOS_UNDEFINED);
    ADD_INT("SDL_WINDOW_ALLOW_HIGHDPI", SDL_WINDOW_ALLOW_HIGHDPI);
    ADD_INT("SDL_WINDOW_ALWAYS_ON_TOP", SDL_WINDOW_ALWAYS_ON_TOP);
    ADD_INT("SDL_WINDOW_BORDERLESS", SDL_WINDOW_BORDERLESS);
    ADD_INT("SDL_WINDOW_FULLSCREEN", SDL_WINDOW_FULLSCREEN);
    ADD_INT("SDL_WINDOW_FULLSCREEN_DESKTOP", SDL_WINDOW_FULLSCREEN_DESKTOP);
    ADD_INT("SDL_WINDOW_HIDDEN", SDL_WINDOW_HIDDEN);
    ADD_INT("SDL_WINDOW_INPUT_FOCUS", SDL_WINDOW_INPUT_FOCUS);
    ADD_INT("SDL_WINDOW_INPUT_GRABBED", SDL_WINDOW_INPUT_GRABBED);
    ADD_INT("SDL_WINDOW_MAXIMIZED", SDL_WINDOW_MAXIMIZED);
    ADD_INT("SDL_WINDOW_MINIMIZED", SDL_WINDOW_MINIMIZED);
    ADD_INT("SDL_WINDOW_MOUSE_CAPTURE", SDL_WINDOW_MOUSE_CAPTURE);
    ADD_INT("SDL_WINDOW_MOUSE_FOCUS", SDL_WINDOW_MOUSE_FOCUS);
    ADD_INT("SDL_WINDOW_OPENGL", SDL_WINDOW_OPENGL);
    ADD_INT("SDL_WINDOW_POPUP_MENU", SDL_WINDOW_POPUP_MENU);
    ADD_INT("SDL_WINDOW_RESIZABLE", SDL_WINDOW_RESIZABLE);
    ADD_INT("SDL_WINDOW_SHOWN", SDL_WINDOW_SHOWN);
    ADD_INT("SDL_WINDOW_SKIP_TASKBAR", SDL_WINDOW_SKIP_TASKBAR);
    ADD_INT("SDL_WINDOW_TOOLTIP", SDL_WINDOW_TOOLTIP);
    ADD_INT("SDL_WINDOW_UTILITY", SDL_WINDOW_UTILITY);
    ADD_INT("SDL_WINDOW_VULKAN", SDL_WINDOW_VULKAN);

#undef ADD_INT

    if (g_timer_mutex == NULL) {
        g_timer_mutex = SDL_CreateMutex();
        if (g_timer_mutex == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "SDL_CreateMutex failed");
            Py_DECREF(m);
            return NULL;
        }
    }

    return m;
}
