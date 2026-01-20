#include "application.hpp"

#include <gio/gio.h>

auto main(int argc, char* argv[]) -> int
{
    // Register the GResource
    auto* resource = typetrace_get_resource();
    g_resources_register(resource);

    auto app = typetrace::frontend::Application::create();
    const int status = app->run(argc, argv);

    // Unregister the GResource
    g_resources_unregister(resource);

    return status;
}
