/*
================================================================================

                Legato - a Lua based gaming framework

 This is free and unencumbered software released into the public domain.

 Anyone is free to copy, modify, publish, use, compile, sell, or
 distribute this software, either in source code form or as a compiled
 binary, for any purpose, commercial or non-commercial, and by any
 means.

 In jurisdictions that recognize copyright laws, the author or authors
 of this software dedicate any and all copyright interest in the
 software to the public domain. We make this dedication for the benefit
 of the public at large and to the detriment of our heirs and
 successors. We intend this dedication to be an overt act of
 relinquishment in perpetuity of all present and future rights to this
 software under copyright law.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.

 For more information, please refer to <http://unlicense.org/>

 This software was written by Sebastian Steinhauer <s.steinhauer@yahoo.de>



 Chanelog:
 ---------

 2013-10-17 - 0.1.0
    This is the first version. Almost every Allegro 5 and PhysFS stuff is
    implemented right now. enet (robust fast UDP networking) is non-functional
    right now.

================================================================================
*/
/*
================================================================================

                INCLUDES

================================================================================
*/
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_physfs.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_color.h>

#include <physfs.h>
#include <enet/enet.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef ALLEGRO_WINDOWS
#include <allegro5/allegro_direct3d.h>
#define LEGATO_DIRECT3D ALLEGRO_DIRECT3D
#else
#define LEGATO_DIRECT3D 0
#endif /* ALLEGRO_WINDOWS */

typedef struct mapping_t {
    const char      *name;
    const int       value;
} mapping_t;

typedef struct object_t {
    void        *ptr;
    const char  *name;
    int         destroy;
    int         dependency_ref;
} object_t;

#define NOT_IMPLEMENTED_MACRO luaL_error(L, "Error: not implemented yet!"); return 0;

#define LEGATO_VERSION_MAJOR    0
#define LEGATO_VERSION_MINOR    1
#define LEGATO_VERSION_PATCH    0

/*
================================================================================

                OBJECT NAMES

================================================================================
*/
#define LEGATO_CONFIG "legato_config"
#define LEGATO_DISPLAY "legato_display"
#define LEGATO_COLOR "legato_color"
#define LEGATO_BITMAP "legato_bitmap"
#define LEGATO_EVENT_QUEUE "legato_event_queue"
#define LEGATO_KEYBOARD_STATE "legato_keyboard_state"
#define LEGATO_MOUSE_STATE "legato_mouse_state"
#define LEGATO_PATH "legato_path"
#define LEGATO_STATE "legato_state"
#define LEGATO_TIMER "legato_timer"
#define LEGATO_TRANSFORM "legato_transform"
#define LEGATO_VOICE "legato_voice"
#define LEGATO_MIXER "legato_mixer"
#define LEGATO_AUDIO_SAMPLE "legato_audio_sample"
#define LEGATO_SAMPLE_ID "legato_sample_id"
#define LEGATO_SAMPLE_INSTANCE "legato_sample_instance"
#define LEGATO_AUDIO_STREAM "legato_audio_stream"
#define LEGATO_FONT "legato_font"
#define LEGATO_FILE "legato_file"
#define LEGATO_HOST "legato_host"
#define LEGATO_PEER "legato_peer"

/*
================================================================================

                PROTOTYPES

================================================================================
*/
static ALLEGRO_CONFIG *to_config( lua_State *L, const int idx );
static ALLEGRO_DISPLAY *to_display( lua_State *L, const int idx );
static ALLEGRO_COLOR to_color( lua_State *L, const int idx );
static ALLEGRO_BITMAP *to_bitmap( lua_State *L, const int idx );
static ALLEGRO_EVENT_QUEUE *to_event_queue( lua_State *L, const int idx );
static ALLEGRO_KEYBOARD_STATE *to_keyboard_state( lua_State *L, const int idx );
static ALLEGRO_MOUSE_STATE *to_mouse_state( lua_State *L, const int idx );
static ALLEGRO_PATH *to_path( lua_State *L, const int idx );
static ALLEGRO_STATE *to_state( lua_State *L, const int idx );
static ALLEGRO_TIMER *to_timer( lua_State *L, const int idx );
static ALLEGRO_TRANSFORM *to_transform( lua_State *L, const int idx );
static ALLEGRO_VOICE *to_voice( lua_State *L, const int idx );
static ALLEGRO_MIXER *to_mixer( lua_State *L, const int idx );
static ALLEGRO_SAMPLE *to_audio_sample( lua_State *L, const int idx );
static ALLEGRO_SAMPLE_ID *to_sample_id( lua_State *L, const int idx );
static ALLEGRO_SAMPLE_INSTANCE *to_sample_instance( lua_State *L, const int idx );
static ALLEGRO_AUDIO_STREAM *to_audio_stream( lua_State *L, const int idx );
static ALLEGRO_FONT *to_font( lua_State *L, const int idx );
static PHYSFS_File *to_file( lua_State *L, const int idx );
static ENetHost *to_host( lua_State *L, const int idx );
static ENetPeer *to_peer( lua_State *L, const int idx );

/*
================================================================================

                ENUMS

================================================================================
*/
static const mapping_t display_flag_mapping[] = {
    {"windowed", ALLEGRO_WINDOWED},
    {"fullscreen", ALLEGRO_FULLSCREEN},
    {"fullscreen_window", ALLEGRO_FULLSCREEN_WINDOW},
    {"resizable", ALLEGRO_RESIZABLE},
    {"opengl", ALLEGRO_OPENGL},
    {"opengl_3_0", ALLEGRO_OPENGL_3_0},
    {"opengl_forward_compatible", ALLEGRO_OPENGL_FORWARD_COMPATIBLE},
    {"direct3d", LEGATO_DIRECT3D},
    {"frameless", ALLEGRO_FRAMELESS},
    {"generate_expose_events", ALLEGRO_GENERATE_EXPOSE_EVENTS},
    {"minimized", ALLEGRO_MINIMIZED},
    {NULL, 0}
};

static const mapping_t display_importance_mapping[] = {
    {"require", ALLEGRO_REQUIRE},
    {"suggest", ALLEGRO_SUGGEST},
    {"dontcare", ALLEGRO_DONTCARE},
    {NULL, 0}
};

static const mapping_t display_option_mapping[] = {
    {"color_size", ALLEGRO_COLOR_SIZE},
    {"red_size", ALLEGRO_RED_SIZE},
    {"green_size", ALLEGRO_GREEN_SIZE},
    {"blue_size", ALLEGRO_BLUE_SIZE},
    {"alpha_size", ALLEGRO_ALPHA_SIZE},
    {"red_shift", ALLEGRO_RED_SHIFT},
    {"green_shift", ALLEGRO_GREEN_SHIFT},
    {"blue_shift", ALLEGRO_BLUE_SHIFT},
    {"alpha_shift", ALLEGRO_ALPHA_SHIFT},
    {"acc_red_size", ALLEGRO_RED_SIZE},
    {"acc_green_size", ALLEGRO_GREEN_SIZE},
    {"acc_blue_size", ALLEGRO_BLUE_SIZE},
    {"acc_alpha_size", ALLEGRO_ALPHA_SIZE},
    {"stereo", ALLEGRO_STEREO},
    {"aux_buffers", ALLEGRO_AUX_BUFFERS},
    {"depth_size", ALLEGRO_DEPTH_SIZE},
    {"stencil_size", ALLEGRO_STENCIL_SIZE},
    {"smaple_buffers", ALLEGRO_SAMPLE_BUFFERS},
    {"samples", ALLEGRO_SAMPLES},
    {"render_method", ALLEGRO_RENDER_METHOD},
    {"float_color", ALLEGRO_FLOAT_COLOR},
    {"float_depth", ALLEGRO_FLOAT_DEPTH},
    {"single_buffer", ALLEGRO_SINGLE_BUFFER},
    {"swap_method", ALLEGRO_SWAP_METHOD},
    {"compatible_display", ALLEGRO_COMPATIBLE_DISPLAY},
    {"update_display_region", ALLEGRO_UPDATE_DISPLAY_REGION},
    {"vsync", ALLEGRO_VSYNC},
    {"max_bitmap_size", ALLEGRO_MAX_BITMAP_SIZE},
    {"support_npot_bitmap", ALLEGRO_SUPPORT_NPOT_BITMAP},
    {"can_draw_into_bitmap", ALLEGRO_CAN_DRAW_INTO_BITMAP},
    {"support_separate_alpha", ALLEGRO_SUPPORT_SEPARATE_ALPHA},
    {NULL, 0}
};

static const mapping_t pixel_format_mapping[] = {
    {"any", ALLEGRO_PIXEL_FORMAT_ANY},
    {"any_no_alpha", ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA},
    {"any_with_alpha", ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA},
    {"any_15_no_alpha", ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA},
    {"any_16_no_alpha", ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA},
    {"any_16_with_alpha", ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA},
    {"any_24_no_alpha", ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA},
    {"any_32_no_alpha", ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA},
    {"any_32_with_alpha", ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA},
    {"argb_8888", ALLEGRO_PIXEL_FORMAT_ARGB_8888},
    {"rgba_8888", ALLEGRO_PIXEL_FORMAT_RGBA_8888},
    {"argb_4444", ALLEGRO_PIXEL_FORMAT_ARGB_4444},
    {"rgb_888", ALLEGRO_PIXEL_FORMAT_RGB_888},
    {"rgb_565", ALLEGRO_PIXEL_FORMAT_RGB_565},
    {"rgb_555", ALLEGRO_PIXEL_FORMAT_RGB_555},
    {"rgba_5551", ALLEGRO_PIXEL_FORMAT_RGBA_5551},
    {"argb_1555", ALLEGRO_PIXEL_FORMAT_ARGB_1555},
    {"abgr_8888", ALLEGRO_PIXEL_FORMAT_ABGR_8888},
    {"xbgr_8888", ALLEGRO_PIXEL_FORMAT_XBGR_8888},
    {"bgr_888", ALLEGRO_PIXEL_FORMAT_BGR_888},
    {"bgr_565", ALLEGRO_PIXEL_FORMAT_BGR_565},
    {"bgr_555", ALLEGRO_PIXEL_FORMAT_BGR_555},
    {"rgbx_8888", ALLEGRO_PIXEL_FORMAT_RGBX_8888},
    {"xrgb_8888", ALLEGRO_PIXEL_FORMAT_XRGB_8888},
    {"abgr_f32", ALLEGRO_PIXEL_FORMAT_ABGR_F32},
    {"abgr_8888_le", ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE},
    {"rgba_4444", ALLEGRO_PIXEL_FORMAT_RGBA_4444},
    {NULL, 0}
};

static const mapping_t bitmap_flag_mapping[] = {
    {"video_bitmap", ALLEGRO_VIDEO_BITMAP},
    {"memory_bitmap", ALLEGRO_MEMORY_BITMAP},
    {"keep_bitmap_format", ALLEGRO_KEEP_BITMAP_FORMAT},
    {"force_locking", ALLEGRO_FORCE_LOCKING},
    {"no_preserve_texture", ALLEGRO_NO_PRESERVE_TEXTURE},
    {"alpha_test", ALLEGRO_ALPHA_TEST},
    {"min_linear", ALLEGRO_MIN_LINEAR},
    {"mag_linear", ALLEGRO_MAG_LINEAR},
    {"mipmap", ALLEGRO_MIPMAP},
    {"no_premultiplied_alpha", ALLEGRO_NO_PREMULTIPLIED_ALPHA},
    {NULL, 0}
};

static const mapping_t draw_bitmap_mapping[] = {
    {"flip_horizontal", ALLEGRO_FLIP_HORIZONTAL},
    {"flip_vertical", ALLEGRO_FLIP_VERTICAL},
    {NULL, 0}
};

static const mapping_t blender_op_mapping[] = {
    {"add", ALLEGRO_ADD},
    {"dest_minus_src", ALLEGRO_DEST_MINUS_SRC},
    {"src_minus_dest", ALLEGRO_SRC_MINUS_DEST},
    {NULL, 0}
};

static const mapping_t blender_arg_mapping[] = {
    {"zero", ALLEGRO_ZERO},
    {"one", ALLEGRO_ONE},
    {"alpha", ALLEGRO_ALPHA},
    {"inverse_alpha", ALLEGRO_INVERSE_ALPHA},
    {"src_color", ALLEGRO_SRC_COLOR},
    {"dest_color", ALLEGRO_DEST_COLOR},
    {"inverse_src_color", ALLEGRO_INVERSE_SRC_COLOR},
    {"inverse_dest_color", ALLEGRO_INVERSE_DEST_COLOR},
    {NULL, 0}
};

static const mapping_t state_flag_mapping[] = {
    {"new_display_parameters", ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS},
    {"new_bitmap_parameters", ALLEGRO_STATE_NEW_BITMAP_PARAMETERS},
    {"display", ALLEGRO_STATE_DISPLAY},
    {"target_bitmap", ALLEGRO_STATE_TARGET_BITMAP},
    {"blender", ALLEGRO_STATE_BLENDER},
    {"transform", ALLEGRO_STATE_TRANSFORM},
    {"new_file_interface", ALLEGRO_STATE_NEW_FILE_INTERFACE},
    {"bitmap", ALLEGRO_STATE_BITMAP},
    {"target_bitmap", ALLEGRO_STATE_TARGET_BITMAP},
    {"all", ALLEGRO_STATE_ALL},
    {NULL, 0}
};

static const mapping_t standard_path_mapping[] = {
    {"resources", ALLEGRO_RESOURCES_PATH},
    {"temp", ALLEGRO_TEMP_PATH},
    {"user_home", ALLEGRO_USER_HOME_PATH},
    {"user_documents", ALLEGRO_USER_DOCUMENTS_PATH},
    {"user_data", ALLEGRO_USER_DATA_PATH},
    {"user_settings", ALLEGRO_USER_SETTINGS_PATH},
    {"exename", ALLEGRO_EXENAME_PATH},
    {NULL, 0}
};

static const mapping_t audio_depth_mapping[] = {
    {"int8", ALLEGRO_AUDIO_DEPTH_INT8},
    {"int16", ALLEGRO_AUDIO_DEPTH_INT16},
    {"int24", ALLEGRO_AUDIO_DEPTH_INT24},
    {"float32", ALLEGRO_AUDIO_DEPTH_FLOAT32},
    {"unsigned", ALLEGRO_AUDIO_DEPTH_UNSIGNED},
    {"uint8", ALLEGRO_AUDIO_DEPTH_UINT8},
    {"uint16", ALLEGRO_AUDIO_DEPTH_UINT16},
    {"uint24", ALLEGRO_AUDIO_DEPTH_UINT24},
    {NULL, 0}
};

static const mapping_t channel_conf_mapping[] = {
    {"1", ALLEGRO_CHANNEL_CONF_1},
    {"2", ALLEGRO_CHANNEL_CONF_2},
    {"3", ALLEGRO_CHANNEL_CONF_3},
    {"4", ALLEGRO_CHANNEL_CONF_4},
    {"5_1", ALLEGRO_CHANNEL_CONF_5_1},
    {"6_1", ALLEGRO_CHANNEL_CONF_6_1},
    {"7_1", ALLEGRO_CHANNEL_CONF_7_1},
    {NULL, 0}
};

static const mapping_t mixer_quality_mapping[] = {
    {"point", ALLEGRO_MIXER_QUALITY_POINT},
    {"linear", ALLEGRO_MIXER_QUALITY_LINEAR},
    {"cubic", ALLEGRO_MIXER_QUALITY_CUBIC},
    {NULL, 0}
};

static const mapping_t playmode_mapping[] = {
    {"once", ALLEGRO_PLAYMODE_ONCE},
    {"loop", ALLEGRO_PLAYMODE_LOOP},
    {"bidir", ALLEGRO_PLAYMODE_BIDIR},
    {NULL, 0}
};

static const mapping_t draw_text_mapping[] = {
    {"left", ALLEGRO_ALIGN_LEFT},
    {"right", ALLEGRO_ALIGN_RIGHT},
    {"centre", ALLEGRO_ALIGN_CENTRE},
    {"integer", ALLEGRO_ALIGN_INTEGER},
    {NULL, 0}
};

static const mapping_t ttf_flag_mapping[] = {
    {"no_kerning", ALLEGRO_TTF_NO_KERNING},
    {"monochrome", ALLEGRO_TTF_MONOCHROME},
    {"no_authohint", ALLEGRO_TTF_NO_AUTOHINT},
    {NULL, 0}
};

/*
================================================================================

                LUA HELPERS

================================================================================
*/
int global_object_table_ref = LUA_NOREF;

static int push_error( lua_State *L, const char *fmt, ... ) {
    va_list va;
    va_start(va, fmt);
    lua_pushnil(L);
    lua_pushvfstring(L, fmt, va);
    va_end(va);
    return 2;
}

static void *push_data( lua_State *L, const char *name, const size_t size ) {
    void *data = lua_newuserdata(L, size);
    if ( data == NULL ) {
        luaL_error(L, "cannot allocate space for " LUA_QS, name);
    }
    luaL_setmetatable(L, name);
    return data;
}

static int push_object_with_dependency( lua_State *L, const char *name, void *ptr, const int destroy, const int dependency ) {
    object_t *obj;
    if ( ptr ) {
        printf("new object: %s (%p)\n", name, ptr);
        obj = (object_t*) push_data(L, name, sizeof(object_t));
        obj->ptr = ptr;
        obj->name = name;
        obj->destroy = destroy;
        if ( dependency ) {
            lua_pushvalue(L, dependency);
            obj->dependency_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
            obj->dependency_ref = LUA_NOREF;
        }
        lua_rawgeti(L, LUA_REGISTRYINDEX, global_object_table_ref); /* register new object in weak-ref table */
        lua_pushlightuserdata(L, ptr);
        lua_pushvalue(L, -3);
        lua_settable(L, -3);
        lua_pop(L, 1);
        return 1;
    } else {
        return push_error(L, "cannot create object " LUA_QS, name);
    }
}

static int push_object( lua_State *L, const char *name, void *ptr, const int destroy ) {
    return push_object_with_dependency(L, name, ptr, destroy, 0);
}

static void *to_object( lua_State *L, const int idx, const char *name ) {
    object_t *obj = (object_t*) luaL_checkudata(L, idx, name);
    if ( obj->ptr == NULL ) {
        luaL_error(L, "attempt to operate on destroyed " LUA_QS, name);
    }
    return obj->ptr;
}

static void *to_object_gc( lua_State *L, const int idx, const char *name ) {
    object_t *obj = (object_t*) luaL_checkudata(L, idx, name);
    if ( obj->ptr && obj->destroy ) {
        return obj->ptr;
    } else {
        return NULL;
    }
}

/* WARNING: this function does no sanity check!! Use it only after to_object/to_object_gc */
static void clear_object( lua_State *L, const int idx ) {
    object_t *obj = (object_t*) lua_touserdata(L, idx);
    printf("clear object: %s (%p)\n", obj->name, obj->ptr);
    luaL_unref(L, LUA_REGISTRYINDEX, obj->dependency_ref);
    obj->ptr = NULL;
    obj->destroy = 0;
    obj->dependency_ref = LUA_NOREF;
}

static void create_meta( lua_State *L, const char *name, const luaL_Reg funcs[] ) {
    luaL_newmetatable(L, name);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, funcs, 0);
    lua_pop(L, 1);
}

static int parse_flag_table( lua_State *L, const int idx, const mapping_t mapping[] ) {
    int i, flags = 0;
    luaL_checktype(L, idx, LUA_TTABLE);
    for ( i = 0; mapping[i].name; ++i ) {
        lua_getfield(L, idx, mapping[i].name);
        if ( ! lua_isnil(L, -1) ) {
            if ( lua_toboolean(L, -1) ) {
                flags |= mapping[i].value;
            }
        }
        lua_pop(L, 1);
    }
    return flags;
}

