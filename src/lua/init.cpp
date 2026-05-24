#include "MainWindow.hpp"
#include "utils.hpp"

#include <cstring>

void
MainWindow::init_lua(const std::string &script_file)
{
    m_L = luaL_newstate();
    luaL_openlibs(m_L);

    init_lua_sonopix();
    init_lua_sonopix_opts();

    if (luaL_dofile(m_L, script_file.c_str()) != LUA_OK)
    {
        std::string error_msg = lua_tostring(m_L, -1);
        throw std::runtime_error("Error executing Lua script: " + error_msg);
    }
}

void
MainWindow::init_lua_sonopix() noexcept
{
    // Create a new table for the sonopix module
    lua_newtable(m_L);

    // sonopix.open_file(filepath: str) -> boolean
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *filepath = luaL_checkstring(L, 1);
        try
        {
            MainWindow *window = static_cast<MainWindow *>(
                lua_touserdata(L, lua_upvalueindex(1)));
            window->open_file(filepath);
            lua_pushboolean(L, 1);
        }
        catch (const std::exception &e)
        {
            lua_pushboolean(L, 0);
            return luaL_error(L, "Error opening file: %s", e.what());
        }
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "open_file");

    // sonopix.file_path -> string or nil
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        std::string path = window->file_path();
        if (path.empty())
            lua_pushnil(L);
        else
            lua_pushstring(L, path.c_str());
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "file_path");

    // sonopix.sonify() -> boolean
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        try
        {
            MainWindow *window = static_cast<MainWindow *>(
                lua_touserdata(L, lua_upvalueindex(1)));
            bool status = window->sonify();
            lua_pushboolean(L, status);
        }
        catch (const std::exception &e)
        {
            lua_pushboolean(L, 0);
            return luaL_error(L, "Error during sonification: %s", e.what());
        }

        return 1;
    }, 1);
    lua_setfield(m_L, -2, "sonify");

    // sonopix.play()
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        try
        {
            MainWindow *window = static_cast<MainWindow *>(
                lua_touserdata(L, lua_upvalueindex(1)));
            window->play();
        }
        catch (const std::exception &e)
        {
            return luaL_error(L, "Error during playback: %s", e.what());
        }
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "play");

    // sonopix.pause()
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)))
            ->pause();
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "pause");

    // sonopix.stop()
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        try
        {
            MainWindow *window = static_cast<MainWindow *>(
                lua_touserdata(L, lua_upvalueindex(1)));
            window->stop();
        }
        catch (const std::exception &e)
        {
            return luaL_error(L, "Error during stop: %s", e.what());
        }
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "stop");

    // sonopix.is_playing() -> boolean
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        lua_pushboolean(L, window->m_audio_engine->is_playing() ? 1 : 0);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "is_playing");

    // sonopix.is_paused() -> boolean
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        lua_pushboolean(L, window->m_audio_engine->is_paused() ? 1 : 0);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "is_paused");

    // sonopix.is_stopped() -> boolean
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        lua_pushboolean(L, window->m_audio_engine->is_stopped() ? 1 : 0);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "is_stopped");

    // sonopix.on(event: string, fn: function)
    // Events: "sonify_complete", "file_loaded", "playback_end"
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *event = luaL_checkstring(L, 1);
        luaL_checktype(L, 2, LUA_TFUNCTION);
        MainWindow *window = static_cast<MainWindow *>(
            lua_touserdata(L, lua_upvalueindex(1)));
        lua_pushvalue(L, 2);
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        window->m_event_listeners[event].push_back(ref);
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "on");

    // sonopix.current_time() -> number
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        try
        {
            MainWindow *window = static_cast<MainWindow *>(
                lua_touserdata(L, lua_upvalueindex(1)));
            double current_time = window->m_audio_engine->sample_index();
            lua_pushnumber(L, current_time);
        }
        catch (const std::exception &e)
        {
            lua_pushnumber(L, 0);
            return luaL_error(L, "Error getting current time: %s", e.what());
        }
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "current_time");

    // sonopix.save_audio(filepath) -> boolean
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *filepath = luaL_checkstring(L, 1);
        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        lua_pushboolean(L, window->save_audio(filepath) ? 1 : 0);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "save_audio");

    // sonopix.audio_data() -> table or nil
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        const auto &audio_data = window->m_audio_engine->data();
        if (audio_data.empty())
        {
            lua_pushnil(L);
            return 1;
        }
        lua_newtable(L);
        for (size_t i = 0; i < audio_data.size(); ++i)
        {
            lua_pushnumber(L, audio_data[i]);
            lua_rawseti(L, -2, static_cast<int>(i + 1));
        }
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "audio_data");

    // sonopix.audio_dataf() -> table or nil
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        const auto &audio_data = window->m_audio_engine->dataf();
        if (audio_data.empty())
        {
            lua_pushnil(L);
            return 1;
        }
        lua_newtable(L);
        for (size_t i = 0; i < audio_data.size(); ++i)
        {
            lua_pushnumber(L, audio_data[i]);
            lua_rawseti(L, -2, static_cast<int>(i + 1));
        }
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "audio_data");

    // sonopix.pixel_brightness(x, y) -> number
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        const int x = static_cast<int>(luaL_checkinteger(L, 1));
        const int y = static_cast<int>(luaL_checkinteger(L, 2));
        lua_pushnumber(L, w->sonifier()->pixel_brightness_at(x, y));
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "pixel_brightness");

    // sonopix.set_roi(x, y, w, h) -- restrict sonification to a sub-region
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        const int x = static_cast<int>(luaL_checkinteger(L, 1));
        const int y = static_cast<int>(luaL_checkinteger(L, 2));
        const int rw = static_cast<int>(luaL_checkinteger(L, 3));
        const int rh = static_cast<int>(luaL_checkinteger(L, 4));
        w->m_sonifier->set_roi(x, y, rw, rh);
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "set_roi");

    // sonopix.clear_roi() -- remove any active ROI
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)))
            ->m_sonifier->clear_roi();
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "clear_roi");

    // Set the sonopix table in the global namespace
    lua_setglobal(m_L, "sonopix");
}

