// Copy to circuitpython/shared-bindings/usdl2/__init__.c

#include "py/runtime.h"
#include "py/obj.h"
#include "usdl2.h"

//| """Unix-only SDL2 subset for pydisplay (linked against libSDL2)."""

static mp_obj_t usdl2_init_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_init(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_init_fun_obj, 1, 1, usdl2_init_obj);

static mp_obj_t usdl2_quit_obj(void) {
    return usdl2_quit();
}
static MP_DEFINE_CONST_FUN_OBJ_0(usdl2_quit_fun_obj, usdl2_quit_obj);

static mp_obj_t usdl2_process_exit_obj(mp_obj_t code_in) {
    return usdl2_process_exit(code_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_process_exit_fun_obj, usdl2_process_exit_obj);

static mp_obj_t usdl2_get_error_obj(void) {
    return usdl2_get_error();
}
static MP_DEFINE_CONST_FUN_OBJ_0(usdl2_get_error_fun_obj, usdl2_get_error_obj);

static mp_obj_t usdl2_create_window_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_create_window(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_create_window_fun_obj, 6, 6, usdl2_create_window_obj);

static mp_obj_t usdl2_destroy_window_obj(mp_obj_t win_in) {
    return usdl2_destroy_window(win_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_destroy_window_fun_obj, usdl2_destroy_window_obj);

static mp_obj_t usdl2_set_window_size_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_set_window_size(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_set_window_size_fun_obj, 3, 3, usdl2_set_window_size_obj);

static mp_obj_t usdl2_create_renderer_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_create_renderer(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_create_renderer_fun_obj, 3, 3, usdl2_create_renderer_obj);

static mp_obj_t usdl2_destroy_renderer_obj(mp_obj_t renderer_in) {
    return usdl2_destroy_renderer(renderer_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_destroy_renderer_fun_obj, usdl2_destroy_renderer_obj);

static mp_obj_t usdl2_set_render_draw_color_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_set_render_draw_color(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_set_render_draw_color_fun_obj, 5, 5, usdl2_set_render_draw_color_obj);

static mp_obj_t usdl2_set_render_target_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_set_render_target(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_set_render_target_fun_obj, 2, 2, usdl2_set_render_target_obj);

static mp_obj_t usdl2_render_clear_obj(mp_obj_t renderer_in) {
    return usdl2_render_clear(renderer_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_render_clear_fun_obj, usdl2_render_clear_obj);

static mp_obj_t usdl2_render_copy_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_render_copy(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_render_copy_fun_obj, 4, 4, usdl2_render_copy_obj);

static mp_obj_t usdl2_render_copyex_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_render_copy_ex(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_render_copyex_fun_obj, 7, 7, usdl2_render_copyex_obj);

static mp_obj_t usdl2_render_present_obj(mp_obj_t renderer_in) {
    return usdl2_render_present(renderer_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_render_present_fun_obj, usdl2_render_present_obj);

static mp_obj_t usdl2_render_fill_rect_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_render_fill_rect(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_render_fill_rect_fun_obj, 2, 2, usdl2_render_fill_rect_obj);

static mp_obj_t usdl2_render_set_logical_size_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_render_set_logical_size(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_render_set_logical_size_fun_obj, 3, 3, usdl2_render_set_logical_size_obj);

static mp_obj_t usdl2_create_texture_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_create_texture(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_create_texture_fun_obj, 5, 5, usdl2_create_texture_obj);

static mp_obj_t usdl2_destroy_texture_obj(mp_obj_t texture_in) {
    return usdl2_destroy_texture(texture_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_destroy_texture_fun_obj, usdl2_destroy_texture_obj);

static mp_obj_t usdl2_set_texture_blend_mode_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_set_texture_blend_mode(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_set_texture_blend_mode_fun_obj, 2, 2, usdl2_set_texture_blend_mode_obj);

static mp_obj_t usdl2_update_texture_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_update_texture(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_update_texture_fun_obj, 4, 4, usdl2_update_texture_obj);

static mp_obj_t usdl2_poll_event_obj(mp_obj_t event_in) {
    return usdl2_poll_event(event_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_poll_event_fun_obj, usdl2_poll_event_obj);

static mp_obj_t usdl2_get_key_name_obj(mp_obj_t sym_in) {
    return usdl2_get_key_name(sym_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_get_key_name_fun_obj, usdl2_get_key_name_obj);

static mp_obj_t usdl2_rect_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_rect_helper(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_rect_fun_obj, 4, 4, usdl2_rect_obj);

static mp_obj_t usdl2_add_timer_obj(size_t n_args, const mp_obj_t *args) {
    return usdl2_add_timer(n_args, args);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usdl2_add_timer_fun_obj, 2, 3, usdl2_add_timer_obj);

static mp_obj_t usdl2_remove_timer_obj(mp_obj_t timer_in) {
    return usdl2_remove_timer(timer_in);
}
static MP_DEFINE_CONST_FUN_OBJ_1(usdl2_remove_timer_fun_obj, usdl2_remove_timer_obj);

static const mp_rom_map_elem_t usdl2_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_usdl2) },
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&usdl2_init_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_quit), MP_ROM_PTR(&usdl2_quit_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_process_exit), MP_ROM_PTR(&usdl2_process_exit_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_error), MP_ROM_PTR(&usdl2_get_error_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_create_window), MP_ROM_PTR(&usdl2_create_window_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_destroy_window), MP_ROM_PTR(&usdl2_destroy_window_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_window_size), MP_ROM_PTR(&usdl2_set_window_size_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_create_renderer), MP_ROM_PTR(&usdl2_create_renderer_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_destroy_renderer), MP_ROM_PTR(&usdl2_destroy_renderer_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_render_draw_color), MP_ROM_PTR(&usdl2_set_render_draw_color_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_render_target), MP_ROM_PTR(&usdl2_set_render_target_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_render_clear), MP_ROM_PTR(&usdl2_render_clear_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_render_copy), MP_ROM_PTR(&usdl2_render_copy_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_render_copyex), MP_ROM_PTR(&usdl2_render_copyex_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_render_present), MP_ROM_PTR(&usdl2_render_present_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_render_fill_rect), MP_ROM_PTR(&usdl2_render_fill_rect_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_render_set_logical_size), MP_ROM_PTR(&usdl2_render_set_logical_size_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_create_texture), MP_ROM_PTR(&usdl2_create_texture_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_destroy_texture), MP_ROM_PTR(&usdl2_destroy_texture_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_texture_blend_mode), MP_ROM_PTR(&usdl2_set_texture_blend_mode_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_update_texture), MP_ROM_PTR(&usdl2_update_texture_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_poll_event), MP_ROM_PTR(&usdl2_poll_event_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_key_name), MP_ROM_PTR(&usdl2_get_key_name_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&usdl2_rect_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_add_timer), MP_ROM_PTR(&usdl2_add_timer_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove_timer), MP_ROM_PTR(&usdl2_remove_timer_fun_obj) },
    { MP_ROM_QSTR(MP_QSTR_Event), MP_ROM_PTR(&usdl2_event_type) },
};

static MP_DEFINE_CONST_DICT(usdl2_module_globals, usdl2_module_globals_table);

const mp_obj_module_t usdl2_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&usdl2_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_usdl2, usdl2_module);