static int parse_opt_flag_table( lua_State *L, const int idx, const mapping_t mapping[], const int flags ) {
    if ( lua_istable(L, idx) ) {
        return parse_flag_table(L, idx, mapping);
    } else {
        return flags;
    }
}

static int push_flag_table( lua_State *L, const int flags, const mapping_t mapping[] ) {
    int i;
    lua_newtable(L);
    for ( i = 0; mapping[i].name; ++i ) {
        lua_pushboolean(L, flags & mapping[i].value);
        lua_setfield(L, -2, mapping[i].name);
    }
    return 1;
}

static int parse_enum_name( lua_State *L, const int idx, const mapping_t mapping[] ) {
    int i;
    const char *name = luaL_checkstring(L, idx);
    for ( i = 0; mapping[i].name; ++i ) {
        if ( strcmp(name, mapping[i].name) == 0 ) {
            return mapping[i].value;
        }
    }
    luaL_argerror(L, idx, "invalid enum");
    return 0;
}

static int push_enum_name( lua_State *L, const int value, const mapping_t mapping[] ) {
    int i;
    for ( i = 0; mapping[i].name; ++i ) {
        if ( value == mapping[i].value ) {
            lua_pushstring(L, mapping[i].name);
            return 1;
        }
    }
    return 0;
}

static void set_str( lua_State *L, const char *key, const char *value ) {
    lua_pushstring(L, value);
    lua_setfield(L, -2, key);
}

static void set_int( lua_State *L, const char *key, const lua_Integer value ) {
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}

static void set_ptr( lua_State *L, const char *key, const void *ptr ) {
    lua_pushlightuserdata(L, (void*) ptr);
    lua_setfield(L, -2, key);
}

static void set_bool( lua_State *L, const char *key, const int value ) {
    lua_pushboolean(L, value);
    lua_setfield(L, -2, key);
}

/*
static int push_equal_check( lua_State *L, const char *name ) {
    object_t *ob1, *ob2;
    ob1 = (object_t*) luaL_testudata(L, 1, name);
    ob2 = (object_t*) luaL_testudata(L, 2, name);
    if ( ob1 && ob2 ) {
        if ( ob1->ptr || ob2->ptr ) {
            lua_pushboolean(L, ob1->ptr == ob2->ptr);
        } else {
            lua_pushboolean(L, 0);
        }
    } else {
        lua_pushboolean(L, 0);
    }
    return 1;
}
*/

static void create_object_table( lua_State *L ) {
    /* create weak-value table for all created objects */
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
    global_object_table_ref = luaL_ref(L, LUA_REGISTRYINDEX);
}

static int push_object_by_pointer( lua_State *L, const char *name, void *ptr ) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, global_object_table_ref);
    lua_pushlightuserdata(L, ptr);
    lua_gettable(L, -2);
    lua_remove(L, -2);
    if ( lua_isnil(L, -1) ) {
        lua_pop(L, 1);
        return push_object(L, name, ptr, 0);
    } else {
        return 1;
    }
}

/*
================================================================================

                ALLEGRO 5 - FUNCTIONS

================================================================================
*/
/*
================================================================================

                Generic functions

================================================================================
*/
static int lg_get_version( lua_State *L ) {
    lua_pushinteger(L, LEGATO_VERSION_MAJOR);
    lua_pushinteger(L, LEGATO_VERSION_MINOR);
    lua_pushinteger(L, LEGATO_VERSION_PATCH);
    return 3;
}

static const char *lg_load_script_callback( lua_State *L, void *data, size_t *size ) {
    static char buffer[4096];
    PHYSFS_sint64 read_bytes = PHYSFS_read((PHYSFS_File*)data, buffer, 1, sizeof(buffer));
    if ( read_bytes > 0 ) {
        *size = (size_t) read_bytes;
        return buffer;
    } else {
        *size = 0L;
        return NULL;
    }
}

static int lg_load_script( lua_State *L ) {
    int success;
    PHYSFS_File *fp = PHYSFS_openRead(luaL_checkstring(L, 1));
    if ( fp == NULL ) {
        luaL_error(L, "cannot load lua script " LUA_QS, lua_tostring(L, 1));
    }
    lua_pushfstring(L, "@%s", lua_tostring(L, 1));
    success = lua_load(L, lg_load_script_callback, fp, lua_tostring(L, -1), NULL);
    PHYSFS_close(fp);
    lua_remove(L, -2);
    if ( success == LUA_OK ) {
        return 1;
    } else {
        lua_error(L);
        return 0;
    }
}

static int lg_encode_UTF8_codepoint( lua_State *L ) {
    char utf8str[4];
    size_t utf8size;
    utf8size = al_utf8_encode(utf8str, luaL_checkint(L, 1));
    lua_pushlstring(L, utf8str, utf8size);
    return 1;
}

static int lg_get_UTF8_length( lua_State *L ) {
    size_t size;
    ALLEGRO_USTR_INFO info;
    const char *data = luaL_checklstring(L, 1, &size);
    lua_pushinteger(L, al_ustr_length(al_ref_buffer(&info, data, size)));
    return 1;
}

/*
================================================================================

                Configuration files

================================================================================
*/
static int lg_create_config( lua_State *L ) {
    return push_object(L, LEGATO_CONFIG, al_create_config(), 1);
}

