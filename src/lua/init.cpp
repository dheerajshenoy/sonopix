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

        sonifier->set_direction(direction);
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

    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, 1);
    return 0;
}

void
MainWindow::init_lua_sonopix_opts() noexcept
{
    lua_newtable(m_L); // sonopix.opts table

    // Metatable for sonopix to expose options as a readable/writable
    // property
    lua_newtable(m_L); // metatable

    // __index: intercept reads of known properties, fall back to rawget for
    // others
    lua_pushlightuserdata(m_L, this);
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (key && strcmp(key, "direction") == 0)
        {
            MainWindow *window = static_cast<MainWindow *>(
                lua_touserdata(L, lua_upvalueindex(1)));
            const char *dir_str = nullptr;
            switch (window->m_direction)
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

    // Set sonopix.opts in the sonopix global table
    lua_getglobal(m_L, "sonopix");
    lua_insert(m_L, -2);           // move sonopix below opts on the stack
    lua_setfield(m_L, -2, "opts"); // sonopix.opts = opts_table

    // Add __newindex to sonopix so `sonopix.opts = { ... }` applies each key
    lua_newtable(m_L); // metatable for sonopix
    lua_pushcclosure(m_L, [](lua_State *L) -> int
    {
        const char *key = lua_tostring(L, 2);
        if (key && strcmp(key, "opts") == 0 && lua_istable(L, 3))
        {
            lua_pushstring(L, "opts");
            lua_rawget(L, 1);         // stack[4] = sonopix.opts
            lua_pushnil(L);           // first key for lua_next
            while (lua_next(L, 3) != 0)
            {
                // stack: 4=opts, 5=k, 6=v
                lua_pushvalue(L, -2); // copy k
                lua_pushvalue(L, -2); // copy v
                lua_settable(L, 4);   // opts[k] = v, triggers opts __newindex
                lua_pop(L, 1);        // pop v, keep k for next iteration
            }
            return 0;
        }
        lua_rawset(L, 1);
        return 0;
    }, 0);
    lua_setfield(m_L, -2, "__newindex");
    lua_setmetatable(m_L, -2); // attach to sonopix
    lua_pop(m_L, 1);           // pop sonopix
}
