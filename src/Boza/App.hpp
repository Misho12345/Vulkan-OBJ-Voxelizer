#pragma once

namespace boza
{
    class App
    {
    public:
        explicit App(const std::string_view& name);
        virtual  ~App();

        virtual bool run() = 0;

        operator bool() const { return !hasThrown; }

    protected:
        bool hasThrown{ false };
    };
}
