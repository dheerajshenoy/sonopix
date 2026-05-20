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
        try
        {
            MainWindow *window = static_cast<MainWindow *>(
                lua_touserdata(L, lua_upvalueindex(1)));
            bool is_playing = window->m_audio_engine->is_playing();
            lua_pushboolean(L, is_playing);
        }
        catch (const std::exception &e)
        {
            lua_pushboolean(L, 0);
            return luaL_error(L, "Error checking playback status: %s",
                              e.what());
        }
        return 1;
    }, 1);
    lua_setfield(m_L, -2, "is_playing");

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

    // Set the sonopix table in the global namespace
    lua_setglobal(m_L, "sonopix");
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

    // sonopix.opts.frequency_range
    if (strcmp(key, "frequency_range") == 0)
    {
        if (!lua_istable(L, 3))
            return luaL_error(L,
                              "frequency_range must be a table {fmin, fmax}");
        lua_rawgeti(L, 3, 1);
        lua_rawgeti(L, 3, 2);
        float fmin = static_cast<float>(luaL_checknumber(L, -2));
        float fmax = static_cast<float>(luaL_checknumber(L, -1));
        lua_pop(L, 2);
        if (fmin <= 0.0f || fmax <= fmin)
            return luaL_error(L, "frequency_range requires 0 < fmin < fmax");
        sonifier->set_freq_range(fmin, fmax);
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
            lua_settable(L,
                         4); // cursor_opts[k] = v → triggers cursor __newindex
            lua_pop(L, 1);
        }
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

        // sonopix.opts.frequency_range
        if (strcmp(key, "frequency_range") == 0)
        {
            sonify::FreqMap fm = window->sonifier()->freq_map();
            lua_newtable(L);
            lua_pushnumber(L, fm.min);
            lua_rawseti(L, -2, 1);
            lua_pushnumber(L, fm.max);
            lua_rawseti(L, -2, 2);
            return 1;
        }

        // sonopix.opts.cursor
        if (strcmp(key, "cursor") == 0)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, "sonopix_cursor_opts");
            return 1;
        }

        // sonopix.opts.channel_count
        if (strcmp(key, "channel_count") == 0)
        {
            lua_pushinteger(L, window->audio_engine()->channel_count());
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