static std::string
sf_color_to_hex(const sf::Color &c)
{
    char buf[10];
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X", c.r, c.g, c.b, c.a);
    return std::string(buf);
}

static sf::Color
hex_to_sf_color(lua_State *L, int idx)
{
    const char *s = luaL_checkstring(L, idx);
    if (!s || s[0] != '#' || (strlen(s) != 7 && strlen(s) != 9))
        luaL_error(L, "color must be '#RRGGBB' or '#RRGGBBAA'");
    std::string hex(s + 1);
    if (hex.length() == 6)
        hex += "FF";
    return sf::Color(static_cast<std::uint32_t>(std::stoul(hex, nullptr, 16)));
}

static int
handle_lua_option(lua_State *L, const char *key, const char *value) noexcept
{
    MainWindow *window
        = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
    sonify::SonifyEngine *sonifier = window->sonifier();

    // sonopix.opts.direction
    if (strcmp(key, "direction") == 0)
    {
        const char *dir_str = luaL_checkstring(L, 3);
        sonify::Direction direction;
        if (strcmp(dir_str, "left-to-right") == 0)
            direction = sonify::Direction::LEFT_TO_RIGHT;
        else if (strcmp(dir_str, "right-to-left") == 0)
            direction = sonify::Direction::RIGHT_TO_LEFT;
        else if (strcmp(dir_str, "top-to-bottom") == 0)
            direction = sonify::Direction::TOP_TO_BOTTOM;
        else if (strcmp(dir_str, "bottom-to-top") == 0)
            direction = sonify::Direction::BOTTOM_TO_TOP;
        else if (strcmp(dir_str, "circle-outwards") == 0)
            direction = sonify::Direction::CIRCLE_OUTWARDS;
        else if (strcmp(dir_str, "circle-inwards") == 0)
            direction = sonify::Direction::CIRCLE_INWARDS;
        else if (strcmp(dir_str, "rotate-cw") == 0)
            direction = sonify::Direction::ROTATE_CW;
        else if (strcmp(dir_str, "rotate-ccw") == 0)
            direction = sonify::Direction::ROTATE_CCW;
        else
            return luaL_error(L, "Invalid direction: %s", dir_str);

        window->set_direction(direction);
        return 0;
    }

    // sonopix.opts.channel_count
    if (strcmp(key, "channel_count") == 0)
    {
        int count = static_cast<int>(luaL_checkinteger(L, 3));
        if (count <= 0)
            return luaL_error(L, "Invalid channel count: %d", count);
        window->audio_engine()->set_channel_count(count);
        window->sonifier()->set_channel_count(count);
        return 0;
    }

    // sonopix.opts.antialiasing_level
    if (strcmp(key, "antialiasing_level") == 0)
    {
        int level = static_cast<int>(luaL_checkinteger(L, 3));
        if (level < 0)
            return luaL_error(L, "antialiasing_level must be >= 0");
        window->set_antialiasing_level(static_cast<unsigned int>(level));
        return 0;
    }

    // sonopix.opts.amplitude
    if (strcmp(key, "amplitude") == 0)
    {
        float amp = static_cast<float>(luaL_checknumber(L, 3));
        if (amp < 0.0f)
            return luaL_error(L, "amplitude must be >= 0");
        window->set_amplitude(amp);
        return 0;
    }

    // sonopix.opts.spu
    if (strcmp(key, "spu") == 0)
    {
        float spu = static_cast<float>(luaL_checknumber(L, 3));
        if (spu <= 0.0f)
            return luaL_error(L, "Invalid seconds per unit: %f", spu);
        sonifier->set_secs_per_unit(spu);
        return 0;
    }

    // sonopix.opts.sample_rate
    if (strcmp(key, "sample_rate") == 0)
    {
        float rate = static_cast<float>(luaL_checknumber(L, 3));
        if (rate <= 0.0f)
            return luaL_error(L, "Invalid sample rate: %f", rate);
        sonifier->set_sample_rate(rate);
        return 0;
    }

    // sonopix.opts.show_progress_bar
    if (strcmp(key, "show_progress_bar") == 0)
    {
        luaL_checktype(L, 3, LUA_TBOOLEAN);
        window->set_show_progress_bar(lua_toboolean(L, 3) != 0);
        return 0;
    }

    // sonopix.opts.loop
    if (strcmp(key, "loop") == 0)
    {
        luaL_checktype(L, 3, LUA_TBOOLEAN);
        window->set_loop(lua_toboolean(L, 3) != 0);
        return 0;
    }

    // sonopix.opts.image_rotation
    if (strcmp(key, "image_rotation") == 0)
    {
        window->set_image_rotation(static_cast<float>(luaL_checknumber(L, 3)));
        return 0;
    }

    // sonopix.opts.traversal_func
    if (strcmp(key, "traversal_func") == 0)
    {
        if (!lua_isfunction(L, 3) && !lua_isnil(L, 3))
            return luaL_error(L, "traversal_func must be a function or nil");
        lua_pushvalue(L, 3);
        lua_setfield(L, LUA_REGISTRYINDEX, "sonopix_traversal_func");
        return 0;
    }

    // sonopix.opts.sonify_func
    if (strcmp(key, "sonify_func") == 0)
    {
        if (!lua_isfunction(L, 3))
            return luaL_error(L, "sonify_func must be a function");

        lua_pushvalue(L, 3);
        lua_setfield(L, LUA_REGISTRYINDEX, "sonopix_sonify_func");

        // Persistent context table — reused every call to avoid per-strip alloc
        lua_newtable(L);
        lua_setfield(L, LUA_REGISTRYINDEX, "sonopix_ctx_table");

        lua_State *Lc = L;
        window->sonifier()->set_sonify_func(
            [Lc](const sonify::SonifyContext &ctx, std::vector<float> &out)
        {
            lua_getfield(Lc, LUA_REGISTRYINDEX, "sonopix_sonify_func");
            lua_getfield(Lc, LUA_REGISTRYINDEX, "sonopix_ctx_table");

            lua_pushnumber(Lc, ctx.sample_rate);
            lua_setfield(Lc, -2, "sample_rate");
            lua_pushnumber(Lc, ctx.brightness);
            lua_setfield(Lc, -2, "brightness");
            lua_pushnumber(Lc, ctx.r);
            lua_setfield(Lc, -2, "r");
            lua_pushnumber(Lc, ctx.g);
            lua_setfield(Lc, -2, "g");
            lua_pushnumber(Lc, ctx.b);
            lua_setfield(Lc, -2, "b");
            lua_pushnumber(Lc, ctx.h);
            lua_setfield(Lc, -2, "h");
            lua_pushnumber(Lc, ctx.s);
            lua_setfield(Lc, -2, "s");
            lua_pushnumber(Lc, ctx.v);
            lua_setfield(Lc, -2, "v");
            lua_pushinteger(Lc, ctx.x);
            lua_setfield(Lc, -2, "x");
            lua_pushinteger(Lc, ctx.y);
            lua_setfield(Lc, -2, "y");
            lua_pushinteger(Lc, ctx.width);
            lua_setfield(Lc, -2, "width");
            lua_pushinteger(Lc, ctx.height);
            lua_setfield(Lc, -2, "height");
            lua_pushnumber(Lc, ctx.t);
            lua_setfield(Lc, -2, "t");
            lua_pushinteger(Lc, ctx.strip_index);
            lua_setfield(Lc, -2, "strip_index");
            lua_pushinteger(Lc, ctx.strip_count);
            lua_setfield(Lc, -2, "strip_count");
            lua_pushinteger(Lc, ctx.n_samples);
            lua_setfield(Lc, -2, "n_samples");
            lua_pushnumber(Lc, ctx.fmin);
            lua_setfield(Lc, -2, "fmin");
            lua_pushnumber(Lc, ctx.fmax);
            lua_setfield(Lc, -2, "fmax");

            const char *scale_str = "linear";
            if (ctx.freq_scale == sonify::FreqScale::LOG)
                scale_str = "log";
            else if (ctx.freq_scale == sonify::FreqScale::EXPONENTIAL)
                scale_str = "exponential";
            lua_pushstring(Lc, scale_str);
            lua_setfield(Lc, -2, "scale");
            lua_pushinteger(Lc, ctx.channel_count);
            lua_setfield(Lc, -2, "channel_count");

            const int total_out = ctx.n_samples * ctx.channel_count;

            if (lua_pcall(Lc, 1, 1, 0) != LUA_OK)
            {
                fprintf(stderr, "sonify_func error: %s\n",
                        lua_tostring(Lc, -1));
                lua_pop(Lc, 1);
                for (int i = 0; i < total_out; ++i)
                    out.push_back(0.0f);
                return;
            }

            if (!lua_istable(Lc, -1))
            {
                lua_pop(Lc, 1);
                for (int i = 0; i < total_out; ++i)
                    out.push_back(0.0f);
                return;
            }

            for (int i = 1; i <= total_out; ++i)
            {
                lua_rawgeti(Lc, -1, i);
                out.push_back(static_cast<float>(lua_tonumber(Lc, -1)));
                lua_pop(Lc, 1);
            }
            lua_pop(Lc, 1); // pop result table
        });
        return 0;
    }

    // sonopix.opts.cursor = { ... }  — forward each key to the cursor sub-table
    if (strcmp(key, "cursor") == 0)
    {
        if (!lua_istable(L, 3))
            return luaL_error(L, "cursor must be a table");
        lua_getfield(L, LUA_REGISTRYINDEX,
                     "sonopix_cursor_opts"); // [4] = cursor opts
        lua_pushnil(L);
        while (lua_next(L, 3) != 0)
        {
            lua_pushvalue(L, -2); // copy k
            lua_pushvalue(L, -2); // copy v
            lua_settable(L, 4);
            lua_pop(L, 1);
        }
        return 0;
    }

    // sonopix.opts.frequency = { ... } — forward each key to the frequency
    // sub-table
    if (strcmp(key, "frequency") == 0)
    {
        if (!lua_istable(L, 3))
            return luaL_error(L, "frequency must be a table");
        lua_getfield(L, LUA_REGISTRYINDEX,
                     "sonopix_frequency_opts"); // [4] = frequency opts
        lua_pushnil(L);
        while (lua_next(L, 3) != 0)
        {
            lua_pushvalue(L, -2); // copy k
            lua_pushvalue(L, -2); // copy v
            lua_settable(L, 4);
            lua_pop(L, 1);
        }
        return 0;
    }

    // sonopix.opts.waveform = { ... }
    if (strcmp(key, "waveform") == 0)
    {
        if (!lua_istable(L, 3))
            return luaL_error(L, "waveform must be a table");
        lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_waveform_opts");
        lua_pushnil(L);
        while (lua_next(L, 3) != 0)
        {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_settable(L, 4);
            lua_pop(L, 1);
        }
        return 0;
    }

    // sonopix.opts.oscilloscope = { ... }
    if (strcmp(key, "oscilloscope") == 0)
    {
        if (!lua_istable(L, 3))
            return luaL_error(L, "oscilloscope must be a table");
        lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_oscilloscope_opts");
        lua_pushnil(L);
        while (lua_next(L, 3) != 0)
        {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_settable(L, 4);
            lua_pop(L, 1);
        }
        return 0;
    }

    // sonopix.opts.progress_bar = { ... }
    if (strcmp(key, "progress_bar") == 0)
    {
        if (!lua_istable(L, 3))
            return luaL_error(L, "progress_bar must be a table");
        lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_progress_bar_opts");
        lua_pushnil(L);
        while (lua_next(L, 3) != 0)
        {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_settable(L, 4);
            lua_pop(L, 1);
        }
        return 0;
    }

    // sonopix.opts.audio_effects
    if (strcmp(key, "audio_effects") == 0)
    {
        if (!lua_istable(L, 3))
            return luaL_error(L, "audio_effects must be a table");
        lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_audio_effects_opts");
        lua_pushnil(L);
        while (lua_next(L, 3) != 0)
        {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_settable(L, 4);
            lua_pop(L, 1);
        }
        return 0;
    }

    // sonopix.opts.image_effects
    if (strcmp(key, "image_effects") == 0)
    {
        if (!lua_istable(L, 3))
            return luaL_error(L, "effects must be a table");
        lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_image_effects_opts");
        lua_pushnil(L);
        while (lua_next(L, 3) != 0)
        {
            lua_pushvalue(L, -2);
            lua_pushvalue(L, -2);
            lua_settable(L, 4);
            lua_pop(L, 1);
        }
        return 0;
    }

    // sonopix.opts.window_title
    if (strcmp(key, "window_title") == 0)
    {
        const char *title = luaL_checkstring(L, 3);
        window->set_window_title(title);
        return 0;
    }

    // sonopix.opts.window_size
    if (strcmp(key, "window_size") == 0)
    {
        if (!lua_istable(L, 3))
            return luaL_error(L, "window_size must be a table");
        lua_getfield(L, 3, "width");
        lua_getfield(L, 3, "height");
        if (!lua_isnumber(L, -2) || !lua_isnumber(L, -1))
        {
            lua_pop(L, 2);
            return luaL_error(L,
                              "window_size must have numeric width and height");
        }
        int width  = static_cast<int>(lua_tonumber(L, -2));
        int height = static_cast<int>(lua_tonumber(L, -1));
        lua_pop(L, 2);
        window->set_window_size(width, height);
        return 0;
    }

    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, 1);
    return 0;
}