static int lg_destroy_config( lua_State *L ) {
    ALLEGRO_CONFIG *config = (ALLEGRO_CONFIG*) to_object_gc(L, 1, LEGATO_CONFIG);
    if ( config ) {
        al_destroy_config(config);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_load_config_file( lua_State *L ) {
    return push_object(L, LEGATO_CONFIG, al_load_config_file(luaL_checkstring(L, 1)), 1);
}

static int lg_save_config_file( lua_State *L ) {
    lua_pushboolean(L, al_save_config_file(luaL_checkstring(L, 2), to_config(L, 1)));
    return 1;
}

static int lg_add_config_section( lua_State *L ) {
    al_add_config_section(to_config(L, 1), luaL_checkstring(L, 2));
    return 0;
}

static int lg_add_config_comment( lua_State *L ) {
    al_add_config_comment(to_config(L, 1), luaL_checkstring(L, 2), luaL_checkstring(L, 3));
    return 0;
}

static int lg_get_config_value( lua_State *L ) {
    lua_pushstring(L, al_get_config_value(to_config(L, 1), luaL_checkstring(L, 2), luaL_checkstring(L, 3)));
    return 1;
}

static int lg_set_config_value( lua_State *L ) {
    al_set_config_value(to_config(L, 1), luaL_checkstring(L, 2), luaL_checkstring(L, 3), luaL_checkstring(L, 4));
    return 0;
}

static int lg_get_config_sections( lua_State *L ) {
    ALLEGRO_CONFIG *config;
    ALLEGRO_CONFIG_SECTION *iterator;
    const char *section;
    int i;
    config = to_config(L, 1);
    lua_newtable(L);
    section = al_get_first_config_section(config, &iterator);
    for ( i = 1; section; ++i ) {
        lua_pushstring(L, section);
        lua_rawseti(L, -2, i);
        section = al_get_next_config_section(&iterator);
    }
    return 1;
}

static int lg_get_config_entries( lua_State *L ) {
    ALLEGRO_CONFIG *config;
    ALLEGRO_CONFIG_ENTRY *iterator;
    const char *section, *key;
    config = to_config(L, 1);
    section = luaL_optstring(L, 2, "");
    lua_newtable(L);
    key = al_get_first_config_entry(config, section, &iterator);
    while ( key ) {
        lua_pushstring(L, al_get_config_value(config, section, key));
        lua_setfield(L, -2, key);
        key = al_get_next_config_entry(&iterator);
    }
    return 1;
}

static int lg_merge_config( lua_State *L ) {
    return push_object(L, LEGATO_CONFIG, al_merge_config(to_config(L, 1), to_config(L, 2)), 1);
}

static int lg_merge_config_into( lua_State *L ) {
    al_merge_config_into(to_config(L, 1), to_config(L, 2));
    return 0;
}

/*
================================================================================

                Display

================================================================================
*/
static int lg_create_display( lua_State *L ) {
    return push_object(L, LEGATO_DISPLAY, al_create_display(luaL_checkint(L, 1), luaL_checkint(L, 2)), 1);
}

static int lg_destroy_display( lua_State *L ) {
    ALLEGRO_DISPLAY *display = (ALLEGRO_DISPLAY*) to_object_gc(L, 1, LEGATO_DISPLAY);
    if ( display ) {
        al_destroy_display(display);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_get_new_display_flags( lua_State *L ) {
    return push_flag_table(L, al_get_new_display_flags(), display_flag_mapping);
}

static int lg_set_new_display_flags( lua_State *L ) {
    al_set_new_display_flags(parse_flag_table(L, 1, display_flag_mapping));
    return 0;
}

static int lg_get_new_display_option( lua_State *L ) {
    int importance;
    lua_pushinteger(L, al_get_new_display_option(parse_enum_name(L, 1, display_option_mapping), &importance));
    push_enum_name(L, importance, display_importance_mapping);
    return 2;
}

static int lg_set_new_display_option( lua_State *L ) {
    al_set_new_display_option(parse_enum_name(L, 1, display_option_mapping),
            luaL_checkint(L, 2), parse_enum_name(L, 3, display_importance_mapping));
    return 0;
}

static int lg_reset_new_display_options( lua_State *L ) {
    al_reset_new_display_options();
    return 0;
}

static int lg_get_new_window_position( lua_State *L ) {
    int x, y;
    al_get_new_window_position(&x, &y);
    lua_pushinteger(L, x); lua_pushinteger(L, y);
    return 2;
}

static int lg_set_new_window_position( lua_State *L ) {
    al_set_new_window_position(luaL_optint(L, 1, INT_MAX), luaL_optint(L, 2, INT_MAX));
    return 0;
}

static int lg_get_new_display_refresh_rate( lua_State *L ) {
    lua_pushinteger(L, al_get_new_display_refresh_rate());
    return 1;
}

static int lg_set_new_display_refresh_rate( lua_State *L ) {
    al_set_new_display_refresh_rate(luaL_checkint(L, 1));
    return 0;
}

static int lg_get_backbuffer( lua_State *L ) {
    return push_object_by_pointer(L, LEGATO_BITMAP, al_get_backbuffer(to_display(L, 1)));
}

static int lg_flip_display( lua_State *L ) {
    al_flip_display();
    return 0;
}

static int lg_update_display_region( lua_State *L ) {
    al_update_display_region(luaL_checkint(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4));
    return 0;
}

static int lg_wait_for_vsync( lua_State *L ) {
    lua_pushboolean(L, al_wait_for_vsync());
    return 1;
}

static int lg_get_display_width( lua_State *L ) {
    lua_pushinteger(L, al_get_display_width(to_display(L, 1)));
    return 1;
}

static int lg_get_display_height( lua_State *L ) {
    lua_pushinteger(L, al_get_display_height(to_display(L, 1)));
    return 1;
}

static int lg_get_display_size( lua_State *L ) {
    ALLEGRO_DISPLAY *display = to_display(L, 1);
    lua_pushinteger(L, al_get_display_width(display));
    lua_pushinteger(L, al_get_display_height(display));
    return 2;
}

static int lg_resize_display( lua_State *L ) {
    lua_pushboolean(L, al_resize_display(to_display(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3)));
    return 1;
}

static int lg_acknowledge_resize( lua_State *L ) {
    lua_pushboolean(L, al_acknowledge_resize(to_display(L, 1)));
    return 1;
}

static int lg_get_window_position( lua_State *L ) {
    int x, y;
    al_get_window_position(to_display(L, 1), &x, &y);
    lua_pushinteger(L, x); lua_pushinteger(L, y);
    return 2;
}

static int lg_set_window_position( lua_State *L ) {
    al_set_window_position(to_display(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3));
    return 0;
}

static int lg_get_display_flags( lua_State *L ) {
    return push_flag_table(L, al_get_display_flags(to_display(L, 1)), display_flag_mapping);
}

static int lg_set_display_flag( lua_State *L ) {
    luaL_checktype(L, 3, LUA_TBOOLEAN);
    lua_pushboolean(L, al_set_display_flag(to_display(L, 1), parse_enum_name(L, 2, display_flag_mapping), lua_toboolean(L, 3)));
    return 1;
}

static int lg_get_display_option( lua_State *L ) {
    lua_pushinteger(L, al_get_display_option(to_display(L, 1), parse_enum_name(L, 2, display_option_mapping)));
    return 1;
}

static int lg_get_display_format( lua_State *L ) {
    return push_enum_name(L, al_get_display_format(to_display(L, 1)), pixel_format_mapping);
}

static int lg_get_display_refresh_rate( lua_State *L ) {
    lua_pushinteger(L, al_get_display_refresh_rate(to_display(L, 1)));
    return 1;
}

static int lg_set_window_title( lua_State *L ) {
    al_set_window_title(to_display(L, 1), luaL_checkstring(L, 2));
    return 0;
}

static int lg_set_display_icon( lua_State *L ) {
    al_set_display_icon(to_display(L, 1), to_bitmap(L, 2));
    return 0;
}

static int lg_set_display_icons( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_inhibit_screensaver( lua_State *L ) {
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    lua_pushboolean(L, al_inhibit_screensaver(lua_toboolean(L, 1)));
    return 0;
}

/*
================================================================================

                Graphics

================================================================================
*/
static int push_color( lua_State *L, ALLEGRO_COLOR color ) {
    ALLEGRO_COLOR *obj = (ALLEGRO_COLOR*) push_data(L, LEGATO_COLOR, sizeof(ALLEGRO_COLOR));
    *obj = color;
    return 1;
}

static int lg_map_rgb( lua_State *L ) {
    return push_color(L, al_map_rgba(luaL_checkint(L, 1), luaL_checkint(L, 2),
                luaL_checkint(L, 3), luaL_optint(L, 4, 255)));
}

static int lg_map_rgb_f( lua_State *L ) {
    return push_color(L, al_map_rgba_f(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
                luaL_checknumber(L, 3), luaL_optnumber(L, 4, 1.0)));
}

static int lg_unmap_rgb( lua_State *L ) {
    unsigned char r, g, b, a;
    al_unmap_rgba(to_color(L, 1), &r, &g, &b, &a);
    lua_pushinteger(L, r); lua_pushinteger(L, g);
    lua_pushinteger(L, b); lua_pushinteger(L, a);
    return 4;
}

static int lg_unmap_rgb_f( lua_State *L ) {
    float r, g, b, a;
    al_unmap_rgba_f(to_color(L, 1), &r, &g, &b, &a);
    lua_pushnumber(L, r); lua_pushnumber(L, g);
    lua_pushnumber(L, b); lua_pushnumber(L, a);
    return 4;
}

static int lg_get_pixel_size( lua_State *L ) {
    lua_pushinteger(L, al_get_pixel_size(parse_enum_name(L, 1, pixel_format_mapping)));
    return 1;
}

static int lg_get_pixel_format_bits( lua_State *L ) {
    lua_pushinteger(L, al_get_pixel_format_bits(parse_enum_name(L, 1, pixel_format_mapping)));
    return 1;
}

static int lg_lock_bitmap( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_lock_bitmap_region( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_unlock_bitmap( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_create_bitmap( lua_State *L ) {
    return push_object(L, LEGATO_BITMAP, al_create_bitmap(luaL_checkint(L, 1), luaL_checkint(L, 2)), 1);
}

static int lg_create_sub_bitmap( lua_State *L ) {
    return push_object_with_dependency(L, LEGATO_BITMAP, al_create_sub_bitmap(to_bitmap(L, 1),
                luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4), luaL_checkint(L, 5)), 1, 1);
}

static int lg_clone_bitmap( lua_State *L ) {
    return push_object(L, LEGATO_BITMAP, al_clone_bitmap(to_bitmap(L, 1)), 1);
}

static int lg_destroy_bitmap( lua_State *L ) {
    ALLEGRO_BITMAP *bitmap = (ALLEGRO_BITMAP*) to_object_gc(L, 1, LEGATO_BITMAP);
    if ( bitmap ) {
        al_destroy_bitmap(bitmap);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_get_new_bitmap_flags( lua_State *L ) {
    return push_flag_table(L, al_get_new_bitmap_flags(), bitmap_flag_mapping);
}

static int lg_get_new_bitmap_format( lua_State *L ) {
    return push_enum_name(L, al_get_new_bitmap_format(), pixel_format_mapping);
}

static int lg_set_new_bitmap_flags( lua_State *L ) {
    al_set_new_bitmap_flags(parse_flag_table(L, 1, bitmap_flag_mapping));
    return 0;
}

static int lg_add_new_bitmap_flag( lua_State *L ) {
    al_add_new_bitmap_flag(parse_enum_name(L, 1, bitmap_flag_mapping));
    return 0;
}

static int lg_set_new_bitmap_format( lua_State *L ) {
    al_set_new_bitmap_format(parse_enum_name(L, 1, pixel_format_mapping));
    return 0;
}

static int lg_get_bitmap_flags( lua_State *L ) {
    return push_flag_table(L, al_get_bitmap_flags(to_bitmap(L, 1)), bitmap_flag_mapping);
}

static int lg_get_bitmap_format( lua_State *L ) {
    return push_enum_name(L, al_get_bitmap_format(to_bitmap(L, 1)), pixel_format_mapping);
}

static int lg_get_bitmap_height( lua_State *L ) {
    lua_pushinteger(L, al_get_bitmap_height(to_bitmap(L, 1)));
    return 1;
}

static int lg_get_bitmap_width( lua_State *L ) {
    lua_pushinteger(L, al_get_bitmap_width(to_bitmap(L, 1)));
    return 1;
}

static int lg_get_bitmap_size( lua_State *L ) {
    ALLEGRO_BITMAP *bitmap = to_bitmap(L, 1);
    lua_pushinteger(L, al_get_bitmap_width(bitmap));
    lua_pushinteger(L, al_get_bitmap_height(bitmap));
    return 2;
}

static int lg_get_pixel( lua_State *L ) {
    return push_color(L, al_get_pixel(to_bitmap(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3)));
}

static int lg_is_bitmap_locked( lua_State *L ) {
    lua_pushboolean(L, al_is_bitmap_locked(to_bitmap(L, 1)));
    return 1;
}

static int lg_is_compatible_bitmap( lua_State *L ) {
    lua_pushboolean(L, al_is_compatible_bitmap(to_bitmap(L, 1)));
    return 1;
}

static int lg_is_sub_bitmap( lua_State *L ) {
    lua_pushboolean(L, al_is_sub_bitmap(to_bitmap(L, 1)));
    return 1;
}

static int lg_get_parent_bitmap( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_clear_to_color( lua_State *L ) {
    al_clear_to_color(to_color(L, 1));
    return 0;
}

static int get_draw_bitmap_flags( lua_State *L, const int idx ) {
    return parse_opt_flag_table(L, idx, draw_bitmap_mapping, 0);
}

static int lg_draw_bitmap( lua_State *L ) {
    al_draw_bitmap(to_bitmap(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), get_draw_bitmap_flags(L, 4));
    return 0;
}

static int lg_draw_tinted_bitmap( lua_State *L ) {
    al_draw_tinted_bitmap(to_bitmap(L, 1), to_color(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4), get_draw_bitmap_flags(L, 5));
    return 0;
}

static int lg_draw_bitmap_region( lua_State *L ) {
    al_draw_bitmap_region(to_bitmap(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3),
            luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6), luaL_checknumber(L, 7), get_draw_bitmap_flags(L, 8));
    return 0;
}

static int lg_draw_tinted_bitmap_region( lua_State *L ) {
    al_draw_tinted_bitmap_region(to_bitmap(L, 1), to_color(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4),
            luaL_checknumber(L, 5), luaL_checknumber(L, 6), luaL_checknumber(L, 7), luaL_checknumber(L, 8), get_draw_bitmap_flags(L, 9));
    return 0;
}

static int lg_draw_pixel( lua_State *L ) {
    al_draw_pixel(luaL_checknumber(L, 1), luaL_checknumber(L, 2), to_color(L, 3));
    return 0;
}

static int lg_draw_rotated_bitmap( lua_State *L ) {
    al_draw_rotated_bitmap(to_bitmap(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3),
            luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6), get_draw_bitmap_flags(L, 7));
    return 0;
}

static int lg_draw_tinted_rotated_bitmap( lua_State *L ) {
    al_draw_tinted_rotated_bitmap(to_bitmap(L, 1), to_color(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4),
            luaL_checknumber(L, 5), luaL_checknumber(L, 6), luaL_checknumber(L, 7), get_draw_bitmap_flags(L, 8));
    return 0;
}

static int lg_draw_scaled_rotated_bitmap( lua_State *L ) {
    al_draw_scaled_rotated_bitmap(to_bitmap(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3),
            luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6), luaL_checknumber(L, 7),
            luaL_checknumber(L, 8), get_draw_bitmap_flags(L, 9));
    return 0;
}

static int lg_draw_tinted_scaled_rotated_bitmap( lua_State *L ) {
    al_draw_tinted_scaled_rotated_bitmap(to_bitmap(L, 1), to_color(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6),
            luaL_checknumber(L, 7), luaL_checknumber(L, 8), luaL_checknumber(L, 9), get_draw_bitmap_flags(L, 10));
    return 0;
}

static int lg_draw_tinted_scaled_rotated_bitmap_region( lua_State *L ) {
    al_draw_tinted_scaled_rotated_bitmap_region(to_bitmap(L, 1),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6),
            to_color(L, 2),
            luaL_checknumber(L, 7), luaL_checknumber(L, 8), luaL_checknumber(L, 9), luaL_checknumber(L, 10),
            luaL_checknumber(L, 11), luaL_checknumber(L, 12), luaL_checknumber(L, 13), get_draw_bitmap_flags(L, 14));
    return 0;
}

static int lg_draw_scaled_bitmap( lua_State *L ) {
    al_draw_scaled_bitmap(to_bitmap(L, 1),
            luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5),
            luaL_checknumber(L, 6), luaL_checknumber(L, 7), luaL_checknumber(L, 8), luaL_checknumber(L, 9),
            get_draw_bitmap_flags(L, 10));
    return 0;
}

static int lg_draw_tinted_scaled_bitmap( lua_State *L ) {
    al_draw_tinted_scaled_bitmap(to_bitmap(L, 1), to_color(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6),
            luaL_checknumber(L, 7), luaL_checknumber(L, 8), luaL_checknumber(L, 9), luaL_checknumber(L, 10),
            get_draw_bitmap_flags(L, 11));
    return 0;
}

static int lg_get_target_bitmap( lua_State *L ) {
    return push_object_by_pointer(L, LEGATO_BITMAP, al_get_target_bitmap());
}

static int lg_put_pixel( lua_State *L ) {
    al_put_pixel(luaL_checkint(L, 1), luaL_checkint(L, 2), to_color(L, 3));
    return 0;
}

static int lg_put_blended_pixel( lua_State *L ) {
    al_put_blended_pixel(luaL_checkint(L, 1), luaL_checkint(L, 3), to_color(L, 3));
    return 0;
}

static int lg_set_target_bitmap( lua_State *L ) {
    al_set_target_bitmap(to_bitmap(L, 1));
    return 0;
}

static int lg_set_target_backbuffer( lua_State *L ) {
    al_set_target_backbuffer(to_display(L, 1));
    return 0;
}

static int lg_get_current_display( lua_State *L ) {
    return push_object_by_pointer(L, LEGATO_DISPLAY, al_get_current_display());
}

static int lg_get_blender( lua_State *L ) {
    int op, src, dst;
    al_get_blender(&op, &src, &dst);
    push_enum_name(L, op, blender_op_mapping); push_enum_name(L, src, blender_arg_mapping); push_enum_name(L, dst, blender_arg_mapping);
    return 3;
}

static int lg_get_separate_blender( lua_State *L ) {
    int op, src, dst, alpha_op, alpha_src, alpha_dst;
    al_get_separate_blender(&op, &src, &dst, &alpha_op, &alpha_src, &alpha_dst);
    push_enum_name(L, op, blender_op_mapping); push_enum_name(L, src, blender_arg_mapping); push_enum_name(L, dst, blender_arg_mapping);
    push_enum_name(L, alpha_op, blender_op_mapping); push_enum_name(L, alpha_src, blender_arg_mapping); push_enum_name(L, alpha_dst, blender_arg_mapping);
    return 6;
}

static int lg_set_blender( lua_State *L ) {
    al_set_blender(parse_enum_name(L, 1, blender_op_mapping), parse_enum_name(L, 2, blender_arg_mapping), parse_enum_name(L, 3, blender_arg_mapping));
    return 0;
}

static int lg_set_separate_blender( lua_State *L ) {
    al_set_separate_blender(parse_enum_name(L, 1, blender_op_mapping), parse_enum_name(L, 2, blender_arg_mapping), parse_enum_name(L, 3, blender_arg_mapping),
            parse_enum_name(L, 4, blender_op_mapping), parse_enum_name(L, 5, blender_arg_mapping), parse_enum_name(L, 6, blender_arg_mapping));
    return 0;
}

static int lg_get_clipping_rectangle( lua_State *L ) {
    int x, y, w, h;
    al_get_clipping_rectangle(&x, &y, &w, &h);
    lua_pushinteger(L, x); lua_pushinteger(L, y);
    lua_pushinteger(L, w); lua_pushinteger(L, h);
    return 4;
}

static int lg_set_clipping_rectangle( lua_State *L ) {
    al_set_clipping_rectangle(luaL_checkint(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3), luaL_checkint(L, 4));
    return 0;
}

static int lg_reset_clipping_rectangle( lua_State *L ) {
    al_reset_clipping_rectangle();
    return 0;
}

static int lg_convert_mask_to_alpha( lua_State *L ) {
    al_convert_mask_to_alpha(to_bitmap(L, 1), to_color(L, 2));
    return 0;
}

static int lg_hold_bitmap_drawing( lua_State *L ) {
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    al_hold_bitmap_drawing(lua_toboolean(L, 1));
    return 0;
}

static int lg_is_bitmap_drawing_held( lua_State *L ) {
    lua_pushboolean(L, al_is_bitmap_drawing_held());
    return 1;
}

static int lg_load_bitmap( lua_State *L ) {
    return push_object(L, LEGATO_BITMAP, al_load_bitmap(luaL_checkstring(L, 1)), 1);
}

/*
================================================================================

                Events

================================================================================
*/
static int lg_create_event_queue( lua_State *L ) {
    return push_object(L, LEGATO_EVENT_QUEUE, al_create_event_queue(), 1);
}

static int lg_destroy_event_queue( lua_State *L ) {
    ALLEGRO_EVENT_QUEUE *event_queue = (ALLEGRO_EVENT_QUEUE*) to_object_gc(L, 1, LEGATO_EVENT_QUEUE);
    if ( event_queue ) {
        al_destroy_event_queue(event_queue);
        clear_object(L, 1);
    }
    return 0;
}

static ALLEGRO_EVENT_SOURCE *get_event_source( lua_State *L ) {
    ALLEGRO_EVENT_SOURCE *source = NULL;
    if ( luaL_testudata(L, 2, LEGATO_DISPLAY) ) {
        source = al_get_display_event_source(to_display(L, 2));
    } else if ( luaL_testudata(L, 2, LEGATO_KEYBOARD_STATE) ) {
        source = al_get_keyboard_event_source();
    } else if ( luaL_testudata(L, 2, LEGATO_MOUSE_STATE) ) {
        source = al_get_mouse_event_source();
    } else if ( luaL_testudata(L, 2, LEGATO_TIMER) ) {
        source = al_get_timer_event_source(to_timer(L, 2));
    }
    if ( source ) {
        return source;
    } else {
        luaL_error(L, "invalid event source given");
        return NULL;
    }
}

static int lg_register_event_source( lua_State *L ) {
    al_register_event_source(to_event_queue(L, 1), get_event_source(L));
    return 0;
}

static int lg_unregister_event_source( lua_State *L ) {
    al_unregister_event_source(to_event_queue(L, 1), get_event_source(L));
    return 0;
}

static int lg_is_event_queue_empty( lua_State *L ) {
    lua_pushboolean(L, al_is_event_queue_empty(to_event_queue(L, 1)));
    return 1;
}

static int push_event( lua_State *L, ALLEGRO_EVENT *event ) {
    lua_createtable(L, 0, 10); /* create big table to avoid many rehashed */
    switch ( event->type ) {
        case ALLEGRO_EVENT_KEY_DOWN:
            set_str(L, "type", "key_down");
            set_int(L, "keycode", event->keyboard.keycode);
            set_ptr(L, "display", event->keyboard.display);
            return 1;
        case ALLEGRO_EVENT_KEY_UP:
            set_str(L, "type", "key_up");
            set_int(L, "keycode", event->keyboard.keycode);
            set_ptr(L, "display", event->keyboard.display);
            return 1;
        case ALLEGRO_EVENT_KEY_CHAR:
            set_str(L, "type", "key_char");
            set_int(L, "keycode", event->keyboard.keycode);
            set_int(L, "unichar", event->keyboard.unichar);
            set_bool(L, "repeat", event->keyboard.repeat);
            set_ptr(L, "display", event->keyboard.display);
            return 1;
        case ALLEGRO_EVENT_MOUSE_AXES:
            set_str(L, "type", "mouse_axes");
            set_int(L, "x", event->mouse.x);
            set_int(L, "y", event->mouse.y);
            set_int(L, "z", event->mouse.z);
            set_int(L, "w", event->mouse.w);
            set_int(L, "dx", event->mouse.dx);
            set_int(L, "dy", event->mouse.dy);
            set_int(L, "dz", event->mouse.dz);
            set_int(L, "dw", event->mouse.dw);
            set_ptr(L, "display", event->mouse.display);
            return 1;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
            set_str(L, "type", "mouse_button_down");
            set_int(L, "x", event->mouse.x);
            set_int(L, "y", event->mouse.y);
            set_int(L, "z", event->mouse.z);
            set_int(L, "w", event->mouse.w);
            set_int(L, "button", event->mouse.button);
            set_ptr(L, "display", event->mouse.display);
            return 1;
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
            set_str(L, "type", "mouse_button_down");
            set_int(L, "x", event->mouse.x);
            set_int(L, "y", event->mouse.y);
            set_int(L, "z", event->mouse.z);
            set_int(L, "w", event->mouse.w);
            set_int(L, "button", event->mouse.button);
            set_ptr(L, "display", event->mouse.display);
            return 1;
        case ALLEGRO_EVENT_MOUSE_WARPED:
            set_str(L, "type", "mouse_warped");
            return 1;
        case ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY:
            set_str(L, "type", "mouse_enter_display");
            set_int(L, "x", event->mouse.x);
            set_int(L, "y", event->mouse.y);
            set_int(L, "z", event->mouse.z);
            set_int(L, "w", event->mouse.w);
            set_ptr(L, "display", event->mouse.display);
            return 1;
        case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
            set_str(L, "type", "mouse_leave_display");
            set_int(L, "x", event->mouse.x);
            set_int(L, "y", event->mouse.y);
            set_int(L, "z", event->mouse.z);
            set_int(L, "w", event->mouse.w);
            set_ptr(L, "display", event->mouse.display);
            return 1;
        case ALLEGRO_EVENT_TIMER:
            set_str(L, "type", "timer");
            set_int(L, "count", event->timer.count);
            set_ptr(L, "timer", event->timer.source);
            return 1;
        case ALLEGRO_EVENT_DISPLAY_EXPOSE:
            set_str(L, "type", "display_expose");
            set_ptr(L, "display", event->display.source);
            set_int(L, "x", event->display.x);
            set_int(L, "y", event->display.y);
            set_int(L, "width", event->display.width);
            set_int(L, "height", event->display.height);
            return 1;
        case ALLEGRO_EVENT_DISPLAY_RESIZE:
            set_str(L, "type", "display_resize");
            set_ptr(L, "display", event->display.source);
            set_int(L, "x", event->display.x);
            set_int(L, "y", event->display.y);
            set_int(L, "width", event->display.width);
            set_int(L, "height", event->display.height);
            return 1;
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            set_str(L, "type", "display_close");
            set_ptr(L, "display", event->display.source);
            return 1;
        case ALLEGRO_EVENT_DISPLAY_LOST:
            set_str(L, "type", "display_lost");
            set_ptr(L, "display", event->display.source);
            return 1;
        case ALLEGRO_EVENT_DISPLAY_FOUND:
            set_str(L, "type", "display_found");
            set_ptr(L, "display", event->display.source);
            return 1;
        case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
            set_str(L, "type", "display_switch_out");
            set_ptr(L, "display", event->display.source);
            return 1;
        case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
            set_str(L, "type", "display_switch_in");
            set_ptr(L, "display", event->display.source);
            return 1;
        case ALLEGRO_EVENT_DISPLAY_ORIENTATION:
            set_str(L, "type", "display_orientation");
            set_ptr(L, "display", event->display.source);
            switch ( event->display.orientation ) {
                case ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES:
                    lua_pushliteral(L, "0"); break;
                case ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES:
                    lua_pushliteral(L, "90"); break;
                case ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES:
                    lua_pushliteral(L, "180"); break;
                case ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES:
                    lua_pushliteral(L, "270"); break;
                case ALLEGRO_DISPLAY_ORIENTATION_FACE_UP:
                    lua_pushliteral(L, "face_up"); break;
                case ALLEGRO_DISPLAY_ORIENTATION_FACE_DOWN:
                    lua_pushliteral(L, "face_down"); break;
                default:
                    lua_pushliteral(L, "unknown"); break;
            }
            lua_setfield(L, -2, "orientation");
            return 1;
        default:
            lua_pop(L, 1);
            return 0;
    }
}

static int lg_get_next_event( lua_State *L ) {
    ALLEGRO_EVENT event;
    if ( al_get_next_event(to_event_queue(L, 1), &event) ) {
        return push_event(L, &event);
    } else {
        return 0;
    }
}

static int lg_peek_next_event( lua_State *L ) {
    ALLEGRO_EVENT event;
    if ( al_peek_next_event(to_event_queue(L, 1), &event) ) {
        return push_event(L, &event);
    } else {
        return 0;
    }
}

static int lg_drop_next_event( lua_State *L ) {
    lua_pushboolean(L, al_drop_next_event(to_event_queue(L, 1)));
    return 1;
}

static int lg_flush_event_queue( lua_State *L ) {
    al_flush_event_queue(to_event_queue(L, 1));
    return 0;
}

static int lg_wait_for_event( lua_State *L ) {
    ALLEGRO_EVENT event;
    al_wait_for_event(to_event_queue(L, 1), &event);
    return push_event(L, &event);
}

static int lg_wait_for_event_timed( lua_State *L ) {
    ALLEGRO_EVENT event;
    al_wait_for_event_timed(to_event_queue(L, 1), &event, luaL_checknumber(L, 2));
    return push_event(L, &event);
}

static int lg_wait_for_event_until( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

/*
================================================================================

                Fullscreen modes

================================================================================
*/
static int lg_get_display_modes( lua_State *L ) {
    ALLEGRO_DISPLAY_MODE mode;
    int i, num_modes;
    num_modes = al_get_num_display_modes();
    lua_newtable(L);
    for ( i = 0; i < num_modes; ++i ) {
        al_get_display_mode(i, &mode);
        lua_newtable(L);
        set_int(L, "width", mode.width);
        set_int(L, "height", mode.height);
        set_int(L, "refresh_rate", mode.refresh_rate);
        set_int(L, "format", mode.format); /* FIXME: use str enums! */
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/*
================================================================================

                Keyboard

================================================================================
*/
static int lg_is_keyboard_installed( lua_State *L ) {
    lua_pushboolean(L, al_is_keyboard_installed());
    return 1;
}

static int lg_create_keyboard_state( lua_State *L ) {
    push_data(L, LEGATO_KEYBOARD_STATE, sizeof(ALLEGRO_KEYBOARD_STATE));
    return 1;
}

static int lg_get_keyboard_state( lua_State *L ) {
    al_get_keyboard_state(to_keyboard_state(L, 1));
    return 0;
}

static int lg_key_down( lua_State *L ) {
    lua_pushboolean(L, al_key_down(to_keyboard_state(L, 1), luaL_checkint(L, 2)));
    return 1;
}

static int lg_keycode_to_name( lua_State *L ) {
    lua_pushstring(L, al_keycode_to_name(luaL_checkint(L, 1)));
    return 1;
}

static int lg_set_keyboard_leds( lua_State *L ) {
    lua_pushboolean(L, al_set_keyboard_leds(luaL_checkint(L, 1)));
    return 1;
}

/*
================================================================================

                Monitor

================================================================================
*/
static int lg_get_new_display_adapter( lua_State *L ) {
    int id = al_get_new_display_adapter();
    if ( id == ALLEGRO_DEFAULT_DISPLAY_ADAPTER ) {
        return 0; 
    } else {
        lua_pushinteger(L, id + 1);
        return 1;
    }
}

static int lg_set_new_display_adapter( lua_State *L ) {
    al_set_new_display_adapter(luaL_optint(L, 1, ALLEGRO_DEFAULT_DISPLAY_ADAPTER + 1) - 1);
    return 0;
}

static int lg_get_monitor_info( lua_State *L ) {
    ALLEGRO_MONITOR_INFO monitor;
    int i, num_monitors;
    num_monitors = al_get_num_video_adapters();
    lua_newtable(L);
    for ( i = 0; i < num_monitors; ++i ) {
        al_get_monitor_info(i, &monitor);
        lua_newtable(L);
        set_int(L, "x1", monitor.x1);
        set_int(L, "y1", monitor.y1);
        set_int(L, "x2", monitor.x2);
        set_int(L, "y2", monitor.y2);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}

/*
================================================================================

                Mouse

================================================================================
*/
static int lg_is_mouse_installed( lua_State *L ) {
    lua_pushboolean(L, al_is_mouse_installed());
    return 1;
}

static int lg_get_mouse_num_axes( lua_State *L ) {
    lua_pushinteger(L, al_get_mouse_num_axes());
    return 1;
}

static int lg_get_mouse_num_buttons( lua_State *L ) {
    lua_pushinteger(L, al_get_mouse_num_buttons());
    return 1;
}

static int lg_create_mouse_state( lua_State *L) {
    push_data(L, LEGATO_MOUSE_STATE, sizeof(ALLEGRO_MOUSE_STATE));
    return 1;
}

static int lg_get_mouse_state( lua_State *L ) {
    al_get_mouse_state(to_mouse_state(L, 1));
    return 0;
}

static int lg_get_mouse_state_axis( lua_State *L ) {
    lua_pushinteger(L, al_get_mouse_state_axis(to_mouse_state(L, 1), luaL_checkint(L, 2)));
    return 1;
}

static int lg_mouse_button_down( lua_State *L ) {
    lua_pushboolean(L, al_mouse_button_down(to_mouse_state(L, 1), luaL_checkint(L, 2)));
    return 1;
}

static int lg_set_mouse_xy( lua_State *L ) {
    lua_pushboolean(L, al_set_mouse_xy(to_display(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3)));
    return 1;
}

static int lg_set_mouse_z( lua_State *L ) {
    lua_pushboolean(L, al_set_mouse_z(luaL_checkint(L, 1)));
    return 1;
}

static int lg_set_mouse_w( lua_State *L ) {
    lua_pushboolean(L, al_set_mouse_w(luaL_checkint(L, 1)));
    return 1;
}

static int lg_set_mouse_axis( lua_State *L ) {
    lua_pushboolean(L, al_set_mouse_axis(luaL_checkint(L, 1), luaL_checkint(L, 2)));
    return 1;
}

static int lg_create_mouse_cursor( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_destroy_mouse_cursor( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_set_mouse_cursor( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_set_system_mouse_cursor( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_get_mouse_cursor_position( lua_State *L ) {
    int x, y;
    if ( al_get_mouse_cursor_position(&x, &y) ) {
        lua_pushinteger(L, x); lua_pushinteger(L, y);
        return 2;
    } else {
        return 0;
    }
}

static int lg_hide_mouse_cursor( lua_State *L ) {
    lua_pushboolean(L, al_hide_mouse_cursor(to_display(L, 1)));
    return 1;
}

static int lg_show_mouse_cursor( lua_State *L ) {
    lua_pushboolean(L, al_show_mouse_cursor(to_display(L, 1)));
    return 1;
}

static int lg_grab_mouse( lua_State *L ) {
    lua_pushboolean(L, al_grab_mouse(to_display(L, 1)));
    return 1;
}

static int lg_ungrab_mouse( lua_State *L ) {
    lua_pushboolean(L, al_ungrab_mouse());
    return 1;
}

/*
================================================================================

                Path

================================================================================
*/
static int lg_create_path( lua_State *L ) {
    return push_object(L, LEGATO_PATH, al_create_path(luaL_checkstring(L, 1)), 1);
}

static int lg_create_path_for_directory( lua_State *L ) {
    return push_object(L, LEGATO_PATH, al_create_path_for_directory(luaL_checkstring(L, 1)), 1);
}

static int lg_destroy_path( lua_State *L ) {
    ALLEGRO_PATH *path = (ALLEGRO_PATH*) to_object_gc(L, 1, LEGATO_PATH);
    if ( path ) {
        al_destroy_path(path);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_clone_path( lua_State *L ) {
    return push_object(L, LEGATO_PATH, al_clone_path(to_path(L, 1)), 1);
}

static int lg_join_paths( lua_State *L ) {
    lua_pushboolean(L, al_join_paths(to_path(L, 1), to_path(L, 2)));
    return 1;
}

static int lg_rebase_path( lua_State *L ) {
    lua_pushboolean(L, al_rebase_path(to_path(L, 1), to_path(L, 2)));
    return 1;
}

static int lg_get_path_drive( lua_State *L ) {
    lua_pushstring(L, al_get_path_drive(to_path(L, 1)));
    return 1;
}

static int lg_get_path_num_components( lua_State *L ) {
    lua_pushinteger(L, al_get_path_num_components(to_path(L, 1)));
    return 1;
}

static int lg_get_path_components( lua_State *L ) {
    int i, num_components;
    ALLEGRO_PATH *path = to_path(L, 1);
    lua_newtable(L);
    num_components = al_get_path_num_components(path);
    for ( i = 0; i < num_components; ++i ) {
        lua_pushstring(L, al_get_path_component(path, i));
        lua_rawgeti(L, -2, i + 1);
    }
    return 1;
}

static int lg_get_path_tail( lua_State *L ) {
    lua_pushstring(L, al_get_path_tail(to_path(L, 1)));
    return 1;
}

static int lg_get_path_filename( lua_State *L ) {
    lua_pushstring(L, al_get_path_filename(to_path(L, 1)));
    return 1;
}

static int lg_get_path_basename( lua_State *L ) {
    lua_pushstring(L, al_get_path_basename(to_path(L, 1)));
    return 1;
}

static int lg_get_path_extension( lua_State *L ) {
    lua_pushstring(L, al_get_path_extension(to_path(L, 1)));
    return 1;
}

static int lg_set_path_drive( lua_State *L ) {
    al_set_path_drive(to_path(L, 1), luaL_checkstring(L, 2));
    return 1;
}

static int lg_append_path_component( lua_State *L ) {
    al_append_path_component(to_path(L, 1), luaL_checkstring(L, 2));
    return 0;
}

static int lg_insert_path_component( lua_State *L ) {
    al_insert_path_component(to_path(L, 1), luaL_checkint(L, 2), luaL_checkstring(L, 3));
    return 0;
}

static int lg_replace_path_component( lua_State *L ) {
    al_replace_path_component(to_path(L, 1), luaL_checkint(L, 2), luaL_checkstring(L, 3));
    return 0;
}

static int lg_remove_path_component( lua_State *L ) {
    al_remove_path_component(to_path(L, 1), luaL_checkint(L, 2));
    return 0;
}

static int lg_drop_path_tail( lua_State *L ) {
    al_drop_path_tail(to_path(L, 1));
    return 0;
}

static int lg_set_path_filename( lua_State *L ) {
    al_set_path_filename(to_path(L, 1), luaL_checkstring(L, 2));
    return 0;
}

static int lg_set_path_extension( lua_State *L ) {
    lua_pushboolean(L, al_set_path_extension(to_path(L, 1), luaL_checkstring(L, 2)));
    return 1;
}

static int lg_path_str( lua_State *L ) {
    static char default_delim[2] = { ALLEGRO_NATIVE_PATH_SEP, 0 };
    const char *delim = luaL_optstring(L, 2, default_delim);
    luaL_argcheck(L, strlen(delim) == 1, 2, "path delimiter must be one character");
    lua_pushstring(L, al_path_cstr(to_path(L, 1), delim[0]));
    return 1;
}

static int lg_make_path_canonical( lua_State *L ) {
    lua_pushboolean(L, al_make_path_canonical(to_path(L, 1)));
    return 1;
}

/*
================================================================================

                State

================================================================================
*/
static int lg_restore_state( lua_State *L ) {
    al_restore_state(to_state(L, 1));
    return 0;
}

static int lg_store_state( lua_State *L ) {
    al_store_state(
            (ALLEGRO_STATE*) push_data(L, LEGATO_STATE, sizeof(ALLEGRO_STATE)),
            parse_flag_table(L, 2, state_flag_mapping));
    return 1;
}

static int lg_get_errno( lua_State *L ) {
    lua_pushinteger(L, al_get_errno());
    return 1;
}

static int lg_set_errno( lua_State *L ) {
    al_set_errno(luaL_checkint(L, 1));
    return 0;
}


/*
================================================================================

                System

================================================================================
*/
static int lg_get_allegro_version( lua_State *L ) {
    uint32_t version = al_get_allegro_version();
    lua_pushinteger(L, version >> 24);
    lua_pushinteger(L, (version >> 16) & 255);
    lua_pushinteger(L, (version >> 8) & 255);
    lua_pushinteger(L, version & 255);
    return 4;
}

static int lg_get_standard_path( lua_State *L ) {
    ALLEGRO_PATH *path = al_get_standard_path(parse_enum_name(L, 1, standard_path_mapping));
    if ( path ) {
        lua_pushstring(L, al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP));
        al_destroy_path(path);
        return 1;
    } else {
        return push_error(L, "cannot get standard path");
    }
}

static int lg_set_exe_name( lua_State *L ) {
    al_set_exe_name(luaL_checkstring(L, 1));
    return 0;
}

static int lg_set_app_name( lua_State *L ) {
    al_set_app_name(luaL_checkstring(L, 1));
    return 0;
}

static int lg_set_org_name( lua_State *L ) {
    al_set_org_name(luaL_checkstring(L, 1));
    return 0;
}

static int lg_get_app_name( lua_State *L ) {
    lua_pushstring(L, al_get_app_name());
    return 1;
}

static int lg_get_org_name( lua_State *L ) {
    lua_pushstring(L, al_get_org_name());
    return 1;
}

static int lg_get_system_config( lua_State *L ) {
    return push_object(L, LEGATO_CONFIG, al_get_system_config(), 0);
}

/*
================================================================================

                Time

================================================================================
*/
static int lg_get_time( lua_State *L ) {
    lua_pushnumber(L, al_get_time());
    return 1;
}

static int lg_rest( lua_State *L ) {
    al_rest(luaL_checknumber(L, 1));
    return 0;
}

/*
================================================================================

                Timer

================================================================================
*/
static int lg_create_timer( lua_State *L ) {
    return push_object(L, LEGATO_TIMER, al_create_timer(luaL_checknumber(L, 1)), 1);
}

static int lg_start_timer( lua_State *L ) {
    al_start_timer(to_timer(L, 1));
    return 0;
}

static int lg_stop_timer( lua_State *L ) {
    al_stop_timer(to_timer(L, 1));
    return 0;
}

static int lg_get_timer_started( lua_State *L ) {
    lua_pushboolean(L, al_get_timer_started(to_timer(L, 1)));
    return 1;
}

static int lg_destroy_timer( lua_State *L ) {
    ALLEGRO_TIMER *timer = (ALLEGRO_TIMER*) to_object_gc(L, 1, LEGATO_TIMER);
    if ( timer ) {
        al_destroy_timer(timer);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_get_timer_count( lua_State *L ) {
    lua_pushinteger(L, al_get_timer_count(to_timer(L, 1)));
    return 1;
}

static int lg_set_timer_count( lua_State *L ) {
    al_set_timer_count(to_timer(L, 1), luaL_checkinteger(L, 2));
    return 0;
}

static int lg_add_timer_count( lua_State *L ) {
    al_add_timer_count(to_timer(L, 1), luaL_checkinteger(L, 2));
    return 0;
}

static int lg_get_timer_speed( lua_State *L ) {
    lua_pushnumber(L, al_get_timer_speed(to_timer(L, 1)));
    return 1;
}

static int lg_set_timer_speed( lua_State *L ) {
    al_set_timer_speed(to_timer(L, 1), luaL_checknumber(L, 2));
    return 0;
}

/*
================================================================================

                Transformations

================================================================================
*/
static int lg_create_transform( lua_State *L ) {
    al_identity_transform((ALLEGRO_TRANSFORM*) push_data(L, LEGATO_TRANSFORM, sizeof(ALLEGRO_TRANSFORM)));
    return 1;
}

static int lg_copy_transform( lua_State *L ) {
    al_copy_transform((ALLEGRO_TRANSFORM*) push_data(L, LEGATO_TRANSFORM, sizeof(ALLEGRO_TRANSFORM)), to_transform(L, 2));
    return 1;
}

static int lg_use_transform( lua_State *L ) {
    al_use_transform(to_transform(L, 1));
    return 0;
}

static int lg_get_current_transform( lua_State *L ) {
    al_copy_transform((ALLEGRO_TRANSFORM*) push_data(L, LEGATO_TRANSFORM, sizeof(ALLEGRO_TRANSFORM)), al_get_current_transform());
    return 1;
}

static int lg_invert_transform( lua_State *L ) {
    al_invert_transform(to_transform(L, 1));
    return 0;
}

static int lg_check_inverse( lua_State *L ) {
    lua_pushboolean(L, al_check_inverse(to_transform(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int lg_identity_transform( lua_State *L ) {
    al_identity_transform(to_transform(L, 1));
    return 0;
}

static int lg_build_transform( lua_State *L ) {
    ALLEGRO_TRANSFORM *transform = (ALLEGRO_TRANSFORM*) push_data(L, LEGATO_TRANSFORM, sizeof(ALLEGRO_TRANSFORM));
    al_build_transform(transform, luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5));
    return 1;
}

static int lg_translate_transform( lua_State *L ) {
    al_translate_transform(to_transform(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3));
    return 0;
}

static int lg_rotate_transform( lua_State *L ) {
    al_rotate_transform(to_transform(L, 1), luaL_checknumber(L, 2));
    return 0;
}

static int lg_scale_transform( lua_State *L ) {
    al_scale_transform(to_transform(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3));
    return 0;
}

static int lg_transform_coordinates( lua_State *L ) {
    float x, y;
    x = luaL_checknumber(L, 2); y = luaL_checknumber(L, 3);
    al_transform_coordinates(to_transform(L, 1), &x, &y);
    lua_pushnumber(L, x); lua_pushnumber(L, y);
    return 2;
}

static int lg_compose_transform( lua_State *L ) {
    al_compose_transform(to_transform(L, 1), to_transform(L, 2));
    return 0;
}

/*
================================================================================

                Audio addons

================================================================================
*/
static int lg_is_audio_installed( lua_State *L ) {
    lua_pushboolean(L, al_is_audio_installed());
    return 1;
}

static int lg_reserve_samples( lua_State *L ) {
    lua_pushboolean(L, al_reserve_samples(luaL_checkint(L, 1)));
    return 1;
}

static int lg_get_audio_depth_size( lua_State *L ) {
    lua_pushinteger(L, al_get_audio_depth_size(parse_flag_table(L, 1, audio_depth_mapping)));
    return 1;
}

static int lg_get_channel_count( lua_State *L ) {
    lua_pushinteger(L, al_get_channel_count(parse_enum_name(L, 1, channel_conf_mapping)));
    return 1;
}

/*
================================================================================

                Voice

================================================================================
*/
static int lg_create_voice( lua_State *L ) {
    return push_object(L, LEGATO_VOICE, al_create_voice(luaL_checkint(L, 1),
                parse_flag_table(L, 2, audio_depth_mapping), parse_enum_name(L, 3, channel_conf_mapping)), 1);
}

static int lg_destroy_voice( lua_State *L ) {
    ALLEGRO_VOICE *voice = (ALLEGRO_VOICE*) to_object_gc(L, 1, LEGATO_VOICE);
    if ( voice ) {
        al_destroy_voice(voice);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_detach_voice( lua_State *L ) {
    al_detach_voice(to_voice(L, 1));
    return 0;
}

static int lg_attach_audio_stream_to_voice( lua_State *L ) {
    lua_pushboolean(L, al_attach_audio_stream_to_voice(to_audio_stream(L, 1), to_voice(L, 2)));
    return 1;
}

static int lg_attach_mixer_to_voice( lua_State *L ) {
    lua_pushboolean(L, al_attach_mixer_to_voice(to_mixer(L, 1), to_voice(L, 2)));
    return 1;
}

static int lg_attach_sample_instance_to_voice( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_get_voice_frequency( lua_State *L ) {
    lua_pushinteger(L, al_get_voice_frequency(to_voice(L, 1)));
    return 1;
}

static int lg_get_voice_channels( lua_State *L ) {
    return push_enum_name(L, al_get_voice_channels(to_voice(L, 1)), channel_conf_mapping);
}

static int lg_get_voice_depth( lua_State *L ) {
    return push_flag_table(L, al_get_voice_depth(to_voice(L, 1)), audio_depth_mapping);
}

static int lg_get_voice_playing( lua_State *L ) {
    lua_pushboolean(L, al_get_voice_playing(to_voice(L, 1)));
    return 1;
}

static int lg_set_voice_playing( lua_State *L ) {
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    lua_pushboolean(L, al_set_voice_playing(to_voice(L, 1), lua_toboolean(L, 2)));
    return 1;
}

static int lg_get_voice_position( lua_State *L ) {
    lua_pushinteger(L, al_get_voice_position(to_voice(L, 1)));
    return 1;
}

static int lg_set_voice_position( lua_State *L ) {
    lua_pushboolean(L, al_set_voice_position(to_voice(L, 1), luaL_checkint(L, 2)));
    return 1;
}

/*
================================================================================

                Sample

================================================================================
*/
static int lg_destroy_sample( lua_State *L ) {
    ALLEGRO_SAMPLE *sample = (ALLEGRO_SAMPLE*) to_object(L, 1, LEGATO_AUDIO_SAMPLE);
    if ( sample ) {
        al_destroy_sample(sample);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_play_sample( lua_State *L ) {
    int success;
    ALLEGRO_SAMPLE_ID *ret_id = (ALLEGRO_SAMPLE_ID*) push_data(L, LEGATO_SAMPLE_ID, sizeof(ALLEGRO_SAMPLE_ID));
    success = al_play_sample(to_audio_sample(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3),
            luaL_checknumber(L, 4), parse_enum_name(L, 5, playmode_mapping), ret_id);
    lua_pushboolean(L, success);
    return success ? 2 : 1;
}

static int lg_stop_sample( lua_State *L ) {
    al_stop_sample(to_sample_id(L, 1));
    return 0;
}

static int lg_stop_samples( lua_State *L ) {
    al_stop_samples();
    return 0;
}

static int lg_get_sample_channels( lua_State *L ) {
    return push_enum_name(L, al_get_sample_channels(to_audio_sample(L, 1)), channel_conf_mapping);
}

static int lg_get_sample_depth( lua_State *L ) {
    return push_flag_table(L, al_get_sample_depth(to_audio_sample(L, 1)), audio_depth_mapping);
}

static int lg_get_sample_frequency( lua_State *L ) {
    lua_pushinteger(L, al_get_sample_frequency(to_audio_sample(L, 1)));
    return 1;
}

static int lg_get_sample_length( lua_State *L ) {
    lua_pushinteger(L, al_get_sample_length(to_audio_sample(L, 1)));
    return 1;
}

/*
================================================================================

                Sample Instance

================================================================================
*/
static int lg_create_sample_instance( lua_State *L ) {
    return push_object_with_dependency(L, LEGATO_SAMPLE_INSTANCE, al_create_sample_instance(to_audio_sample(L, 1)), 1, 1);
}

static int lg_destroy_sample_instance( lua_State *L ) {
    ALLEGRO_SAMPLE_INSTANCE *si = (ALLEGRO_SAMPLE_INSTANCE*) to_object_gc(L, 1, LEGATO_SAMPLE_INSTANCE);
    if ( si ) {
        al_destroy_sample_instance(si);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_play_sample_instance( lua_State *L ) {
    lua_pushboolean(L, al_play_sample_instance(to_sample_instance(L, 1)));
    return 1;
}

static int lg_stop_sample_instance( lua_State *L ) {
    lua_pushboolean(L, al_stop_sample_instance(to_sample_instance(L, 1)));
    return 1;
}

static int lg_get_sample_instance_channels( lua_State *L ) {
    return push_enum_name(L, al_get_sample_instance_channels(to_sample_instance(L, 1)), channel_conf_mapping);
}

static int lg_get_sample_instance_depth( lua_State *L ) {
    return push_flag_table(L, al_get_sample_instance_channels(to_sample_instance(L, 1)), audio_depth_mapping);
}

static int lg_get_sample_instance_frequency( lua_State *L ) {
    lua_pushinteger(L, al_get_sample_instance_frequency(to_sample_instance(L, 1)));
    return 1;
}

static int lg_get_sample_instance_length( lua_State *L ) {
    lua_pushinteger(L, al_get_sample_instance_length(to_sample_instance(L, 1)));
    return 1;
}

static int lg_set_sample_instance_length( lua_State *L ) {
    lua_pushboolean(L, al_set_sample_instance_length(to_sample_instance(L, 1), luaL_checkinteger(L, 2)));
    return 1;
}

static int lg_get_sample_instance_position( lua_State *L ) {
    lua_pushinteger(L, al_get_sample_instance_position(to_sample_instance(L, 1)));
    return 1;
}

static int lg_set_sample_instance_position( lua_State *L ) {
    lua_pushboolean(L, al_set_sample_instance_position(to_sample_instance(L, 1), luaL_checkinteger(L, 2)));
    return 1;
}

static int lg_get_sample_instance_speed( lua_State *L ) {
    lua_pushnumber(L, al_get_sample_instance_speed(to_sample_instance(L, 1)));
    return 1;
}

static int lg_set_sample_instance_speed( lua_State *L ) {
    lua_pushboolean(L, al_set_sample_instance_speed(to_sample_instance(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int lg_get_sample_instance_gain( lua_State *L ) {
    lua_pushnumber(L, al_get_sample_instance_gain(to_sample_instance(L, 1)));
    return 1;
}

static int lg_set_sample_instance_gain( lua_State *L ) {
    lua_pushboolean(L, al_set_sample_instance_gain(to_sample_instance(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int lg_get_sample_instance_pan( lua_State *L ) {
    lua_pushnumber(L, al_get_sample_instance_pan(to_sample_instance(L, 1)));
    return 1;
}

static int lg_set_sample_instance_pan( lua_State *L ) {
    lua_pushboolean(L, al_set_sample_instance_pan(to_sample_instance(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int lg_get_sample_instance_time( lua_State *L ) {
    lua_pushnumber(L, al_get_sample_instance_time(to_sample_instance(L, 1)));
    return 1;
}

static int lg_get_sample_instance_playmode( lua_State *L ) {
    return push_enum_name(L, al_get_sample_instance_playmode(to_sample_instance(L, 1)), playmode_mapping);
}

static int lg_set_sample_instance_playmode( lua_State *L ) {
    lua_pushboolean(L, al_set_sample_instance_playmode(to_sample_instance(L, 1), parse_enum_name(L, 2, playmode_mapping)));
    return 1;
}

static int lg_get_sample_instance_playing( lua_State *L ) {
    lua_pushboolean(L, al_get_sample_instance_playing(to_sample_instance(L, 1)));
    return 1;
}

static int lg_set_sample_instance_playing( lua_State *L ) {
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    lua_pushboolean(L, al_set_sample_instance_playing(to_sample_instance(L, 1), lua_toboolean(L, 2)));
    return 1;
}

static int lg_get_sample_instance_attached( lua_State *L ) {
    lua_pushboolean(L, al_get_sample_instance_attached(to_sample_instance(L, 1)));
    return 1;
}

static int lg_detach_sample_instance( lua_State *L ) {
    lua_pushboolean(L, al_detach_sample_instance(to_sample_instance(L, 1)));
    return 1;
}

static int lg_get_sample( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_set_sample( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

/*
================================================================================

                Mixer

================================================================================
*/
static int lg_create_mixer( lua_State *L ) {
    return push_object(L, LEGATO_MIXER, al_create_mixer(luaL_checkint(L, 1),
                parse_flag_table(L, 2, audio_depth_mapping), parse_enum_name(L, 3, channel_conf_mapping)), 1);
}

static int lg_destroy_mixer( lua_State *L ) {
    ALLEGRO_MIXER *mixer = (ALLEGRO_MIXER*) to_object(L, 1, LEGATO_MIXER);
    if ( mixer ) {
        al_destroy_mixer(mixer);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_get_default_mixer( lua_State *L ) {
    ALLEGRO_MIXER *mixer = al_get_default_mixer();
    if ( mixer ) {
        return push_object(L, LEGATO_MIXER, mixer, 0);
    } else {
        return 0;
    }
}

static int lg_set_default_mixer( lua_State *L ) {
    lua_pushboolean(L, al_set_default_mixer(to_mixer(L, 1)));
    return 1;
}

static int lg_restore_default_mixer( lua_State *L ) {
    lua_pushboolean(L, al_restore_default_mixer());
    return 1;
}

static int lg_attach_mixer_to_mixer( lua_State *L ) {
    lua_pushboolean(L, al_attach_mixer_to_mixer(to_mixer(L, 1), to_mixer(L, 2)));
    return 1;
}

static int lg_attach_sample_instance_to_mixer( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_attach_audio_stream_to_mixer( lua_State *L ) {
    lua_pushboolean(L, al_attach_audio_stream_to_mixer(to_audio_stream(L, 1), to_mixer(L, 2)));
    return 1;
}

static int lg_get_mixer_frequency( lua_State *L ) {
    lua_pushinteger(L, al_get_mixer_frequency(to_mixer(L, 1)));
    return 1;
}

static int lg_set_mixer_frequency( lua_State *L ) {
    lua_pushboolean(L, al_set_mixer_frequency(to_mixer(L, 1), luaL_checkint(L, 2)));
    return 1;
}

static int lg_get_mixer_channels( lua_State *L ) {
    return push_enum_name(L, al_get_mixer_channels(to_mixer(L, 1)), channel_conf_mapping);
}

static int lg_get_mixer_depth( lua_State *L ) {
    return push_flag_table(L, al_get_mixer_depth(to_mixer(L, 1)), audio_depth_mapping);
}

static int lg_get_mixer_gain( lua_State *L ) {
    lua_pushnumber(L, al_get_mixer_gain(to_mixer(L, 1)));
    return 1;
}

static int lg_set_mixer_gain( lua_State *L ) {
    lua_pushboolean(L, al_set_mixer_gain(to_mixer(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int lg_get_mixer_quality( lua_State *L ) {
    return push_enum_name(L, al_get_mixer_quality(to_mixer(L, 1)), mixer_quality_mapping);
}

static int lg_set_mixer_quality( lua_State *L ) {
    lua_pushboolean(L, al_set_mixer_quality(to_mixer(L, 1), parse_enum_name(L, 2, mixer_quality_mapping)));
    return 1;
}

static int lg_get_mixer_playing( lua_State *L ) {
    lua_pushboolean(L, al_get_mixer_playing(to_mixer(L, 1)));
    return 1;
}

static int lg_set_mixer_playing( lua_State *L ) {
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    lua_pushboolean(L, al_set_mixer_playing(to_mixer(L, 1), lua_toboolean(L, 2)));
    return 1;
}

static int lg_get_mixer_attached( lua_State *L ) {
    lua_pushboolean(L, al_get_mixer_attached(to_mixer(L, 1)));
    return 1;
}

static int lg_detach_mixer( lua_State *L ) {
    lua_pushboolean(L, al_detach_mixer(to_mixer(L, 1)));
    return 1;
}

/*
================================================================================

                Audio Stream

================================================================================
*/
static int lg_destroy_audio_stream( lua_State *L ) {
    ALLEGRO_AUDIO_STREAM *stream = (ALLEGRO_AUDIO_STREAM*) to_object_gc(L, 1, LEGATO_AUDIO_STREAM);
    if ( stream ) {
        al_destroy_audio_stream(stream);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_drain_audio_stream( lua_State *L ) {
    al_drain_audio_stream(to_audio_stream(L, 1));
    return 0;
}

static int lg_rewind_audio_stream( lua_State *L ) {
    lua_pushboolean(L, al_rewind_audio_stream(to_audio_stream(L, 1)));
    return 1;
}

static int lg_get_audio_stream_frequency( lua_State *L ) {
    lua_pushinteger(L, al_get_audio_stream_frequency(to_audio_stream(L, 1)));
    return 1;
}

static int lg_get_audio_stream_channels( lua_State *L ) {
    return push_enum_name(L, al_get_audio_stream_channels(to_audio_stream(L, 1)), channel_conf_mapping);
}

static int lg_get_audio_stream_depth( lua_State *L ) {
    return push_flag_table(L, al_get_audio_stream_depth(to_audio_stream(L, 1)), audio_depth_mapping);
}

static int lg_get_audio_stream_length( lua_State *L ) {
    lua_pushinteger(L, al_get_audio_stream_length(to_audio_stream(L, 1)));
    return 1;
}

static int lg_get_audio_stream_speed( lua_State *L ) {
    lua_pushnumber(L, al_get_audio_stream_speed(to_audio_stream(L, 1)));
    return 1;
}

static int lg_set_audio_stream_speed( lua_State *L ) {
    lua_pushboolean(L, al_set_audio_stream_speed(to_audio_stream(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int lg_get_audio_stream_gain( lua_State *L ) {
    lua_pushnumber(L, al_get_audio_stream_gain(to_audio_stream(L, 1)));
    return 1;
}

static int lg_set_audio_stream_gain( lua_State *L ) {
    lua_pushboolean(L, al_set_audio_stream_gain(to_audio_stream(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int lg_get_audio_stream_pan( lua_State *L ) {
    lua_pushnumber(L, al_get_audio_stream_pan(to_audio_stream(L, 1)));
    return 1;
}

static int lg_set_audio_stream_pan( lua_State *L ) {
    lua_pushboolean(L, al_set_audio_stream_pan(to_audio_stream(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int lg_get_audio_stream_playing( lua_State *L ) {
    lua_pushboolean(L, al_get_audio_stream_playing(to_audio_stream(L, 1)));
    return 1;
}

static int lg_set_audio_stream_playing( lua_State *L ) {
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    lua_pushboolean(L, al_set_audio_stream_playing(to_audio_stream(L, 1), lua_toboolean(L, 2)));
    return 1;
}

static int lg_get_audio_stream_playmode( lua_State *L ) {
    return push_enum_name(L, al_get_audio_stream_playmode(to_audio_stream(L, 1)), playmode_mapping);
}

static int lg_set_audio_stream_playmode( lua_State *L ) {
    lua_pushboolean(L, al_set_audio_stream_playmode(to_audio_stream(L, 1), parse_enum_name(L, 2, playmode_mapping)));
    return 1;
}

static int lg_get_audio_stream_attached( lua_State *L ) {
    lua_pushboolean(L, al_get_audio_stream_attached(to_audio_stream(L, 1)));
    return 1;
}

static int lg_detach_audio_stream( lua_State *L ) {
    lua_pushboolean(L, al_detach_audio_stream(to_audio_stream(L, 1)));
    return 1;
}

static int lg_seek_audio_stream_secs( lua_State *L ) {
    lua_pushboolean(L, al_seek_audio_stream_secs(to_audio_stream(L, 1), luaL_checknumber(L, 2)));
    return 1;
}

static int lg_get_audio_stream_position_secs( lua_State *L ) {
    lua_pushnumber(L, al_get_audio_stream_position_secs(to_audio_stream(L, 1)));
    return 1;
}

static int lg_get_audio_stream_length_secs( lua_State *L ) {
    lua_pushnumber(L, al_get_audio_stream_length_secs(to_audio_stream(L, 1)));
    return 1;
}

static int lg_set_audio_stream_loop_secs( lua_State *L ) {
    lua_pushboolean(L, al_set_audio_stream_loop_secs(to_audio_stream(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3)));
    return 1;
}

/*
================================================================================

                Audio Loaders

================================================================================
*/
static int lg_load_sample( lua_State *L ) {
    return push_object(L, LEGATO_AUDIO_SAMPLE, al_load_sample(luaL_checkstring(L, 1)), 1);
}

static int lg_load_audio_stream( lua_State *L ) {
    return push_object(L, LEGATO_AUDIO_STREAM, al_load_audio_stream(luaL_checkstring(L, 1), luaL_checkint(L, 2), luaL_checkint(L, 3)), 1);
}

/*
================================================================================

                Color addon

================================================================================
*/
static int lg_color_cmyk( lua_State *L ) {
    return push_color(L, al_color_cmyk(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4)));
}

static int lg_color_hsl( lua_State *L ) {
    return push_color(L, al_color_hsl(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3)));
}

static int lg_color_hsv( lua_State *L ) {
    return push_color(L, al_color_hsv(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3)));
}

static int lg_color_html( lua_State *L ) {
    return push_color(L, al_color_html(luaL_checkstring(L, 1)));
}

static int lg_color_name( lua_State *L ) {
    return push_color(L, al_color_name(luaL_checkstring(L, 1)));
}

static int lg_color_yuv( lua_State *L ) {
    return push_color(L, al_color_yuv(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3)));
}

/*
================================================================================

                Font addons

================================================================================
*/
static int lg_load_font( lua_State *L ) {
    return push_object(L, LEGATO_FONT, al_load_font(luaL_checkstring(L, 1), luaL_checkint(L, 2), parse_opt_flag_table(L, 3, ttf_flag_mapping, 0)), 1);
}

static int lg_destroy_font( lua_State *L ) {
    ALLEGRO_FONT *font = (ALLEGRO_FONT*) to_object_gc(L, 1, LEGATO_FONT);
    if ( font ) {
        al_destroy_font(font);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_get_font_line_height( lua_State *L ) {
    lua_pushinteger(L, al_get_font_line_height(to_font(L, 1)));
    return 1;
}

static int lg_get_font_ascent( lua_State *L ) {
    lua_pushinteger(L, al_get_font_ascent(to_font(L, 1)));
    return 1;
}

static int lg_get_font_descent( lua_State *L ) {
    lua_pushinteger(L, al_get_font_descent(to_font(L, 1)));
    return 1;
}

static int lg_get_text_width( lua_State *L ) {
    lua_pushinteger(L, al_get_text_width(to_font(L, 1), luaL_checkstring(L, 2)));
    return 1;
}

static int get_draw_text_flags( lua_State *L, const int idx ) {
    return parse_opt_flag_table(L, idx, draw_text_mapping, 0);
}

static int lg_draw_text( lua_State *L ) {
    al_draw_text(to_font(L, 1), to_color(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4),
            get_draw_text_flags(L, 6), luaL_checkstring(L, 5));
    return 0;
}

static int lg_draw_justified_text( lua_State *L ) {
    al_draw_justified_text(to_font(L, 1), to_color(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4),
            luaL_checknumber(L, 5), luaL_checknumber(L, 6), get_draw_text_flags(L, 8), luaL_checkstring(L, 7));
    return 0;
}

static int lg_get_text_dimensions( lua_State *L ) {
    int bbx, bby, bbw, bbh;
    al_get_text_dimensions(to_font(L, 1), luaL_checkstring(L, 2), &bbx, &bby, &bbw, &bbh);
    lua_pushinteger(L, bbx); lua_pushinteger(L, bby);
    lua_pushinteger(L, bbw); lua_pushinteger(L, bbh);
    return 4;
}

static int lg_grab_font_from_bitmap( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_load_bitmap_font( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_create_builtin_font( lua_State *L ) {
    return push_object(L, LEGATO_FONT, al_create_builtin_font(), 1);
}

static int lg_load_ttf_font( lua_State *L ) {
    return push_object(L, LEGATO_FONT, al_load_ttf_font(luaL_checkstring(L, 1), luaL_checkint(L, 2), parse_opt_flag_table(L, 3, ttf_flag_mapping, 0)), 1);
}

static int lg_load_ttf_font_stretch( lua_State *L ) {
    return push_object(L, LEGATO_FONT, al_load_ttf_font_stretch(luaL_checkstring(L, 1),
                luaL_checkint(L, 2), luaL_checkint(L, 3), parse_flag_table(L, 4, ttf_flag_mapping)), 1);
}

/*
================================================================================

                Primitives addons

================================================================================
*/
static int lg_draw_line( lua_State *L ) {
    al_draw_line(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4),
            to_color(L, 5), luaL_optnumber(L, 6, 1.0));
    return 0;
}

static int lg_draw_triangle( lua_State *L ) {
    al_draw_triangle(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4),
            luaL_checknumber(L, 5), luaL_checknumber(L, 6), to_color(L, 7), luaL_optnumber(L, 8, 1.0));
    return 0;
}

static int lg_draw_filled_triangle( lua_State *L ) {
    al_draw_filled_triangle(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4),
            luaL_checknumber(L, 5), luaL_checknumber(L, 6), to_color(L, 7));
    return 0;
}

static int lg_draw_rectangle( lua_State *L ) {
    al_draw_rectangle(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), to_color(L, 5), luaL_optnumber(L, 6, 1.0));
    return 0;
}

static int lg_draw_filled_rectangle( lua_State *L ) {
    al_draw_filled_rectangle(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), to_color(L, 5));
    return 0;
}

static int lg_draw_rounded_rectangle( lua_State *L ) {
    al_draw_rounded_rectangle(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6),
            to_color(L, 7), luaL_optnumber(L, 8, 1.0));
    return 0;
}

static int lg_draw_filled_rounded_rectangle( lua_State *L ) {
    al_draw_filled_rounded_rectangle(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6),
            to_color(L, 7));
    return 0;
}

static int lg_draw_pieslice( lua_State *L ) {
    al_draw_pieslice(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3),
            luaL_checknumber(L, 4), luaL_checknumber(L, 5), to_color(L, 6), luaL_optnumber(L, 7, 1.0));
    return 0;
}

static int lg_draw_filled_pieslice( lua_State *L ) {
    al_draw_filled_pieslice(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3),
            luaL_checknumber(L, 4), luaL_checknumber(L, 5), to_color(L, 6));
    return 0;
}

static int lg_draw_ellipse( lua_State *L ) {
    al_draw_ellipse(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), to_color(L, 5), luaL_optnumber(L, 6, 1.0));
    return 0;
}

static int lg_draw_filled_ellipse( lua_State *L ) {
    al_draw_filled_ellipse(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), luaL_checknumber(L, 4), to_color(L, 5));
    return 0;
}

static int lg_draw_circle( lua_State *L ) {
    al_draw_circle(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3),
            to_color(L, 4), luaL_optnumber(L, 5, 1.0));
    return 0;
}

static int lg_draw_filled_circle( lua_State *L ) {
    al_draw_filled_circle(luaL_checknumber(L, 1), luaL_checknumber(L, 2),
            luaL_checknumber(L, 3), to_color(L, 4));
    return 0;
}

static int lg_draw_arc( lua_State *L ) {
    al_draw_arc(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3),
            luaL_checknumber(L, 4), luaL_checknumber(L, 5), to_color(L, 6), luaL_optnumber(L, 7, 1.0));
    return 0;
}

static int lg_draw_elliptical_arc( lua_State *L ) {
    al_draw_elliptical_arc(luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3),
            luaL_checknumber(L, 4), luaL_checknumber(L, 5), luaL_checknumber(L, 6), to_color(L, 7), luaL_optnumber(L, 8, 1.0));
    return 0;
}


/*
================================================================================

                FUNCTION DEFS

================================================================================
*/
static const luaL_Reg lg__functions[] = {
    {"get_version", lg_get_version},
    {"load_script", lg_load_script},
    {"encode_UTF8_codepoint", lg_encode_UTF8_codepoint},
    {"get_UTF8_length", lg_get_UTF8_length},

    {"create_config", lg_create_config},
    {"destroy_config", lg_destroy_config},
    {"load_config_file", lg_load_config_file},
    {"save_config_file", lg_save_config_file},
    {"add_config_section", lg_add_config_section},
    {"add_config_comment", lg_add_config_comment},
    {"get_config_value", lg_get_config_value},
    {"set_config_value", lg_set_config_value},
    {"get_config_sections", lg_get_config_sections},
    {"get_config_entries", lg_get_config_entries},
    {"merge_config", lg_merge_config},
    {"merge_config_into", lg_merge_config_into},

    {"create_display", lg_create_display},
    {"destroy_display", lg_destroy_display},
    {"get_new_display_flags", lg_get_new_display_flags},
    {"set_new_display_flags", lg_set_new_display_flags},
    {"get_new_display_option", lg_get_new_display_option},
    {"set_new_display_option", lg_set_new_display_option},
    {"reset_new_display_options", lg_reset_new_display_options},
    {"get_new_window_position", lg_get_new_window_position},
    {"set_new_window_position", lg_set_new_window_position},
    {"get_new_display_refresh_rate", lg_get_new_display_refresh_rate},
    {"set_new_display_refresh_rate", lg_set_new_display_refresh_rate},
    {"get_backbuffer", lg_get_backbuffer},
    {"flip_display", lg_flip_display},
    {"update_display_region", lg_update_display_region},
    {"wait_for_vsync", lg_wait_for_vsync},
    {"get_display_width", lg_get_display_width},
    {"get_display_height", lg_get_display_height},
    {"get_display_size", lg_get_display_size},
    {"resize_display", lg_resize_display},
    {"acknowledge_display", lg_acknowledge_resize},
    {"get_window_position", lg_get_window_position},
    {"set_window_position", lg_set_window_position},
    {"get_display_flags", lg_get_display_flags},
    {"set_display_flag", lg_set_display_flag},
    {"get_display_option", lg_get_display_option},
    {"get_display_format", lg_get_display_format},
    {"get_dsplay_refresh_rate", lg_get_display_refresh_rate},
    {"set_window_title", lg_set_window_title},
    {"set_display_icon", lg_set_display_icon},
    {"set_display_icons", lg_set_display_icons},
    {"inhibit_screensaver", lg_inhibit_screensaver},

    {"create_event_queue", lg_create_event_queue},
    {"destroy_event_queue", lg_destroy_event_queue},
    {"register_event_source", lg_register_event_source},
    {"unregister_event_source", lg_unregister_event_source},
    {"is_event_queue_empty", lg_is_event_queue_empty},
    {"get_next_event", lg_get_next_event},
    {"peek_next_event", lg_peek_next_event},
    {"drop_next_event", lg_drop_next_event},
    {"flush_event_queue", lg_flush_event_queue},
    {"wait_for_event", lg_wait_for_event},
    {"wait_for_event_timed", lg_wait_for_event_timed},
    {"wait_for_event_until", lg_wait_for_event_until},

    {"get_display_modes", lg_get_display_modes},

    {"map_rgb", lg_map_rgb},
    {"map_rgb_f", lg_map_rgb_f},
    {"unmap_rgb", lg_unmap_rgb},
    {"unmap_rgb_f", lg_unmap_rgb_f},
    {"get_pixel_size", lg_get_pixel_size},
    {"get_pixel_format_bits", lg_get_pixel_format_bits},
    {"lock_bitmap", lg_lock_bitmap},
    {"lock_bitmap_region", lg_lock_bitmap_region},
    {"unlock_bitmap", lg_unlock_bitmap},
    {"create_bitmap", lg_create_bitmap},
    {"create_sub_bitmap", lg_create_sub_bitmap},
    {"clone_bitmap", lg_clone_bitmap},
    {"destroy_bitmap", lg_destroy_bitmap},
    {"get_new_bitmap_flags", lg_get_new_bitmap_flags},
    {"get_new_bitmap_format", lg_get_new_bitmap_format},
    {"set_new_bitmap_flags", lg_set_new_bitmap_flags},
    {"add_new_bitmap_flag", lg_add_new_bitmap_flag},
    {"set_new_bitmap_format", lg_set_new_bitmap_format},
    {"get_bitmap_flags", lg_get_bitmap_flags},
    {"get_bitmap_format", lg_get_bitmap_format},
    {"get_bitmap_height", lg_get_bitmap_height},
    {"get_bitmap_width", lg_get_bitmap_width},
    {"get_bitmap_size", lg_get_bitmap_size},
    {"get_pixel", lg_get_pixel},
    {"is_bitmap_locked", lg_is_bitmap_locked},
    {"is_compatible_bitmap", lg_is_compatible_bitmap},
    {"is_sub_bitmap", lg_is_sub_bitmap},
    {"get_parent_bitmap", lg_get_parent_bitmap},
    {"clear_to_color", lg_clear_to_color},
    {"draw_bitmap", lg_draw_bitmap},
    {"draw_tinted_bitmap", lg_draw_tinted_bitmap},
    {"draw_bitmap_region", lg_draw_bitmap_region},
    {"draw_tinted_bitmap_region", lg_draw_tinted_bitmap_region},
    {"draw_pixel", lg_draw_pixel},
    {"draw_rotated_bitmap", lg_draw_rotated_bitmap},
    {"draw_tinted_rotated_bitmap", lg_draw_tinted_rotated_bitmap},
    {"draw_scaled_rotated_bitmap", lg_draw_scaled_rotated_bitmap},
    {"draw_tinted_scaled_rotated_bitmap", lg_draw_tinted_scaled_rotated_bitmap},
    {"draw_tinted_scaled_rotated_bitmap_region", lg_draw_tinted_scaled_rotated_bitmap_region},
    {"draw_scaled_bitmap", lg_draw_scaled_bitmap},
    {"draw_tinted_scaled_bitmap", lg_draw_tinted_scaled_bitmap},
    {"get_target_bitmap", lg_get_target_bitmap},
    {"put_pixel", lg_put_pixel},
    {"put_blended_pixel", lg_put_blended_pixel},
    {"set_target_bitmap", lg_set_target_bitmap},
    {"set_target_backbuffer", lg_set_target_backbuffer},
    {"get_current_display", lg_get_current_display},
    {"get_blender", lg_get_blender},
    {"get_separate_blender", lg_get_separate_blender},
    {"set_blender", lg_set_blender},
    {"set_separate_blender", lg_set_separate_blender},
    {"get_clipping_rectangle", lg_get_clipping_rectangle},
    {"set_clipping_rectangle", lg_set_clipping_rectangle},
    {"reset_clipping_rectangle", lg_reset_clipping_rectangle},
    {"convert_mask_to_alpha", lg_convert_mask_to_alpha},
    {"hold_bitmap_drawing", lg_hold_bitmap_drawing},
    {"is_bitmap_drawing_held", lg_is_bitmap_drawing_held},
    {"load_bitmap", lg_load_bitmap},

    {"is_keyboard_installed", lg_is_keyboard_installed},
    {"create_keyboard_state", lg_create_keyboard_state},
    {"get_keyboard_state", lg_get_keyboard_state},
    {"key_down", lg_key_down},
    {"keycode_to_name", lg_keycode_to_name},
    {"set_keyboard_leds", lg_set_keyboard_leds},

    {"get_new_display_adapter", lg_get_new_display_adapter},
    {"set_new_display_adapter", lg_set_new_display_adapter},
    {"get_monitor_info", lg_get_monitor_info},

    {"is_mouse_installed", lg_is_mouse_installed},
    {"get_mouse_num_axes", lg_get_mouse_num_axes},
    {"get_mouse_num_buttons", lg_get_mouse_num_buttons},
    {"create_mouse_state", lg_create_mouse_state},
    {"get_mouse_state", lg_get_mouse_state},
    {"get_mouse_state_axis", lg_get_mouse_state_axis},
    {"mouse_button_down", lg_mouse_button_down},
    {"set_mouse_xy", lg_set_mouse_xy},
    {"set_mouse_z", lg_set_mouse_z},
    {"set_mouse_w", lg_set_mouse_w},
    {"set_mouse_axis", lg_set_mouse_axis},
    {"create_mouse_cursor", lg_create_mouse_cursor},
    {"destroy_mouse_cursor", lg_destroy_mouse_cursor},
    {"set_mouse_cursor", lg_set_mouse_cursor},
    {"set_system_mouse_cursor", lg_set_system_mouse_cursor},
    {"get_mouse_cursor_position", lg_get_mouse_cursor_position},
    {"hide_mouse_cursor", lg_hide_mouse_cursor},
    {"show_mouse_cursor", lg_show_mouse_cursor},
    {"grab_mouse", lg_grab_mouse},
    {"ungrab_mouse", lg_ungrab_mouse},

    {"create_path", lg_create_path},
    {"create_path_for_directory", lg_create_path_for_directory},
    {"destroy_path", lg_destroy_path},
    {"clone_path", lg_clone_path},
    {"join_paths", lg_join_paths},
    {"rebase_path", lg_rebase_path},
    {"get_path_drive", lg_get_path_drive},
    {"get_path_num_components", lg_get_path_num_components},
    {"get_path_components", lg_get_path_components},
    {"get_path_tail", lg_get_path_tail},
    {"get_path_filename", lg_get_path_filename},
    {"get_path_basename", lg_get_path_basename},
    {"get_path_extension", lg_get_path_extension},
    {"set_path_drive", lg_set_path_drive},
    {"append_path_component", lg_append_path_component},
    {"insert_path_component", lg_insert_path_component},
    {"replace_path_component", lg_replace_path_component},
    {"remove_path_component", lg_remove_path_component},
    {"drop_path_tail", lg_drop_path_tail},
    {"set_path_filename", lg_set_path_filename},
    {"set_path_extension", lg_set_path_extension},
    {"path_str", lg_path_str},
    {"make_path_canonical", lg_make_path_canonical},

    {"restore_state", lg_restore_state},
    {"store_state", lg_store_state},
    {"get_errno", lg_get_errno},
    {"set_errno", lg_set_errno},

    {"get_allegro_version", lg_get_allegro_version},
    {"get_standard_path", lg_get_standard_path},
    {"set_exe_name", lg_set_exe_name},
    {"set_app_name", lg_set_app_name},
    {"set_org_name", lg_set_org_name},
    {"get_app_name", lg_get_app_name},
    {"get_org_name", lg_get_org_name},
    {"get_system_config", lg_get_system_config},

    {"get_time", lg_get_time},
    {"rest", lg_rest},

    {"create_timer", lg_create_timer},
    {"start_timer", lg_start_timer},
    {"stop_timer", lg_stop_timer},
    {"get_timer_started", lg_get_timer_started},
    {"destroy_timer", lg_destroy_timer},
    {"get_timer_count", lg_get_timer_count},
    {"set_timer_count", lg_set_timer_count},
    {"add_timer_count", lg_add_timer_count},
    {"get_timer_speed", lg_get_timer_speed},
    {"set_timer_speed", lg_set_timer_speed},

    {"create_transform", lg_create_transform},
    {"copy_transform", lg_copy_transform},
    {"use_transform", lg_use_transform},
    {"get_current_transform", lg_get_current_transform},
    {"invert_transform", lg_invert_transform},
    {"check_inverse", lg_check_inverse},
    {"identity_transform", lg_identity_transform},
    {"build_transform", lg_build_transform},
    {"translate_transform", lg_translate_transform},
    {"rotate_transform", lg_rotate_transform},
    {"scale_transform", lg_scale_transform},
    {"transform_coordinates", lg_transform_coordinates},
    {"compose_transform", lg_compose_transform},

    {"is_audio_installed", lg_is_audio_installed},
    {"reserve_samples", lg_reserve_samples},
    {"get_audio_depth_size", lg_get_audio_depth_size},
    {"get_channel_count", lg_get_channel_count},
    {"create_voice", lg_create_voice},
    {"destroy_voice", lg_destroy_voice},
    {"detach_voice", lg_detach_voice},
    {"attach_audio_stream_to_voice", lg_attach_audio_stream_to_voice},
    {"attach_mixer_to_voice", lg_attach_mixer_to_voice},
    {"attach_sample_instance_to_voice", lg_attach_sample_instance_to_voice},
    {"get_voice_frequency", lg_get_voice_frequency},
    {"get_voice_channels", lg_get_voice_channels},
    {"get_voice_depth", lg_get_voice_depth},
    {"get_voice_playing", lg_get_voice_playing},
    {"set_voice_playing", lg_set_voice_playing},
    {"get_voice_position", lg_get_voice_position},
    {"set_voice_position", lg_set_voice_position},
    
    {"destroy_sample", lg_destroy_sample},
    {"play_sample", lg_play_sample},
    {"stop_sample", lg_stop_sample},
    {"stop_samples", lg_stop_samples},
    {"get_sample_channels", lg_get_sample_channels},
    {"get_sample_depth", lg_get_sample_depth},
    {"get_sample_frequency", lg_get_sample_frequency},
    {"get_sample_length", lg_get_sample_length},

    {"create_sample_instance", lg_create_sample_instance},
    {"destroy_sample_instance", lg_destroy_sample_instance},
    {"play_sample_instance", lg_play_sample_instance},
    {"stop_sample_instance", lg_stop_sample_instance},
    {"get_sample_instance_channels", lg_get_sample_instance_channels},
    {"get_sample_instance_depth", lg_get_sample_instance_depth},
    {"get_sample_instance_frequency", lg_get_sample_instance_frequency},
    {"get_sample_instance_length", lg_get_sample_instance_length},
    {"set_sample_instance_length", lg_set_sample_instance_length},
    {"get_sample_instance_position", lg_get_sample_instance_position},
    {"set_sample_instance_position", lg_set_sample_instance_position},
    {"get_sample_instance_position", lg_get_sample_instance_position},
    {"get_sample_instance_speed", lg_get_sample_instance_speed},
    {"set_sample_instance_speed", lg_set_sample_instance_speed},
    {"get_sample_instance_gain", lg_get_sample_instance_gain},
    {"set_sample_instance_gain", lg_set_sample_instance_gain},
    {"get_sample_instance_pan", lg_get_sample_instance_pan},
    {"set_sample_instance_pan", lg_set_sample_instance_pan},
    {"get_sample_instance_time", lg_get_sample_instance_time},
    {"get_sample_instance_playmode", lg_get_sample_instance_playmode},
    {"set_sample_instance_playmode", lg_set_sample_instance_playmode},
    {"get_sample_instance_playing", lg_get_sample_instance_playing},
    {"set_sample_instance_playing", lg_set_sample_instance_playing},
    {"get_sample_instance_attached", lg_get_sample_instance_attached},
    {"detach_sample_instance", lg_detach_sample_instance},
    {"get_sample", lg_get_sample},
    {"set_sample", lg_set_sample},
    
    {"create_mixer", lg_create_mixer},
    {"destroy_mixer", lg_destroy_mixer},
    {"get_default_mixer", lg_get_default_mixer},
    {"set_default_mixer", lg_set_default_mixer},
    {"restore_default_mixer", lg_restore_default_mixer},
    {"attach_mixer_to_mixer", lg_attach_mixer_to_mixer},
    {"attach_sample_instance_to_mixer", lg_attach_sample_instance_to_mixer},
    {"attach_audio_stream_to_mixer", lg_attach_audio_stream_to_mixer},
    {"get_mixer_frequency", lg_get_mixer_frequency},
    {"set_mixer_frequency", lg_set_mixer_frequency},
    {"get_mixer_channels", lg_get_mixer_channels},
    {"get_mixer_depth", lg_get_mixer_depth},
    {"get_mixer_gain", lg_get_mixer_gain},
    {"set_mixer_gain", lg_set_mixer_gain},
    {"get_mixer_quality", lg_get_mixer_quality},
    {"set_mixer_quality", lg_set_mixer_quality},
    {"get_mixer_playing", lg_get_mixer_playing},
    {"set_mixer_playing", lg_set_mixer_playing},
    {"get_mixer_attached", lg_get_mixer_attached},
    {"detach_mixer", lg_detach_mixer},

    {"destroy_audio_stream", lg_destroy_audio_stream},
    {"drain_audio_stream", lg_drain_audio_stream},
    {"rewind_audio_stream", lg_rewind_audio_stream},
    {"get_audio_stream_frequency", lg_get_audio_stream_frequency},
    {"get_audio_stream_channels", lg_get_audio_stream_channels},
    {"get_audio_stream_depth", lg_get_audio_stream_depth},
    {"get_audio_stream_length", lg_get_audio_stream_length},
    {"get_audio_stream_speed", lg_get_audio_stream_speed},
    {"set_audio_stream_speed", lg_set_audio_stream_speed},
    {"get_audio_stream_gain", lg_get_audio_stream_gain},
    {"set_audio_stream_gain", lg_set_audio_stream_gain},
    {"get_audio_stream_pan", lg_get_audio_stream_pan},
    {"set_audio_stream_pan", lg_set_audio_stream_pan},
    {"get_audio_stream_playing", lg_get_audio_stream_playing},
    {"set_audio_stream_playing", lg_set_audio_stream_playing},
    {"get_audio_stream_playmode", lg_get_audio_stream_playmode},
    {"set_audio_stream_playmode", lg_set_audio_stream_playmode},
    {"get_audio_stream_attached", lg_get_audio_stream_attached},
    {"get_detach_audio_stream", lg_detach_audio_stream},
    {"seek_audio_stream_secs", lg_seek_audio_stream_secs},
    {"get_audio_stream_position_secs", lg_get_audio_stream_position_secs},
    {"get_audio_stream_length_secs", lg_get_audio_stream_length_secs},
    {"set_audio_stream_loop_secs", lg_set_audio_stream_loop_secs},

    {"load_sample", lg_load_sample},
    {"load_audio_stream", lg_load_audio_stream},

    {"color_cmyk", lg_color_cmyk},
    {"color_hsl", lg_color_hsl},
    {"color_hsv", lg_color_hsv},
    {"color_html", lg_color_html},
    {"color_name", lg_color_name},
    {"color_yuv", lg_color_yuv},

    {"load_font", lg_load_font},
    {"destroy_font", lg_destroy_font},
    {"get_font_line_height", lg_get_font_line_height},
    {"get_font_ascent", lg_get_font_ascent},
    {"get_font_descent", lg_get_font_descent},
    {"get_text_width", lg_get_text_width},
    {"draw_text", lg_draw_text},
    {"draw_justified_text", lg_draw_justified_text},
    {"get_text_dimensions", lg_get_text_dimensions},
    {"grab_font_from_bitmap", lg_grab_font_from_bitmap},
    {"load_bitmap_font", lg_load_bitmap_font},
    {"create_builtin_font", lg_create_builtin_font},
    {"load_ttf_font", lg_load_ttf_font},
    {"load_ttf_font_stretch", lg_load_ttf_font_stretch},

    {"draw_line", lg_draw_line},
    {"draw_triangle", lg_draw_triangle},
    {"draw_filled_triangle", lg_draw_filled_triangle},
    {"draw_rectangle", lg_draw_rectangle},
    {"draw_filled_rectangle", lg_draw_filled_rectangle},
    {"draw_rounded_rectangle", lg_draw_rounded_rectangle},
    {"draw_filled_rounded_rectangle", lg_draw_filled_rounded_rectangle},
    {"draw_pieslice", lg_draw_pieslice},
    {"draw_filled_pieslice", lg_draw_filled_pieslice},
    {"draw_ellipse", lg_draw_ellipse},
    {"draw_filled_ellipse", lg_draw_filled_ellipse},
    {"draw_circle", lg_draw_circle},
    {"draw_filled_circle", lg_draw_filled_circle},
    {"draw_arc", lg_draw_arc},
    {"draw_elliptical_arc", lg_draw_elliptical_arc},

    {NULL, NULL}
};

/*
================================================================================

                ALLEGRO 5 - OBJECTS

================================================================================
*/
/*
================================================================================

                Configuration files

================================================================================
*/
static ALLEGRO_CONFIG *to_config( lua_State *L, const int idx ) {
    return (ALLEGRO_CONFIG*) to_object(L, idx, LEGATO_CONFIG);
}

static int config__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_CONFIG, to_object_gc(L, 1, LEGATO_CONFIG));
    return 1;
}

/*
static int config__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_CONFIG);
}
*/

static const luaL_Reg config__methods[] = {
    {"__gc", lg_destroy_config},
    {"__tostring", config__tostring},
    /*{"__eq", config__eq},*/
    {"destroy", lg_destroy_config},
    {"save_file", lg_save_config_file},
    {"add_section", lg_add_config_section},
    {"add_comment", lg_add_config_comment},
    {"get_value", lg_get_config_value},
    {"set_value", lg_set_config_value},
    {"get_sections", lg_get_config_sections},
    {"get_entries", lg_get_config_entries},
    {"merge", lg_merge_config},
    {"merge_into", lg_merge_config_into},
    {NULL, NULL}
};

/*
================================================================================

                Display

================================================================================
*/
static ALLEGRO_DISPLAY *to_display( lua_State *L, const int idx ) {
    return (ALLEGRO_DISPLAY*) to_object(L, idx, LEGATO_DISPLAY);
}

static int display__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_DISPLAY, to_object_gc(L, 1, LEGATO_DISPLAY));
    return 1;
}

/*
static int display__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_DISPLAY);
}
*/

static const luaL_Reg display__methods[] = {
    {"__gc", lg_destroy_display},
    {"__tostring", display__tostring},
    /*{"__eq", display__eq},*/
    {"get_backbuffer", lg_get_backbuffer},
    {"get_width", lg_get_display_width},
    {"get_height", lg_get_display_height},
    {"get_size", lg_get_display_size},
    {"resize", lg_resize_display},
    {"acknowledge_resize", lg_acknowledge_resize},
    {"get_position", lg_get_window_position},
    {"set_position", lg_set_window_position},
    {"get_flags", lg_get_display_flags},
    {"set_flag", lg_set_display_flag},
    {"get_option", lg_get_display_option},
    {"get_format", lg_get_display_format},
    {"get_refresh_rate", lg_get_display_refresh_rate},
    {"set_title", lg_set_window_title},
    {"set_icon", lg_set_display_icon},
    {"set_icons", lg_set_display_icons},
    {"set_target", lg_set_target_backbuffer},
    {NULL, NULL}
};

/*
================================================================================

                Color

================================================================================
*/
static ALLEGRO_COLOR to_color( lua_State *L, const int idx ) {
    return *((ALLEGRO_COLOR*) luaL_checkudata(L, idx, LEGATO_COLOR));
}

static int color__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s", LEGATO_COLOR);
    return 1;
}

static const luaL_Reg color__methods[] = {
    {"__tostring", color__tostring},
    {"unmap_rgb", lg_unmap_rgb},
    {"unmap_rgb_f", lg_unmap_rgb_f},
    {NULL, NULL}
};

/*
================================================================================

                Bitmap

================================================================================
*/
static ALLEGRO_BITMAP *to_bitmap( lua_State *L, const int idx ) {
    return (ALLEGRO_BITMAP*) to_object(L, idx, LEGATO_BITMAP);
}

static int bitmap__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_BITMAP, to_object_gc(L, 1, LEGATO_BITMAP));
    return 1;
}

/*
static int bitmap__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_BITMAP);
}
*/

static const luaL_Reg bitmap__methods[] = {
    {"__gc", lg_destroy_bitmap},
    {"__tostring", bitmap__tostring},
    /*{"__eq", bitmap__eq},*/
    {"lock", lg_lock_bitmap},
    {"lock_region", lg_lock_bitmap_region},
    {"unlock", lg_unlock_bitmap},
    {"create_sub_bitmap", lg_create_sub_bitmap},
    {"clone", lg_clone_bitmap},
    {"destroy", lg_destroy_bitmap},
    {"get_flags", lg_get_bitmap_flags},
    {"get_format", lg_get_bitmap_format},
    {"get_height", lg_get_bitmap_height},
    {"get_width", lg_get_bitmap_width},
    {"get_size", lg_get_bitmap_size},
    {"get_pixel", lg_get_pixel},
    {"is_locked", lg_is_bitmap_locked},
    {"is_compatible", lg_is_compatible_bitmap},
    {"is_sub_bitmap", lg_is_sub_bitmap},
    {"get_parent", lg_get_parent_bitmap},
    {"draw", lg_draw_bitmap},
    {"draw_tinted", lg_draw_tinted_bitmap},
    {"draw_region", lg_draw_bitmap_region},
    {"draw_tinted_region", lg_draw_tinted_bitmap_region},
    {"draw_scaled_rotated", lg_draw_scaled_rotated_bitmap},
    {"draw_tinted_scaled_rotated", lg_draw_tinted_scaled_rotated_bitmap},
    {"draw_tinted_scaled_rotated_region", lg_draw_tinted_scaled_rotated_bitmap_region},
    {"draw_scaled", lg_draw_scaled_bitmap},
    {"draw_tinted_scaled", lg_draw_tinted_scaled_bitmap},
    {"set_target", lg_set_target_bitmap},
    {"convert_mask_to_alpha", lg_convert_mask_to_alpha},
    {NULL, NULL}
};

/*
================================================================================

                Event queue

================================================================================
*/
static ALLEGRO_EVENT_QUEUE *to_event_queue( lua_State *L, const int idx ) {
    return (ALLEGRO_EVENT_QUEUE*) to_object(L, idx, LEGATO_EVENT_QUEUE);
}

static int event_queue__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_EVENT_QUEUE, to_object_gc(L, 1, LEGATO_EVENT_QUEUE));
    return 1;
}

/*
static int event_queue__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_EVENT_QUEUE);
}
*/

static const luaL_Reg event_queue__methods[] = {
    {"__gc", lg_destroy_event_queue},
    {"__tostring", event_queue__tostring},
    /*{"__eq", event_queue__eq},*/
    {"register", lg_register_event_source},
    {"unregister", lg_unregister_event_source},
    {"is_empty", lg_is_event_queue_empty},
    {"get_next_event", lg_get_next_event},
    {"peek_next_event", lg_peek_next_event},
    {"drop_next_event", lg_drop_next_event},
    {"flush", lg_flush_event_queue},
    {"wait_for_event", lg_wait_for_event},
    {"wait_for_event_timed", lg_wait_for_event_timed},
    {"wait_for_event_until", lg_wait_for_event_until},
    {NULL, NULL}
};

/*
================================================================================

                Keyboard State

================================================================================
*/
static ALLEGRO_KEYBOARD_STATE *to_keyboard_state( lua_State *L, const int idx ) {
    return (ALLEGRO_KEYBOARD_STATE*) luaL_checkudata(L, idx, LEGATO_KEYBOARD_STATE);
}

static int keyboard_state__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_KEYBOARD_STATE, to_keyboard_state(L, 1));
    return 1;
}

static const luaL_Reg keyboard_state__methods[] = {
    {"__tostring", keyboard_state__tostring},
    {"get_state", lg_get_keyboard_state},
    {"key_down", lg_key_down},
    {NULL, NULL}
};

/*
================================================================================

                Mouse State

================================================================================
*/
static ALLEGRO_MOUSE_STATE *to_mouse_state( lua_State *L, const int idx ) {
    return (ALLEGRO_MOUSE_STATE*) luaL_checkudata(L, idx, LEGATO_MOUSE_STATE);
}

static int mouse_state__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_MOUSE_STATE, to_mouse_state(L, 1));
    return 1;
}

static const luaL_Reg mouse_state__methods[] = {
    {"__tostring", mouse_state__tostring},
    {"get_state", lg_get_mouse_state},
    {"get_axis", lg_get_mouse_state_axis},
    {"button_down", lg_mouse_button_down},
    {NULL, NULL}
};

/*
================================================================================

                Path

================================================================================
*/
static ALLEGRO_PATH *to_path( lua_State *L, const int idx ) {
    return (ALLEGRO_PATH*) to_object(L, idx, LEGATO_PATH);
}

static int path__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_PATH, to_object_gc(L, 1, LEGATO_PATH));
    return 1;
}

/*
static int path__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_PATH);
}
*/

static const luaL_Reg path__methods[] = {
    {"__tostring", path__tostring},
    /*{"__eq", path__eq},*/
    {"__gc", lg_destroy_path},
    {"destroy", lg_destroy_path},
    {"clone", lg_clone_path},
    {"join_path", lg_join_paths},
    {"rebase", lg_rebase_path},
    {"get_drive", lg_get_path_drive},
    {"get_num_components", lg_get_path_num_components},
    {"get_components", lg_get_path_components},
    {"get_tail", lg_get_path_tail},
    {"get_filename", lg_get_path_filename},
    {"get_basename", lg_get_path_basename},
    {"get_extension", lg_get_path_extension},
    {"set_drive", lg_set_path_drive},
    {"append_component", lg_append_path_component},
    {"insert_component", lg_insert_path_component},
    {"replace_component", lg_replace_path_component},
    {"remove_component", lg_remove_path_component},
    {"drop_tail", lg_drop_path_tail},
    {"set_filename", lg_set_path_filename},
    {"set_extension", lg_set_path_extension},
    {"str", lg_path_str},
    {"make_canonical", lg_make_path_canonical},
    {NULL, NULL}
};

/*
================================================================================

                State

================================================================================
*/
static ALLEGRO_STATE *to_state( lua_State *L, const int idx ) {
    return (ALLEGRO_STATE*) luaL_checkudata(L, idx, LEGATO_STATE);
}

static int state__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_STATE, to_state(L, 1));
    return 1;
}

static const luaL_Reg state__methods[] = {
    {"__tostring", state__tostring},
    {"restore", lg_restore_state},
    {NULL, NULL}
};

/*
================================================================================

                Timers

================================================================================
*/
static ALLEGRO_TIMER *to_timer( lua_State *L, const int idx ) {
    return (ALLEGRO_TIMER*) to_object(L, idx, LEGATO_TIMER);
}

static int timer__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_TIMER, to_object_gc(L, 1, LEGATO_TIMER));
    return 1;
}

/*
static int timer__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_TIMER);
}
*/

static const luaL_Reg timer__methods[] = {
    {"__gc", lg_destroy_timer},
    {"__tostring", timer__tostring},
    /*{"__eq", timer__eq},*/
    {"start", lg_start_timer},
    {"stop", lg_stop_timer},
    {"get_started", lg_get_timer_started},
    {"get_count", lg_get_timer_count},
    {"set_count", lg_set_timer_count},
    {"add_count", lg_add_timer_count},
    {"get_speed", lg_get_timer_speed},
    {"set_speed", lg_set_timer_speed},
    {NULL, NULL}
};

/*
================================================================================

                Transformations

================================================================================
*/
static ALLEGRO_TRANSFORM *to_transform( lua_State *L, const int idx ) {
    return (ALLEGRO_TRANSFORM*) luaL_checkudata(L, idx, LEGATO_TRANSFORM);
}

static int transform__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_TRANSFORM, to_transform(L, 1));
    return 1;
}

static const luaL_Reg transform__methods[] = {
    {"__tostring", transform__tostring},
    {"copy", lg_copy_transform},
    {"use", lg_use_transform},
    {"invert", lg_invert_transform},
    {"check_inverse", lg_check_inverse},
    {"identity", lg_identity_transform},
    {"translate", lg_translate_transform},
    {"rotate", lg_rotate_transform},
    {"scale", lg_scale_transform},
    {"transform", lg_transform_coordinates},
    {"compose", lg_compose_transform},
    {NULL, NULL}
};

/*
================================================================================

                Audio Voice

================================================================================
*/
static ALLEGRO_VOICE *to_voice( lua_State *L, const int idx ) {
    return (ALLEGRO_VOICE*) to_object(L, idx, LEGATO_VOICE);
}

static int voice__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_VOICE, to_voice(L, 1));
    return 1;
}

/*
static int voice__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_VOICE);
}
*/

static const luaL_Reg voice__methods[] = {
    {"__gc", lg_destroy_voice},
    {"__tostring", voice__tostring},
    /*{"__eq", voice__eq},*/
    {"get_frequency", lg_get_voice_frequency},
    {"get_channels", lg_get_voice_channels},
    {"get_depth", lg_get_voice_depth},
    {"get_playing", lg_get_voice_playing},
    {"set_playing", lg_set_voice_playing},
    {"get_position", lg_get_voice_position},
    {"set_position", lg_set_voice_position},
    {NULL, NULL}
};

/*
================================================================================

                Audio Mixer

================================================================================
*/
static ALLEGRO_MIXER *to_mixer( lua_State *L, const int idx ) {
    return (ALLEGRO_MIXER*) to_object(L, idx, LEGATO_MIXER);
}

static int mixer__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_MIXER, to_mixer(L, 1));
    return 1;
}

/*
static int mixer__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_MIXER);
}
*/

static const luaL_Reg mixer__methods[] = {
    {"__tostring", mixer__tostring},
    /*{"__eq", mixer__eq},*/
    {"__gc", lg_destroy_mixer},
    {"attach_to_mixer", lg_attach_mixer_to_mixer},
    {"get_frequency", lg_get_mixer_frequency},
    {"set_frequency", lg_set_mixer_frequency},
    {"get_channels", lg_get_mixer_channels},
    {"get_depth", lg_get_mixer_depth},
    {"get_gain", lg_get_mixer_gain},
    {"set_gain", lg_set_mixer_gain},
    {"get_quality", lg_get_mixer_quality},
    {"set_quality", lg_set_mixer_quality},
    {"get_playing", lg_get_mixer_playing},
    {"set_playing", lg_set_mixer_playing},
    {"get_attached", lg_get_mixer_attached},
    {"detach", lg_detach_mixer},
    {NULL, NULL}
};

/*
================================================================================

                Audio Sample

================================================================================
*/
static ALLEGRO_SAMPLE *to_audio_sample( lua_State *L, const int idx ) {
    return (ALLEGRO_SAMPLE*) to_object(L, idx, LEGATO_AUDIO_SAMPLE);
}

static int audio_sample__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_AUDIO_SAMPLE, to_audio_sample(L, 1));
    return 1;
}

/*
static int audio_sample__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_AUDIO_SAMPLE);
}
*/

static const luaL_Reg audio_sample__methods[] = {
    {"__tostring", audio_sample__tostring},
    /*{"__eq", audio_sample__eq},*/
    {NULL, NULL}
};

/*
================================================================================

                Audio Sample ID

================================================================================
*/
static ALLEGRO_SAMPLE_ID *to_sample_id( lua_State *L, const int idx ) {
    return (ALLEGRO_SAMPLE_ID*) luaL_checkudata(L, idx, LEGATO_SAMPLE_ID);
}

static int sample_id__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_SAMPLE_ID, to_sample_id(L, 1));
    return 1;
}

static const luaL_Reg sample_id__methods[] = {
    {"__tostring", sample_id__tostring},
    {"stop", lg_stop_sample},
    {NULL, NULL}
};

/*
================================================================================

                Audio Sample Instance

================================================================================
*/
static ALLEGRO_SAMPLE_INSTANCE *to_sample_instance( lua_State *L, const int idx ) {
    return (ALLEGRO_SAMPLE_INSTANCE*) to_object(L, idx, LEGATO_SAMPLE_INSTANCE);
}

static int sample_instance__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_SAMPLE_INSTANCE, to_sample_instance(L, 1));
    return 1;
}

/*
static int sample_instance__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_SAMPLE_INSTANCE);
}
*/

static const luaL_Reg sample_instance__methods[] = {
    {"__tostring", sample_instance__tostring},
    /*{"__eq", sample_instance__eq},*/
    {"__gc", lg_destroy_sample_instance},
    {"destroy", lg_destroy_sample_instance},
    {"play", lg_play_sample_instance},
    {"stop", lg_stop_sample_instance},
    {"get_channels", lg_get_sample_instance_channels},
    {"get_depth", lg_get_sample_instance_depth},
    {"get_frequency", lg_get_sample_instance_frequency},
    {"get_length", lg_get_sample_instance_length},
    {"set_length", lg_set_sample_instance_length},
    {"get_position", lg_get_sample_instance_position},
    {"set_position", lg_set_sample_instance_position},
    {"get_speed", lg_get_sample_instance_speed},
    {"set_speed", lg_set_sample_instance_speed},
    {"get_gain", lg_get_sample_instance_gain},
    {"set_gain", lg_set_sample_instance_gain},
    {"get_pan", lg_get_sample_instance_pan},
    {"set_pan", lg_set_sample_instance_pan},
    {"get_time", lg_get_sample_instance_time},
    {"get_playmode", lg_get_sample_instance_playmode},
    {"set_playmode", lg_set_sample_instance_playmode},
    {"get_playing", lg_get_sample_instance_playing},
    {"set_playing", lg_set_sample_instance_playing},
    {"get_attached", lg_get_sample_instance_attached},
    {"detach", lg_detach_sample_instance},
    {"get_sample", lg_get_sample},
    {"set_sample", lg_set_sample},
    {NULL, NULL}
};

/*
================================================================================

                Audio Stream

================================================================================
*/
static ALLEGRO_AUDIO_STREAM *to_audio_stream( lua_State *L, const int idx ) {
    return (ALLEGRO_AUDIO_STREAM*) to_object(L, idx, LEGATO_AUDIO_STREAM);
}

static int audio_stream__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_AUDIO_STREAM, to_audio_stream(L, 1));
    return 1;
}

/*
static int audio_stream__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_AUDIO_STREAM);
}
*/

static const luaL_Reg audio_stream__methods[] = {
    {"__tostring", audio_stream__tostring},
    /*{"__eq", audio_stream__eq},*/
    {"__gc", lg_destroy_audio_stream},
    {"destroy", lg_destroy_audio_stream},
    {"drain", lg_drain_audio_stream},
    {"rewind", lg_rewind_audio_stream},
    {"get_frequency", lg_get_audio_stream_frequency},
    {"get_channels", lg_get_audio_stream_channels},
    {"get_depth", lg_get_audio_stream_depth},
    {"get_length", lg_get_audio_stream_length},
    {"get_speed", lg_get_audio_stream_speed},
    {"set_speed", lg_set_audio_stream_speed},
    {"get_gain", lg_get_audio_stream_gain},
    {"set_gain", lg_set_audio_stream_gain},
    {"get_pan", lg_get_audio_stream_pan},
    {"set_pan", lg_set_audio_stream_pan},
    {"get_playing", lg_get_audio_stream_playing},
    {"set_playing", lg_set_audio_stream_playing},
    {"get_playmode", lg_get_audio_stream_playmode},
    {"set_playmode", lg_set_audio_stream_playmode},
    {"get_attached", lg_get_audio_stream_attached},
    {"detach", lg_detach_audio_stream},
    {"seek_secs", lg_seek_audio_stream_secs},
    {"get_position_secs", lg_get_audio_stream_position_secs},
    {"get_length_secs", lg_get_audio_stream_length_secs},
    {"set_loop_secs", lg_set_audio_stream_loop_secs},
    {"attach_to_mixer", lg_attach_audio_stream_to_mixer},
    {NULL, NULL}
};

/*
================================================================================

                Font

================================================================================
*/
static ALLEGRO_FONT *to_font( lua_State *L, const int idx ) {
    return (ALLEGRO_FONT*) to_object(L, idx, LEGATO_FONT);
}

static int font__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_FONT, to_object_gc(L, 1, LEGATO_FONT));
    return 1;
}

/*
static int font__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_FONT);
}
*/

static const luaL_Reg font__methods[] = {
    {"__gc", lg_destroy_font},
    {"__tostring", font__tostring},
    /*{"__eq", font__eq},*/
    {"get_line_height", lg_get_font_line_height},
    {"get_ascent", lg_get_font_ascent},
    {"get_descent", lg_get_font_descent},
    {"get_text_width", lg_get_text_width},
    {"draw_text", lg_draw_text},
    {"draw_justified_text", lg_draw_justified_text},
    {"get_text_dimensions", lg_get_text_dimensions},
    {NULL, NULL}
};

/*
================================================================================

                PHYSFS - FUNCTIONS

================================================================================
*/
static int push_fs_list( lua_State *L, char **list ) {
    int i;
    char **it;
    lua_newtable(L);
    for ( i = 1, it = list; *it; it++, i++ ) {
        lua_pushinteger(L, i);
        lua_pushstring(L, *it);
        lua_settable(L, -3);
    }
    PHYSFS_freeList(list);
    return 1;
}

static int fs_get_supported_archive_types( lua_State *L ) {
    int i;
    const PHYSFS_ArchiveInfo **info;
    lua_newtable(L);
    for ( i = 1, info = PHYSFS_supportedArchiveTypes(); *info; info++, i++ ) {
        lua_newtable(L);
        set_str(L, "extension", (*info)->extension);
        set_str(L, "description", (*info)->description);
        set_str(L, "author", (*info)->author);
        set_str(L, "url", (*info)->url);
        lua_rawseti(L, -2, i);
    }
    return 1;
}

static int fs_get_dir_separator( lua_State *L ) {
    lua_pushstring(L, PHYSFS_getDirSeparator());
    return 1;
}

static int fs_permit_symbolic_links( lua_State *L ) {
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    PHYSFS_permitSymbolicLinks(lua_toboolean(L, 1));
    return 0;
}

static int fs_get_cdrom_dirs( lua_State *L ) {
    return push_fs_list(L, PHYSFS_getCdRomDirs());
}

static int fs_get_base_dir( lua_State *L ) {
    lua_pushstring(L, PHYSFS_getBaseDir());
    return 1;
}

static int fs_get_user_dir( lua_State *L ) {
    lua_pushstring(L, PHYSFS_getUserDir());
    return 1;
}

static int fs_get_write_dir( lua_State *L ) {
    lua_pushstring(L, PHYSFS_getWriteDir());
    return 1;
}

static int fs_set_write_dir( lua_State *L ) {
    const char *dir = luaL_checkstring(L, 1);
    al_set_standard_fs_interface();
    al_set_standard_file_interface();
    lua_pushboolean(L, PHYSFS_setWriteDir(dir));
    al_set_physfs_file_interface();
    return 1;
}

static int fs_remove_from_search_path( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_removeFromSearchPath(luaL_checkstring(L, 1)));
    return 1;
}

static int fs_get_search_path( lua_State *L ) {
    return push_fs_list(L, PHYSFS_getSearchPath());
}

static int fs_mkdir( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_mkdir(luaL_checkstring(L, 1)));
    return 1;
}

static int fs_delete( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_delete(luaL_checkstring(L, 1)));
    return 1;
}

static int fs_get_real_dir( lua_State *L ) {
    lua_pushstring(L, PHYSFS_getRealDir(luaL_checkstring(L, 1)));
    return 1;
}

static int fs_enumerate_files( lua_State *L ) {
    return push_fs_list(L, PHYSFS_enumerateFiles(luaL_checkstring(L, 1)));
}

static int fs_exists( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_exists(luaL_checkstring(L, 1)));
    return 1;
}

static int fs_is_directory( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_isDirectory(luaL_checkstring(L, 1)));
    return 1;
}

static int fs_is_symbolic_link( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_isSymbolicLink(luaL_checkstring(L, 1)));
    return 1;
}

static int fs_get_last_mod_time( lua_State *L ) {
    lua_pushinteger(L, PHYSFS_getLastModTime(luaL_checkstring(L, 1)));
    return 1;
}

static int fs_open_write( lua_State *L ) {
    return push_object(L, LEGATO_FILE, PHYSFS_openWrite(luaL_checkstring(L, 1)), 1);
}

static int fs_open_append( lua_State *L ) {
    return push_object(L, LEGATO_FILE, PHYSFS_openAppend(luaL_checkstring(L, 1)), 1);
}

static int fs_open_read( lua_State *L ) {
    return push_object(L, LEGATO_FILE, PHYSFS_openRead(luaL_checkstring(L, 1)), 1);
}

static int fs_close( lua_State *L ) {
    PHYSFS_File *fp = (PHYSFS_File*) to_object_gc(L, 1, LEGATO_FILE);
    if ( fp ) {
        PHYSFS_close(fp);
        clear_object(L, 1);
        return 0;
    }
    return 0;
}

static int fs_read( lua_State *L ) {
    luaL_Buffer buffer;
    size_t size;
    char *data;
    PHYSFS_sint64 read_bytes;
    PHYSFS_File *fp = to_file(L, 1);
    size = luaL_checkint(L, 2);
    luaL_buffinit(L, &buffer);
    data = luaL_prepbuffsize(&buffer, size);
    read_bytes = PHYSFS_read(fp, data, 1, size);
    if ( read_bytes >= 0 ) {
        luaL_addsize(&buffer, read_bytes);
        luaL_pushresult(&buffer);
        return 1;
    } else {
        return 0;
    }
}

static int fs_write( lua_State *L ) {
    const char *data;
    size_t size;
    PHYSFS_sint64 written_bytes;
    PHYSFS_File *fp = to_file(L, 1);
    data = luaL_checklstring(L, 2, &size);
    written_bytes = PHYSFS_write(fp, data, 1, size);
    if ( written_bytes >= 0 ) {
        lua_pushinteger(L, written_bytes);
        return 1;
    } else {
        return 0;
    }
}

static int fs_eof( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_eof(to_file(L, 1)));
    return 1;
}

static int fs_tell( lua_State *L ) {
    lua_pushinteger(L, PHYSFS_tell(to_file(L, 1)));
    return 1;
}

static int fs_seek( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_seek(to_file(L, 1), luaL_checkinteger(L, 2)));
    return 1;
}

static int fs_get_file_length( lua_State *L ) {
    lua_pushinteger(L, PHYSFS_fileLength(to_file(L, 1)));
    return 1;
}

static int fs_set_buffer_size( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_setBuffer(to_file(L, 1), luaL_checkinteger(L, 2)));
    return 1;
}

static int fs_flush( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_flush(to_file(L, 1)));
    return 1;
}

static int fs_symbolic_links_permitted( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_symbolicLinksPermitted());
    return 1;
}

static int fs_mount( lua_State *L ) {
    lua_pushboolean(L, PHYSFS_mount(luaL_checkstring(L, 1), luaL_optstring(L, 2, "/"), lua_toboolean(L, 3)));
    return 1;
}

static int fs_get_mount_point( lua_State *L ) {
    lua_pushstring(L, PHYSFS_getMountPoint(luaL_checkstring(L, 1)));
    return 1;
}

static const luaL_Reg fs__functions[] = {
    {"get_supported_archive_types", fs_get_supported_archive_types},
    {"get_dir_separator", fs_get_dir_separator},
    {"permit_symbolic_links", fs_permit_symbolic_links},
    {"get_cdrom_dirs", fs_get_cdrom_dirs},
    {"get_base_dir", fs_get_base_dir},
    {"get_user_dir", fs_get_user_dir},
    {"get_write_dir", fs_get_write_dir},
    {"set_write_dir", fs_set_write_dir},
    {"remove_from_search_path", fs_remove_from_search_path},
    {"get_search_path", fs_get_search_path},
    {"mkdir", fs_mkdir},
    {"delete", fs_delete},
    {"get_real_dir", fs_get_real_dir},
    {"enumerate_files", fs_enumerate_files},
    {"exists", fs_exists},
    {"is_directory", fs_is_directory},
    {"is_symbolic_link", fs_is_symbolic_link},
    {"get_last_mod_time", fs_get_last_mod_time},
    {"open_write", fs_open_write},
    {"open_append", fs_open_append},
    {"open_read", fs_open_read},
    {"close", fs_close},
    {"read", fs_read},
    {"write", fs_write},
    {"eof", fs_eof},
    {"tell", fs_tell},
    {"seek", fs_seek},
    {"get_file_length", fs_get_file_length},
    {"set_buffer_size", fs_set_buffer_size},
    {"flush", fs_flush},
    {"symbolic_links_permitted", fs_symbolic_links_permitted},
    {"mount", fs_mount},
    {"get_mount_point", fs_get_mount_point},
    {NULL, NULL}
};

/*
================================================================================

                PHYSFS - OBJECTS

================================================================================
*/
static PHYSFS_File *to_file( lua_State *L, const int idx ) {
    return (PHYSFS_File*) to_object(L, idx, LEGATO_FILE);
}

static int file__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_FILE, to_object_gc(L, 1, LEGATO_FILE));
    return 1;
}

