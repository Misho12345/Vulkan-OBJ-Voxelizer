#include "ComputeApp.hpp"
#include "Boza/Logger.hpp"

using namespace boza;

int main()
{
    if (![]
    {
        ComputeApp app(128, 64, 128);

        BOZA_CHECK(app, "Failed to initialize ComputeApp");
        BOZA_CHECK(app.run(), "Failed to run ComputeApp");

        return true;
    }()) return -1;
}
