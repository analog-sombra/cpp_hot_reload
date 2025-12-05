#ifndef __PLUG_H__
#define __PLUG_H__

#include <SFML/Graphics.hpp>

// Plugin state structure to hold SFML window and objects
struct PlugState
{
    sf::RenderWindow* window;
};

extern "C"
{
    __declspec(dllexport) void plug_init(PlugState* state);
    __declspec(dllexport) void plug_update(PlugState* state);

    typedef void (*plug_init_t)(PlugState*);
    typedef void (*plug_update_t)(PlugState*);
}

#endif // __PLUG_H__