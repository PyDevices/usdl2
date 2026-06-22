// SPDX-License-Identifier: MIT
// usdl2 — desktop pydisplay-sized SDL2 subset for MicroPython and CircuitPython.

#pragma once

#include "py/obj.h"
#include <SDL.h>
#include <stdint.h>

#define USDL2_EVENT_SIZE 56

extern const mp_obj_type_t usdl2_event_type;

void *usdl2_ptr_from_obj(mp_obj_t obj);
mp_obj_t usdl2_ptr_obj(void *ptr);
bool usdl2_obj_is_none_or_null(mp_obj_t obj);
void usdl2_rect_from_bytes(mp_obj_t obj, SDL_Rect *rect);
SDL_Rect *usdl2_optional_rect(mp_obj_t obj, SDL_Rect *storage);

mp_obj_t usdl2_event_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
uint8_t *usdl2_event_buffer(mp_obj_t self);
mp_int_t usdl2_event_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags);

mp_obj_t usdl2_init(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_quit(void);
mp_obj_t usdl2_process_exit(mp_obj_t code_in);
mp_obj_t usdl2_get_error(void);
mp_obj_t usdl2_create_window(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_destroy_window(mp_obj_t win_in);
mp_obj_t usdl2_set_window_size(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_create_renderer(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_destroy_renderer(mp_obj_t renderer_in);
mp_obj_t usdl2_set_render_draw_color(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_set_render_target(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_render_clear(mp_obj_t renderer_in);
mp_obj_t usdl2_render_copy(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_render_copy_ex(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_render_present(mp_obj_t renderer_in);
mp_obj_t usdl2_render_fill_rect(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_render_set_logical_size(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_create_texture(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_destroy_texture(mp_obj_t texture_in);
mp_obj_t usdl2_set_texture_blend_mode(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_update_texture(size_t n_args, const mp_obj_t *args);
mp_obj_t usdl2_poll_event(mp_obj_t event_in);
mp_obj_t usdl2_get_key_name(mp_obj_t sym_in);
mp_obj_t usdl2_rect_helper(size_t n_args, const mp_obj_t *args);
