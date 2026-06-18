// SPDX-License-Identifier: MIT

#include "usdl2.h"

#include "py/binary.h"
#include "py/runtime.h"
#include <string.h>
#include <unistd.h>

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

mp_obj_t usdl2_create_window(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    const char *title = mp_obj_str_get_str(args[0]);
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

mp_obj_t usdl2_rect_helper(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    int32_t values[4] = {
        (int32_t)mp_obj_get_int(args[0]),
        (int32_t)mp_obj_get_int(args[1]),
        (int32_t)mp_obj_get_int(args[2]),
        (int32_t)mp_obj_get_int(args[3]),
    };
    return mp_obj_new_bytes((const byte *)values, sizeof(values));
}
