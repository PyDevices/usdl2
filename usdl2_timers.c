// SPDX-License-Identifier: MIT

#include "usdl2.h"

#include "py/runtime.h"

#define USDL2_MAX_TIMERS 16

static mp_obj_t usdl2_timer_callbacks[USDL2_MAX_TIMERS];

MP_REGISTER_ROOT_POINTER(mp_obj_t usdl2_timer_callbacks[USDL2_MAX_TIMERS]);

static uint32_t usdl2_timer_intervals[USDL2_MAX_TIMERS];
static bool usdl2_timer_periodic[USDL2_MAX_TIMERS];
static bool usdl2_timer_active[USDL2_MAX_TIMERS];
static SDL_TimerID usdl2_timer_ids[USDL2_MAX_TIMERS];

static int usdl2_timer_find_index(SDL_TimerID id) {
    for (size_t i = 0; i < USDL2_MAX_TIMERS; i++) {
        if (usdl2_timer_active[i] && usdl2_timer_ids[i] == id) {
            return (int)i;
        }
    }
    return -1;
}

static int usdl2_timer_alloc_index(void) {
    for (size_t i = 0; i < USDL2_MAX_TIMERS; i++) {
        if (!usdl2_timer_active[i]) {
            return (int)i;
        }
    }
    return -1;
}

static uint32_t usdl2_sdl_timer_cb(uint32_t interval, void *param) {
    size_t index = (size_t)(uintptr_t)param;
    if (index >= USDL2_MAX_TIMERS || !usdl2_timer_active[index]) {
        return 0;
    }
    mp_obj_t callback = usdl2_timer_callbacks[index];
    if (callback == MP_OBJ_NULL) {
        return 0;
    }
    #if MICROPY_ENABLE_SCHEDULER
    (void)mp_sched_schedule(callback, mp_obj_new_int(interval));
    #endif
    if (!usdl2_timer_periodic[index]) {
        return 0;
    }
    return usdl2_timer_intervals[index];
}

mp_obj_t usdl2_add_timer(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    uint32_t interval_ms = (uint32_t)mp_obj_get_int(args[0]);
    mp_obj_t callback = args[1];
    bool periodic = true;
    if (n_args > 2) {
        periodic = mp_obj_is_true(args[2]);
    }
    if (!mp_obj_is_callable(callback)) {
        mp_raise_TypeError(MP_ERROR_TEXT("callback must be callable"));
    }
    if (interval_ms == 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("interval must be > 0"));
    }

    int index = usdl2_timer_alloc_index();
    if (index < 0) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("no free usdl2 timers"));
    }

    usdl2_timer_callbacks[index] = callback;
    usdl2_timer_intervals[index] = interval_ms;
    usdl2_timer_periodic[index] = periodic;
    usdl2_timer_active[index] = true;

    SDL_TimerID id = SDL_AddTimer(interval_ms, usdl2_sdl_timer_cb, (void *)(uintptr_t)index);
    if (id == 0) {
        usdl2_timer_active[index] = false;
        usdl2_timer_callbacks[index] = MP_OBJ_NULL;
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("SDL_AddTimer failed"));
    }
    usdl2_timer_ids[index] = id;
    return usdl2_ptr_obj((void *)(uintptr_t)id);
}

mp_obj_t usdl2_remove_timer(mp_obj_t timer_in) {
    SDL_TimerID id = (SDL_TimerID)(uintptr_t)usdl2_ptr_from_obj(timer_in);
    int index = usdl2_timer_find_index(id);
    if (index >= 0) {
        usdl2_timer_active[index] = false;
        usdl2_timer_callbacks[index] = MP_OBJ_NULL;
        usdl2_timer_ids[index] = 0;
    }
    return mp_obj_new_bool(SDL_RemoveTimer(id));
}
