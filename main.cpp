#include "plug.hpp"
#include <stdio.h>
#include <Windows.h>
#include "normal.hpp"
#include <SFML/Graphics.hpp>

HMODULE load_plug(plug_init_t *plug_init, plug_update_t *plug_update);
void reload_plug(HMODULE* libplug, plug_init_t *plug_init, plug_update_t *plug_update);

int main()
{
    normal_function();
    
    plug_init_t plug_init = NULL;
    plug_update_t plug_update = NULL;

    // Load the plugin DLL
    HMODULE libplug = load_plug(&plug_init, &plug_update);
    if (!libplug)
    {
        return 1;
    }

    // Initialize plugin state
    PlugState state = {nullptr};
    plug_init(&state);

    if (!state.window)
    {
        printf("Failed to create window in plug_init\n");
        FreeLibrary(libplug);
        return 1;
    }

    // Main loop
    while (state.window->isOpen())
    {
        // Handle events
        while (const std::optional event = state.window->pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                state.window->close();
            }
            // Hot reload on 'R' key press
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::R)
                {
                    printf("\n=== Hot Reload Triggered ===\n");
                    reload_plug(&libplug, &plug_init, &plug_update);
                }
            }
        }

        // Call plugin update to render
        plug_update(&state);
    }

    // Cleanup
    if (state.window)
    {
        delete state.window;
    }
    FreeLibrary(libplug);
    return 0;
}

HMODULE load_plug(plug_init_t *plug_init, plug_update_t *plug_update)
{
    const char *libplug_file_name = "plug.dll";
    const char *libplug_temp_file_name = "plug_temp.dll";
    
    // Copy the DLL to a temp file so the original can be rebuilt
    if (!CopyFileA(libplug_file_name, libplug_temp_file_name, FALSE))
    {
        printf("Failed to copy %s to %s\n", libplug_file_name, libplug_temp_file_name);
        return NULL;
    }
    
    // Load from the temp copy
    HMODULE libplug = LoadLibraryA(libplug_temp_file_name);
    if (!libplug)
    {
        printf("Failed to load %s\n", libplug_temp_file_name);
        return NULL;
    }

    *plug_init = (plug_init_t)GetProcAddress(libplug, "plug_init");
    if (!*plug_init)
    {
        printf("Failed to get plug_init address.\n");
        FreeLibrary(libplug);
        return NULL;
    }

    *plug_update = (plug_update_t)GetProcAddress(libplug, "plug_update");
    if (!*plug_update)
    {
        printf("Failed to get plug_update address.\n");
        FreeLibrary(libplug);
        return NULL;
    }

    printf("Plugin loaded successfully from %s\n", libplug_temp_file_name);
    return libplug;
}

void reload_plug(HMODULE* libplug, plug_init_t *plug_init, plug_update_t *plug_update)
{
    // Unload the old DLL
    if (*libplug)
    {
        FreeLibrary(*libplug);
        printf("Old plugin unloaded\n");
        *libplug = NULL;
    }

    // Small delay to ensure file is released
    Sleep(100);

    // Reload the DLL (don't call plug_init - just reload the function pointers)
    *libplug = load_plug(plug_init, plug_update);
    if (*libplug)
    {
        printf("Plugin reloaded successfully! New code is active.\n");
    }
    else
    {
        printf("Failed to reload plugin!\n");
    }
}