/*
static int file__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_FILE);
}
*/

static const luaL_Reg file__methods[] = {
    {"__gc", fs_close},
    {"__tostring", file__tostring},
    /*{"__eq", file__eq},*/
    {"close", fs_close},
    {"read", fs_read},
    {"write", fs_write},
    {"eof", fs_eof},
    {"tell", fs_tell},
    {"seek", fs_seek},
    {"get_length", fs_get_file_length},
    {"set_buffer_size", fs_set_buffer_size},
    {"flush", fs_flush},
    {NULL, NULL}
};

/*
================================================================================

                ENET - FUNCTIONS

================================================================================
*/
/*
================================================================================

                Host

================================================================================
*/
static int lg_enet_compress_host_with_range_coder( lua_State *L ) {
    lua_pushboolean(L, enet_host_compress_with_range_coder(to_host(L, 1)) == 0);
    return 1;
}

static int lg_enet_create_host( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_enet_destroy_host( lua_State *L ) {
    ENetHost *host = (ENetHost*) to_object_gc(L, 1, LEGATO_HOST);
    if ( host ) {
        enet_host_destroy(host);
        clear_object(L, 1);
    }
    return 0;
}

static int lg_enet_connect_host( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_enet_broadcast_packet( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_enet_set_host_channel_limit( lua_State *L ) {
    enet_host_channel_limit(to_host(L, 1), luaL_checkinteger(L, 2));
    return 0;
}

static int lg_enet_get_host_channel_limit( lua_State *L ) {
    lua_pushinteger(L, to_host(L, 1)->channelLimit);
    return 1;
}

static int lg_enet_set_host_bandwidth_limit( lua_State *L ) {
    enet_host_bandwidth_limit(to_host(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3));
    return 0;
}

static int lg_enet_get_host_bandwidth_limit( lua_State *L ) {
    ENetHost *host = to_host(L, 1);
    lua_pushinteger(L, host->incomingBandwidth);
    lua_pushinteger(L, host->outgoingBandwidth);
    return 2;
}

static int lg_enet_flush_host( lua_State *L ) {
    enet_host_flush(to_host(L, 1));
    return 0;
}

static int lg_enet_check_host_events( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_enet_service_host( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

/*
================================================================================

                Peer

================================================================================
*/
static int lg_enet_set_peer_throttle( lua_State *L ) {
    enet_peer_throttle_configure(to_peer(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4));
    return 0;
}

static int lg_enet_get_peer_throttle( lua_State *L ) {
    ENetPeer *peer = to_peer(L, 1);
    lua_pushinteger(L, peer->packetThrottleInterval);
    lua_pushinteger(L, peer->packetThrottleAcceleration);
    lua_pushinteger(L, peer->packetThrottleDeceleration);
    return 3;
}

static int lg_enet_send_packet( lua_State *L ) {
    NOT_IMPLEMENTED_MACRO;
}

static int lg_enet_reset_peer( lua_State *L ) {
    enet_peer_reset(to_peer(L, 1));
    return 0;
}

static int lg_enet_ping_peer( lua_State *L ) {
    enet_peer_ping(to_peer(L, 1));
    return 0;
}

static int lg_enet_set_ping_interval( lua_State *L ) {
    enet_peer_ping_interval(to_peer(L, 1), luaL_checkinteger(L, 2));
    return 0;
}

static int lg_enet_get_ping_interval( lua_State *L ) {
    lua_pushinteger(L, to_peer(L, 1)->pingInterval);
    return 1;
}

static int lg_enet_set_timeout( lua_State *L ) {
    enet_peer_timeout(to_peer(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4));
    return 0;
}

static int lg_enet_get_timeout( lua_State *L ) {
    ENetPeer *peer = to_peer(L, 1);
    lua_pushinteger(L, peer->timeoutLimit);
    lua_pushinteger(L, peer->timeoutMinimum);
    lua_pushinteger(L, peer->timeoutMaximum);
    return 3;
}

static int lg_enet_disconnect_peer( lua_State *L ) {
    enet_peer_disconnect(to_peer(L, 1), luaL_optinteger(L, 2, 0));
    return 0;
}

static int lg_enet_disconnect_peer_now( lua_State *L ) {
    enet_peer_disconnect_now(to_peer(L, 1), luaL_optinteger(L, 2, 0));
    return 0;
}

static int lg_enet_disconnect_peer_later( lua_State *L ) {
    enet_peer_disconnect_later(to_peer(L, 1), luaL_optinteger(L, 2, 0));
    return 0;
}

static const luaL_Reg enet__functions[] = {
    {"compress_host_with_range_coder", lg_enet_compress_host_with_range_coder},
    {"create_host", lg_enet_create_host},
    {"connect_host", lg_enet_connect_host},
    {"broadcast_packet", lg_enet_broadcast_packet},
    {"set_host_channel_limit", lg_enet_set_host_channel_limit},
    {"get_host_channel_limit", lg_enet_get_host_channel_limit},
    {"get_host_bandwidth_limit", lg_enet_get_host_bandwidth_limit},
    {"set_host_bandwidth_limit", lg_enet_set_host_bandwidth_limit},
    {"flush_host", lg_enet_flush_host},
    {"check_host_events", lg_enet_check_host_events},
    {"service_host", lg_enet_service_host},

    {"set_peer_throttle", lg_enet_set_peer_throttle},
    {"get_peer_throttle", lg_enet_get_peer_throttle},
    {"send_packet", lg_enet_send_packet},
    {"reset_peer", lg_enet_reset_peer},
    {"ping_peer", lg_enet_ping_peer},
    {"set_ping_interval", lg_enet_set_ping_interval},
    {"get_ping_interval", lg_enet_get_ping_interval},
    {"set_timeout", lg_enet_set_timeout},
    {"get_timeout", lg_enet_get_timeout},
    {"disconnect_peer", lg_enet_disconnect_peer},
    {"disconnect_peer_now", lg_enet_disconnect_peer_now},
    {"disconnect_peer_later", lg_enet_disconnect_peer_later},

    {NULL, NULL}
};

/*
================================================================================

                ENET - OBJECTS

================================================================================
*/
/*
================================================================================

                Host

================================================================================
*/
static ENetHost *to_host( lua_State *L, const int idx ) {
    return (ENetHost*) to_object(L, idx, LEGATO_HOST);
}

static int host__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_HOST, to_object_gc(L, 1, LEGATO_HOST));
    return 1;
}

/*
static int host__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_HOST);
}
*/

static const luaL_Reg host__methods[] = {
    {"__tostring", host__tostring},
    /*{"__eq", host__eq},*/
    {"__gc", lg_enet_destroy_host},
    {NULL, NULL}
};

/*
================================================================================

                Peer

================================================================================
*/
static ENetPeer *to_peer( lua_State *L, const int idx ) {
    return (ENetPeer*) to_object(L, idx, LEGATO_PEER);
}

static int peer__tostring( lua_State *L ) {
    lua_pushfstring(L, "%s: %p", LEGATO_PEER, to_object_gc(L, 1, LEGATO_PEER));
    return 1;
}

/*
static int peer__eq( lua_State *L ) {
    return push_equal_check(L, LEGATO_PEER);
}
*/

static int peer__gc( lua_State *L ) {
    (void) to_peer(L, 1);
    clear_object(L, 1);
    return 0;
}

static const luaL_Reg peer__methods[] = {
    {"__tostring", peer__tostring},
    /*{"__eq", peer__eq},*/
    {"__gc", peer__gc},
    {"set_throttle", lg_enet_set_peer_throttle},
    {"get_throttle", lg_enet_get_peer_throttle},
    {"send_packet", lg_enet_send_packet},
    {"reset", lg_enet_reset_peer},
    {"ping", lg_enet_ping_peer},
    {"set_ping_interval", lg_enet_set_ping_interval},
    {"get_ping_interval", lg_enet_get_ping_interval},
    {"set_timeout", lg_enet_set_timeout},
    {"get_timeout", lg_enet_get_timeout},
    {"disconnect", lg_enet_disconnect_peer},
    {"disconnect_now", lg_enet_disconnect_peer_now},
    {"disconnect_later", lg_enet_disconnect_peer_later},
    {NULL, NULL}
};

/*
================================================================================

                BOOTSTRAPPING CODE

================================================================================
*/
static int luaopen_legato( lua_State *L ) {
    create_meta(L, LEGATO_CONFIG, config__methods);
    create_meta(L, LEGATO_DISPLAY, display__methods);
    create_meta(L, LEGATO_COLOR, color__methods);
    create_meta(L, LEGATO_BITMAP, bitmap__methods);
    create_meta(L, LEGATO_EVENT_QUEUE, event_queue__methods);
    create_meta(L, LEGATO_KEYBOARD_STATE, keyboard_state__methods);
    create_meta(L, LEGATO_MOUSE_STATE, mouse_state__methods);
    create_meta(L, LEGATO_STATE, state__methods);
    create_meta(L, LEGATO_TIMER, timer__methods);
    create_meta(L, LEGATO_TRANSFORM, transform__methods);
    create_meta(L, LEGATO_VOICE, voice__methods);
    create_meta(L, LEGATO_MIXER, mixer__methods);
    create_meta(L, LEGATO_AUDIO_SAMPLE, audio_sample__methods);
    create_meta(L, LEGATO_SAMPLE_ID, sample_id__methods);
    create_meta(L, LEGATO_SAMPLE_INSTANCE, sample_instance__methods);
    create_meta(L, LEGATO_AUDIO_STREAM, audio_stream__methods);
    create_meta(L, LEGATO_FONT, font__methods);
    create_meta(L, LEGATO_FILE, file__methods);
    lua_newtable(L);
    luaL_newlib(L, lg__functions);
    lua_setfield(L, -2, "al");
    luaL_newlib(L, fs__functions);
    lua_setfield(L, -2, "fs");
    return 1;
}

static int push_boot_script( lua_State *L ) {
    static const char *filenames[] = { "boot.lua", "boot.lc", "/scripts/boot.lua", "/script/boot.lc",
        "BOOT.LUA", "BOOT.LC", "BOOT", NULL };
    int i;
    for ( i = 0; filenames[i]; ++i ) {
        if ( PHYSFS_exists(filenames[i]) ) {
            lua_pushstring(L, filenames[i]);
            return 1;
        }
    }
    luaL_error(L, "unable to locate boot script");
    return 0;
}

static int boot_legato( lua_State *L ) {
    lua_pushcfunction(L, lg_load_script);
    push_boot_script(L);
    lua_call(L, 1, 1); /* load script */
    lua_call(L, 0, 0); /* run chunk */
    return 0;
}

static void mount_data( void ) {
    static const char *extensions[] = {".dat", ".7z", ".zip", ".wad", ".hog", ".grp", NULL};
    int i;
    ALLEGRO_PATH *path = al_get_standard_path(ALLEGRO_EXENAME_PATH);
    for ( i = 0; extensions[i]; ++i ) {
        al_set_path_extension(path, extensions[i]);
        PHYSFS_mount(al_path_cstr(path, ALLEGRO_NATIVE_PATH_SEP), "/", 0);
    }
    PHYSFS_mount("./data", "/", 0);
    al_destroy_path(path);
}

int main( int argc, char *argv[] ) {
    lua_State *L;

    PHYSFS_init(argv[0]);

    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_install_audio();
    al_init_image_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_acodec_addon();
    al_init_primitives_addon();
    al_set_physfs_file_interface();

    mount_data();

    L = luaL_newstate();
    luaL_openlibs(L);
    luaL_requiref(L, "legato", luaopen_legato, 1);
    create_object_table(L);
    lua_getglobal(L, "debug");
    lua_getfield(L, -1, "traceback");
    lua_remove(L, -2);
    lua_pushcfunction(L, boot_legato);
    if ( lua_pcall(L, 0, 0, -2) != LUA_OK ) {
        /* there might be room for improvement... */
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
    }
    lua_close(L);

    al_shutdown_primitives_addon();
    al_shutdown_ttf_addon();
    al_shutdown_font_addon();
    al_shutdown_image_addon();
    al_uninstall_audio();
    al_uninstall_system();

    PHYSFS_deinit();

    return EXIT_SUCCESS;
}
