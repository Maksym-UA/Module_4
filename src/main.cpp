#include "application.h"

extern "C" void app_main(void)
{
    static Application app;
    app.start();
}
