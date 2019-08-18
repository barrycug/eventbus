#include "mpm/eventbus.h"
#include "mpm/enable_polymorphic_dispatch.h"
#include <catch2/catch.hpp>


struct base_event : mpm::enable_polymorphic_dispatch<base_event>
{
    int i = 1;
};

struct derived_event
    : mpm::enable_polymorphic_dispatch<derived_event, base_event>
{
    int j = 2;
};


struct very_derived_event : derived_event
{
    int k = 3;
};


class non_polymorphic_event
{
  public:
    non_polymorphic_event()
    {
    }


    int l = 4;

  private:
    non_polymorphic_event(const non_polymorphic_event&);
    non_polymorphic_event& operator=(const non_polymorphic_event&);
};

// todo C++14 - all event handlers can use const auto&

TEST_CASE("base event", "[mpm.eventbus]")
{
    mpm::eventbus ebus;
    int calls = 0;
    mpm::scoped_subscription<base_event> ss(ebus,
        [&](const base_event&) noexcept { calls++; }
    );
    ebus.publish(base_event());
    CHECK(1 == calls);
}


TEST_CASE("derived event", "[mpm.eventbus]")
{
    mpm::eventbus ebus;
    int calls = 0;
    auto c1 = ebus.subscribe<base_event>(
        [&](const base_event&) noexcept { calls++; }
    );
    auto c2 = ebus.subscribe<derived_event>(
        [&](const derived_event&) noexcept { calls++; }
    );
    ebus.publish(base_event());
    CHECK(1 == calls);
    ebus.publish(derived_event());
    CHECK(3 == calls);
    ebus.unsubscribe(c1);
    ebus.publish(derived_event());
    CHECK(4 == calls);
    ebus.unsubscribe(c2);
    ebus.publish(derived_event());
    CHECK(4 == calls);
}


TEST_CASE("very derived event", "[mpm.eventbus]")
{
    mpm::eventbus ebus;
    int derived_calls = 0;
    int very_derived_calls = 0;
    mpm::scoped_subscription<very_derived_event> ssvde(ebus,
        [&](const very_derived_event&) noexcept { very_derived_calls ++; }
    );
    mpm::scoped_subscription<derived_event> ssde(ebus,
        [&](const derived_event&) noexcept { derived_calls ++; }
    );

    ebus.publish(base_event());
    CHECK(0 == derived_calls);
    CHECK(0 == very_derived_calls);

    ebus.publish(derived_event());
    CHECK(1 == derived_calls);
    CHECK(0 == very_derived_calls);

    ebus.publish(very_derived_event());
    CHECK(1 == very_derived_calls);
    CHECK(2 == derived_calls);
}


TEST_CASE("non polymorphic event", "[mpm.eventbus]")
{
    mpm::eventbus ebus;
    int calls = 0;
    mpm::scoped_subscription<non_polymorphic_event> ssnpe(ebus,
        [&](const non_polymorphic_event&) noexcept { calls++; }
    );

    ebus.publish(non_polymorphic_event());
    CHECK(1 == calls);
}


TEST_CASE("raii", "[mpm.eventbus]")
{
    mpm::eventbus ebus;
    int calls = 0;
    {
        mpm::scoped_subscription<base_event> ss(ebus,
            [&](const base_event&) noexcept { calls++; }
        );
        ebus.publish(base_event());
        CHECK(1 == calls);
    }
    ebus.publish(base_event());
    CHECK(1 == calls); // no new call - subscription went out of scope

    mpm::scoped_subscription<base_event> ss;
    ss.assign(ebus,
        [&](const base_event&) noexcept { calls++; }
    );
    ebus.publish(base_event());
    CHECK(2 == calls);

    {
        mpm::scoped_subscription<non_polymorphic_event> ss;
        //just a test that we don't segfault when destructing a
        //scoped_subscription that is "empty"
    }
}


TEST_CASE("publish non-class type", "[mpm.eventbus]")
{
    mpm::eventbus ebus;
    int calls = 0;
    bool b = false;
    auto intsub = mpm::scoped_subscription<int> {
        ebus, [&](int i) noexcept { calls += i; }
    };
    auto boolsub = mpm::scoped_subscription<bool> {
        ebus, [&](bool event) noexcept { b = event; }
    };
    ebus.publish(12);
    ebus.publish(true);
    CHECK(12 == calls);
    CHECK(b);
}


TEST_CASE("publish pointer", "[mpm.eventbus]")
{
    mpm::eventbus ebus;
    non_polymorphic_event npe;
    auto subscription = mpm::scoped_subscription<non_polymorphic_event*> {
        ebus, [&](non_polymorphic_event* ptr) noexcept { CHECK(ptr == &npe); }
    };
    ebus.publish(&npe);
}