void
MainWindow::init_lua_sonopix_opts() noexcept
{
    // --- cursor sub-table ---
    lua_newtable(m_L); // cursor opts table
    lua_newtable(m_L); // cursor opts metatable

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (key)
        {

            //  sonopix.opts.cursor.width
            if (strcmp(key, "width") == 0)
            {
                MainWindow *window = static_cast<MainWindow *>(
                    lua_touserdata(L, lua_upvalueindex(1)));
                lua_pushnumber(L, window->cursor_width());
                return 1;
            }

            // sonopix.opts.cursor.color
            if (strcmp(key, "color") == 0)
            {
                MainWindow *window = static_cast<MainWindow *>(
                    lua_touserdata(L, lua_upvalueindex(1)));
                std::string color_str = window->cursor_color();
                lua_pushstring(L, color_str.c_str());
                return 1;
            }
        }

        lua_pushvalue(L, 2);
        lua_rawget(L, 1);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "__index");

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));

        if (key)
        {

            //   sonopix.opts.cursor.width = number
            if (strcmp(key, "width") == 0)
            {
                float w = static_cast<float>(luaL_checknumber(L, 3));
                if (w <= 0.0f)
                    return luaL_error(L, "cursor.width must be positive");
                window->set_cursor_width(w);
                return 0;
            }

            // sonopix.opts.cursor.color = "#RRGGBB" or "#RRGGBBAA"
            else if (strcmp(key, "color") == 0)
            {
                const char *color_str = luaL_checkstring(L, 3);

                if (!color_str)
                    return luaL_error(L, "cursor.color must be a string");

                if (color_str[0] != '#'
                    || (strlen(color_str) != 7 && strlen(color_str) != 9))
                {
                    return luaL_error(
                        L,
                        "cursor.color must be in format #RRGGBB or #RRGGBBAA");
                }

                window->set_cursor_color(color_str);
                return 0;
            }
        }

        lua_pushvalue(L, 2);
        lua_pushvalue(L, 3);
        lua_rawset(L, 1);
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "__newindex");

    lua_setmetatable(m_L, -2); // attach metatable to cursor opts table
    lua_pushvalue(m_L, -1);
    lua_setfield(m_L, LUA_REGISTRYINDEX, "sonopix_cursor_opts");
    lua_pop(m_L, 1); // registry holds the ref; pop local copy

    // --- frequency sub-table ---
    lua_newtable(m_L); // frequency opts table
    lua_newtable(m_L); // frequency opts metatable

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);

        if (!key)
        {
            lua_pushnil(L);
            return 1;
        }

        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        sonify::FreqMap fm = window->sonifier()->freq_map();

        if (strcmp(key, "min") == 0)
        {
            lua_pushnumber(L, fm.min);
            return 1;
        }

        if (strcmp(key, "max") == 0)
        {
            lua_pushnumber(L, fm.max);
            return 1;
        }

        if (strcmp(key, "scale") == 0)
        {
            const char *s = nullptr;
            switch (fm.scale)
            {
                case sonify::FreqScale::LINEAR:
                    s = "linear";
                    break;
                case sonify::FreqScale::LOG:
                    s = "log";
                    break;
                case sonify::FreqScale::EXPONENTIAL:
                    s = "exponential";
                    break;
                default:
                    return luaL_error(L, "Invalid freq scale enum");
            }
            lua_pushstring(L, s);
            return 1;
        }
        lua_pushvalue(L, 2);
        lua_rawget(L, 1);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "__index");

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key)
            return 0;
        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        sonify::SonifyEngine *sonifier = window->sonifier();

        if (strcmp(key, "min") == 0)
        {
            float fmin = static_cast<float>(luaL_checknumber(L, 3));
            if (fmin <= 0.0f)
                return luaL_error(L, "frequency.fmin must be > 0");
            sonifier->set_freq_range(fmin, sonifier->freq_map().max);
            return 0;
        }

        if (strcmp(key, "max") == 0)
        {
            float fmax = static_cast<float>(luaL_checknumber(L, 3));
            if (fmax <= 0.0f)
                return luaL_error(L, "frequency.fmax must be > 0");
            sonifier->set_freq_range(sonifier->freq_map().min, fmax);
            return 0;
        }

        if (strcmp(key, "scale") == 0)
        {
            const char *scale_str = luaL_checkstring(L, 3);
            sonify::FreqScale scale;
            if (strcmp(scale_str, "linear") == 0)
                scale = sonify::FreqScale::LINEAR;
            else if (strcmp(scale_str, "log") == 0)
                scale = sonify::FreqScale::LOG;
            else if (strcmp(scale_str, "exponential") == 0)
                scale = sonify::FreqScale::EXPONENTIAL;
            else
                return luaL_error(L, "frequency.scale must be \"linear\", "
                                     "\"log\", or \"exponential\"");
            sonifier->set_freq_scale(scale);
            return 0;
        }
        lua_pushvalue(L, 2);
        lua_pushvalue(L, 3);
        lua_rawset(L, 1);
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "__newindex");

    lua_setmetatable(m_L, -2); // attach metatable to frequency opts table
    lua_pushvalue(m_L, -1);
    lua_setfield(m_L, LUA_REGISTRYINDEX, "sonopix_frequency_opts");
    lua_pop(m_L, 1);

    // --- waveform sub-table ---
    lua_newtable(m_L);
    lua_newtable(m_L);

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) { lua_pushnil(L); return 1; }
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        if (strcmp(key, "visible") == 0)
        {
            lua_pushboolean(L, w->m_config.waveform.visible ? 1 : 0);
            return 1;
        }
        if (strcmp(key, "height") == 0)
        {
            lua_pushnumber(L, w->m_config.waveform.height);
            return 1;
        }
        if (strcmp(key, "color") == 0)
        {
            lua_pushstring(L, sf_color_to_hex(w->m_config.waveform.color).c_str());
            return 1;
        }
        lua_pushvalue(L, 2); lua_rawget(L, 1);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "__index");

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) return 0;
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        if (strcmp(key, "visible") == 0)
        {
            luaL_checktype(L, 3, LUA_TBOOLEAN);
            w->m_config.waveform.visible = lua_toboolean(L, 3) != 0;
            return 0;
        }
        if (strcmp(key, "height") == 0)
        {
            float h = static_cast<float>(luaL_checknumber(L, 3));
            if (h <= 0.f) return luaL_error(L, "waveform.height must be > 0");
            w->m_config.waveform.height = h;
            if (w->m_tex_size.x > 0)
            {
                w->init_waveform();
                if (!w->m_audio_engine->dataf().empty())
                    w->build_waveform();
            }
            return 0;
        }
        if (strcmp(key, "color") == 0)
        {
            w->m_config.waveform.color = hex_to_sf_color(L, 3);
            if (w->m_tex_size.x > 0 && !w->m_audio_engine->dataf().empty())
                w->build_waveform();
            return 0;
        }
        lua_pushvalue(L, 2); lua_pushvalue(L, 3); lua_rawset(L, 1);
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "__newindex");

    lua_setmetatable(m_L, -2);
    lua_pushvalue(m_L, -1);
    lua_setfield(m_L, LUA_REGISTRYINDEX, "sonopix_waveform_opts");
    lua_pop(m_L, 1);

    // --- oscilloscope sub-table ---
    lua_newtable(m_L);
    lua_newtable(m_L);

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) { lua_pushnil(L); return 1; }
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        if (strcmp(key, "visible") == 0)
        {
            lua_pushboolean(L, w->m_config.oscilloscope.visible ? 1 : 0);
            return 1;
        }
        if (strcmp(key, "height") == 0)
        {
            lua_pushnumber(L, w->m_config.oscilloscope.height);
            return 1;
        }
        if (strcmp(key, "window_samples") == 0)
        {
            lua_pushinteger(L, w->m_config.oscilloscope.window_samples);
            return 1;
        }
        if (strcmp(key, "color") == 0)
        {
            lua_pushstring(L, sf_color_to_hex(w->m_config.oscilloscope.color).c_str());
            return 1;
        }
        lua_pushvalue(L, 2); lua_rawget(L, 1);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "__index");

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) return 0;
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        if (strcmp(key, "visible") == 0)
        {
            luaL_checktype(L, 3, LUA_TBOOLEAN);
            w->m_config.oscilloscope.visible = lua_toboolean(L, 3) != 0;
            return 0;
        }
        if (strcmp(key, "height") == 0)
        {
            float h = static_cast<float>(luaL_checknumber(L, 3));
            if (h <= 0.f) return luaL_error(L, "oscilloscope.height must be > 0");
            w->m_config.oscilloscope.height = h;
            if (w->m_tex_size.x > 0)
                w->init_oscilloscope();
            return 0;
        }
        if (strcmp(key, "window_samples") == 0)
        {
            int n = static_cast<int>(luaL_checkinteger(L, 3));
            if (n <= 0) return luaL_error(L, "oscilloscope.window_samples must be > 0");
            w->m_config.oscilloscope.window_samples = n;
            return 0;
        }
        if (strcmp(key, "color") == 0)
        {
            w->m_config.oscilloscope.color = hex_to_sf_color(L, 3);
            return 0;
        }
        lua_pushvalue(L, 2); lua_pushvalue(L, 3); lua_rawset(L, 1);
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "__newindex");

    lua_setmetatable(m_L, -2);
    lua_pushvalue(m_L, -1);
    lua_setfield(m_L, LUA_REGISTRYINDEX, "sonopix_oscilloscope_opts");
    lua_pop(m_L, 1);

    // --- progress_bar sub-table ---
    lua_newtable(m_L);
    lua_newtable(m_L);

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) { lua_pushnil(L); return 1; }
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        if (strcmp(key, "visible") == 0)
        {
            lua_pushboolean(L, w->m_config.progress_bar.visible ? 1 : 0);
            return 1;
        }
        if (strcmp(key, "height") == 0)
        {
            lua_pushnumber(L, w->m_config.progress_bar.height);
            return 1;
        }
        if (strcmp(key, "color") == 0)
        {
            lua_pushstring(L, sf_color_to_hex(w->m_config.progress_bar.color).c_str());
            return 1;
        }
        lua_pushvalue(L, 2); lua_rawget(L, 1);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "__index");

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) return 0;
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        if (strcmp(key, "visible") == 0)
        {
            luaL_checktype(L, 3, LUA_TBOOLEAN);
            w->m_config.progress_bar.visible = lua_toboolean(L, 3) != 0;
            return 0;
        }
        if (strcmp(key, "height") == 0)
        {
            float h = static_cast<float>(luaL_checknumber(L, 3));
            if (h <= 0.f) return luaL_error(L, "progress_bar.height must be > 0");
            w->m_config.progress_bar.height = h;
            if (w->m_tex_size.x > 0)
                w->init_playback_bar();
            return 0;
        }
        if (strcmp(key, "color") == 0)
        {
            w->m_config.progress_bar.color = hex_to_sf_color(L, 3);
            if (w->m_playback_fill)
                w->m_playback_fill->setFillColor(w->m_config.progress_bar.color);
            return 0;
        }
        lua_pushvalue(L, 2); lua_pushvalue(L, 3); lua_rawset(L, 1);
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "__newindex");

    lua_setmetatable(m_L, -2);
    lua_pushvalue(m_L, -1);
    lua_setfield(m_L, LUA_REGISTRYINDEX, "sonopix_progress_bar_opts");
    lua_pop(m_L, 1);

    // --- audio_effects sub-table ---
    lua_newtable(m_L);
    lua_newtable(m_L);

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) { lua_pushnil(L); return 1; }
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        const auto &ae = w->m_config.audio_effects;

        if (strcmp(key, "gain") == 0) { lua_pushnumber(L, ae.gain); return 1; }

        // delay sub-table
        if (strcmp(key, "delay") == 0)
        {
            lua_newtable(L);
            lua_pushnumber(L, ae.delay_time);     lua_setfield(L, -2, "time");
            lua_pushnumber(L, ae.delay_feedback);  lua_setfield(L, -2, "feedback");
            lua_pushnumber(L, ae.delay_mix);       lua_setfield(L, -2, "mix");
            return 1;
        }
        // reverb sub-table
        if (strcmp(key, "reverb") == 0)
        {
            lua_newtable(L);
            lua_pushnumber(L, ae.reverb_room);    lua_setfield(L, -2, "room_size");
            lua_pushnumber(L, ae.reverb_damping); lua_setfield(L, -2, "damping");
            lua_pushnumber(L, ae.reverb_mix);     lua_setfield(L, -2, "mix");
            return 1;
        }
        // distortion sub-table
        if (strcmp(key, "distortion") == 0)
        {
            lua_newtable(L);
            lua_pushnumber(L, ae.distortion_drive); lua_setfield(L, -2, "drive");
            lua_pushnumber(L, ae.distortion_mix);   lua_setfield(L, -2, "mix");
            return 1;
        }
        // process_func
        if (strcmp(key, "process_func") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_audio_process_func");
            return 1;
        }

        lua_pushvalue(L, 2); lua_rawget(L, 1);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "__index");

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) return 0;
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        auto &ae = w->m_config.audio_effects;

        if (strcmp(key, "gain") == 0)
        {
            ae.gain = static_cast<float>(luaL_checknumber(L, 3));
            return 0;
        }
        if (strcmp(key, "delay") == 0)
        {
            if (!lua_istable(L, 3)) return luaL_error(L, "delay must be a table");
            lua_getfield(L, 3, "time");
            if (lua_isnumber(L, -1)) ae.delay_time = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            lua_getfield(L, 3, "feedback");
            if (lua_isnumber(L, -1)) ae.delay_feedback = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            lua_getfield(L, 3, "mix");
            if (lua_isnumber(L, -1)) ae.delay_mix = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            return 0;
        }
        if (strcmp(key, "reverb") == 0)
        {
            if (!lua_istable(L, 3)) return luaL_error(L, "reverb must be a table");
            lua_getfield(L, 3, "room_size");
            if (lua_isnumber(L, -1)) ae.reverb_room = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            lua_getfield(L, 3, "damping");
            if (lua_isnumber(L, -1)) ae.reverb_damping = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            lua_getfield(L, 3, "mix");
            if (lua_isnumber(L, -1)) ae.reverb_mix = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            return 0;
        }
        if (strcmp(key, "distortion") == 0)
        {
            if (!lua_istable(L, 3)) return luaL_error(L, "distortion must be a table");
            lua_getfield(L, 3, "drive");
            if (lua_isnumber(L, -1)) ae.distortion_drive = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            lua_getfield(L, 3, "mix");
            if (lua_isnumber(L, -1)) ae.distortion_mix = static_cast<float>(lua_tonumber(L, -1));
            lua_pop(L, 1);
            return 0;
        }
        if (strcmp(key, "process_func") == 0)
        {
            if (lua_isfunction(L, 3))
            {
                lua_pushvalue(L, 3);
                lua_setfield(L, LUA_REGISTRYINDEX, "sonopix_audio_process_func");
                w->m_config.audio_effects.has_process_func = true;
            }
            else
            {
                lua_pushnil(L);
                lua_setfield(L, LUA_REGISTRYINDEX, "sonopix_audio_process_func");
                w->m_config.audio_effects.has_process_func = false;
            }
            return 0;
        }
        lua_pushvalue(L, 2); lua_pushvalue(L, 3); lua_rawset(L, 1);
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "__newindex");

    lua_setmetatable(m_L, -2);
    lua_pushvalue(m_L, -1);
    lua_setfield(m_L, LUA_REGISTRYINDEX, "sonopix_audio_effects_opts");
    lua_pop(m_L, 1);

    // --- image_effects sub-table ---
    lua_newtable(m_L);
    lua_newtable(m_L);

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) { lua_pushnil(L); return 1; }
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        const auto &e = w->m_config.image_effects;
        if (strcmp(key, "grayscale")  == 0) { lua_pushnumber(L, e.grayscale);  return 1; }
        if (strcmp(key, "brightness") == 0) { lua_pushnumber(L, e.brightness); return 1; }
        if (strcmp(key, "saturation") == 0) { lua_pushnumber(L, e.saturation); return 1; }
        if (strcmp(key, "contrast")   == 0) { lua_pushnumber(L, e.contrast);   return 1; }
        if (strcmp(key, "hue")        == 0) { lua_pushnumber(L, e.hue);        return 1; }
        if (strcmp(key, "blur")       == 0) { lua_pushnumber(L, e.blur);       return 1; }
        if (strcmp(key, "sharpen")    == 0) { lua_pushnumber(L, e.sharpen);    return 1; }
        if (strcmp(key, "threshold")  == 0) { lua_pushnumber(L, e.threshold);  return 1; }
        if (strcmp(key, "invert")     == 0) { lua_pushboolean(L, e.invert ? 1 : 0); return 1; }
        lua_pushvalue(L, 2); lua_rawget(L, 1);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "__index");

    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key) return 0;
        MainWindow *w = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));
        if (strcmp(key, "invert") == 0)
        {
            luaL_checktype(L, 3, LUA_TBOOLEAN);
            w->set_effect_invert(lua_toboolean(L, 3) != 0);
            return 0;
        }
        // All other fields are floats
        w->set_effect(key, static_cast<float>(luaL_checknumber(L, 3)));
        return 0;
    }, 1);
    lua_setfield(m_L, -2, "__newindex");

    lua_setmetatable(m_L, -2);
    lua_pushvalue(m_L, -1);
    lua_setfield(m_L, LUA_REGISTRYINDEX, "sonopix_image_effects_opts");
    lua_pop(m_L, 1);

    // --- opts table ---
    lua_newtable(m_L); // sonopix.opts table
    lua_newtable(m_L); // opts metatable

    // __index: intercept reads of known properties, fall back to rawget for
    // others
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (!key)
        {
            lua_pushnil(L);
            return 1;
        }

        MainWindow *window
            = static_cast<MainWindow *>(lua_touserdata(L, lua_upvalueindex(1)));

        // sonopix.opts.direction
        if (strcmp(key, "direction") == 0)
        {
            const char *dir_str = nullptr;
            switch (window->direction())
            {
                case sonify::Direction::LEFT_TO_RIGHT:
                    dir_str = "left-to-right";
                    break;
                case sonify::Direction::RIGHT_TO_LEFT:
                    dir_str = "right-to-left";
                    break;
                case sonify::Direction::TOP_TO_BOTTOM:
                    dir_str = "top-to-bottom";
                    break;
                case sonify::Direction::BOTTOM_TO_TOP:
                    dir_str = "bottom-to-top";
                    break;
                case sonify::Direction::CIRCLE_OUTWARDS:
                    dir_str = "circle-outwards";
                    break;
                case sonify::Direction::CIRCLE_INWARDS:
                    dir_str = "circle-inwards";
                    break;
                case sonify::Direction::ROTATE_CW:
                    dir_str = "rotate-cw";
                    break;
                case sonify::Direction::ROTATE_CCW:
                    dir_str = "rotate-ccw";
                    break;
                default:
                    return luaL_error(L, "Invalid direction enum value");
            }
            lua_pushstring(L, dir_str);
            return 1;
        }

        // sonopix.opts.spu
        if (strcmp(key, "spu") == 0)
        {
            sonify::SonifyEngine *sonifier = window->sonifier();
            lua_pushnumber(L, sonifier->secs_per_unit());
            return 1;
        }

        // sonopix.opts.sample_rate
        if (strcmp(key, "sample_rate") == 0)
        {
            sonify::SonifyEngine *sonifier = window->sonifier();
            lua_pushnumber(L, sonifier->sample_rate());
            return 1;
        }

        // sonopix.opts.cursor
        if (strcmp(key, "cursor") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_cursor_opts");
            return 1;
        }

        // sonopix.opts.frequency
        if (strcmp(key, "frequency") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_frequency_opts");
            return 1;
        }

        // sonopix.opts.window_title
        if (strcmp(key, "window_title") == 0)
        {
            lua_pushstring(L, window->window_title().c_str());
            return 1;
        }

        // sonopix.opts.window_size
        if (strcmp(key, "window_size") == 0)
        {
            auto winsize = window->window_size();
            lua_newtable(L);
            lua_pushnumber(L, winsize.x);
            lua_setfield(L, -2, "width");
            lua_pushnumber(L, winsize.y);
            lua_setfield(L, -2, "height");
            return 1;
        }

        // sonopix.opts.channel_count
        if (strcmp(key, "channel_count") == 0)
        {
            lua_pushinteger(L, window->audio_engine()->channel_count());
            return 1;
        }

        // sonopix.opts.antialiasing_level
        if (strcmp(key, "antialiasing_level") == 0)
        {
            lua_pushinteger(
                L, static_cast<lua_Integer>(window->antialiasing_level()));
            return 1;
        }

        // sonopix.opts.waveform
        if (strcmp(key, "waveform") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_waveform_opts");
            return 1;
        }

        // sonopix.opts.oscilloscope
        if (strcmp(key, "oscilloscope") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_oscilloscope_opts");
            return 1;
        }

        // sonopix.opts.progress_bar
        if (strcmp(key, "progress_bar") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_progress_bar_opts");
            return 1;
        }

        // sonopix.opts.image_effects
        if (strcmp(key, "image_effects") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_image_effects_opts");
            return 1;
        }

        // sonopix.opts.audio_effects
        if (strcmp(key, "audio_effects") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_audio_effects_opts");
            return 1;
        }

        // sonopix.opts.show_progress_bar
        if (strcmp(key, "show_progress_bar") == 0)
        {
            lua_pushboolean(L, window->show_progress_bar() ? 1 : 0);
            return 1;
        }

        // sonopix.opts.loop
        if (strcmp(key, "loop") == 0)
        {
            lua_pushboolean(L, window->loop() ? 1 : 0);
            return 1;
        }

        // sonopix.opts.image_rotation
        if (strcmp(key, "image_rotation") == 0)
        {
            lua_pushnumber(L, static_cast<lua_Number>(window->image_rotation()));
            return 1;
        }

        // sonopix.opts.traversal_func
        if (strcmp(key, "traversal_func") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_traversal_func");
            return 1;
        }

        // sonopix.opts.amplitude
        if (strcmp(key, "amplitude") == 0)
        {
            lua_pushnumber(L, static_cast<lua_Number>(window->amplitude()));
            return 1;
        }

        // sonopix.opts.sonify_func
        if (strcmp(key, "sonify_func") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_sonify_func");
            return 1;
        }

        lua_pushvalue(L, 2);
        lua_rawget(L, 1);
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "__index");

    // __newindex: intercept writes of known properties, fall back to rawset for
    // others
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    { return handle_lua_option(L, lua_tostring(L, 2), nullptr); }, 1);
    lua_setfield(m_L, -2, "__newindex");

    lua_setmetatable(m_L, -2); // attach metatable to opts table

    // Store opts in registry so __index/__newindex on sonopix can reach it.
    // We intentionally do NOT rawset "opts" into the sonopix table: if the key
    // existed there, Lua would bypass __newindex on `sonopix.opts = {...}`.
    lua_pushvalue(m_L, -1); // duplicate opts
    lua_setfield(m_L, LUA_REGISTRYINDEX, "sonopix_opts");

    lua_getglobal(m_L, "sonopix");

    lua_newtable(m_L); // metatable for sonopix

    // __index: return opts from registry; fall back to rawget for other keys
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (key && strcmp(key, "opts") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_opts");
            return 1;
        }
        lua_pushvalue(L, 2);
        lua_rawget(L, 1);
        return 1;
    }, 0);
    lua_setfield(m_L, -2, "__index");

    // __newindex: handle `sonopix.opts = { ... }` by forwarding each key
    // through opts's own __newindex (which calls handle_lua_option)
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (key && strcmp(key, "opts") == 0 && lua_istable(L, 3))
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_opts"); // [4] = opts
            lua_pushnil(L);
            while (lua_next(L, 3) != 0)
            {
                // stack: [4]=opts, [5]=k, [6]=v
                lua_pushvalue(L, -2); // copy k
                lua_pushvalue(L, -2); // copy v
                lua_settable(L, 4);   // opts[k] = v → triggers opts __newindex
                lua_pop(L, 1);        // pop v, keep k for lua_next
            }
            return 0;
        }
        lua_rawset(L, 1);
        return 0;
    }, 0);
    lua_setfield(m_L, -2, "__newindex");

    lua_setmetatable(m_L, -2); // attach metatable to sonopix
    lua_pop(m_L, 2);           // pop sonopix and opts
}
