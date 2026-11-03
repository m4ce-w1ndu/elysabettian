#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <ely/stdlib/math.hpp>
#include <ely/stdlib/system.hpp>
#include <numbers>
#include <cmath>

TEST_CASE("Stdlib math module constants and function registration", "[stdlib][math]") {
    const auto math = ely::stdlib::make_math_module();
    REQUIRE(math.name() == "math");

    // Verify constants
    auto find_const = [&](std::string_view name) -> std::optional<ely::core::Value> {
        for (const auto& [c_name, c_val] : math.constants()) {
            if (c_name == name) return c_val;
        }
        return std::nullopt;
    };

    const auto pi = find_const("PI");
    REQUIRE(pi.has_value());
    REQUIRE(pi->is_number());
    REQUIRE_THAT(pi->as_number(), Catch::Matchers::WithinRel(std::numbers::pi, 1e-9));

    const auto tau = find_const("TAU");
    REQUIRE(tau.has_value());
    REQUIRE(tau->is_number());
    REQUIRE_THAT(tau->as_number(), Catch::Matchers::WithinRel(2.0 * std::numbers::pi, 1e-9));

    const auto e = find_const("E");
    REQUIRE(e.has_value());
    REQUIRE(e->is_number());
    REQUIRE_THAT(e->as_number(), Catch::Matchers::WithinRel(std::numbers::e, 1e-9));

    const auto inf = find_const("INFINITY");
    REQUIRE(inf.has_value());
    REQUIRE(inf->is_number());
    REQUIRE(std::isinf(inf->as_number()));
}

TEST_CASE("Stdlib math functions evaluation", "[stdlib][math]") {
    const auto math = ely::stdlib::make_math_module();

    auto call = [&](std::string_view name, std::span<const ely::core::Value> args) -> ely::core::Value {
        for (const auto& [fn_name, fn] : math.functions()) {
            if (fn_name == name) {
                return fn(args);
            }
        }
        return ely::core::Value::null();
    };

    // sqrt
    {
        const ely::core::Value args[] = { ely::core::Value::number(144.0) };
        const auto res = call("sqrt", args);
        REQUIRE(res.is_number());
        REQUIRE(res.as_number() == 12.0);
    }

    // pow
    {
        const ely::core::Value args[] = { ely::core::Value::number(2.0), ely::core::Value::number(5.0) };
        const auto res = call("pow", args);
        REQUIRE(res.is_number());
        REQUIRE(res.as_number() == 32.0);
    }

    // abs
    {
        const ely::core::Value args[] = { ely::core::Value::number(-42.5) };
        const auto res = call("abs", args);
        REQUIRE(res.is_number());
        REQUIRE(res.as_number() == 42.5);
    }

    // floor, ceil, round, trunc
    {
        const ely::core::Value args[] = { ely::core::Value::number(3.7) };
        REQUIRE(call("floor", args).as_number() == 3.0);
        REQUIRE(call("ceil", args).as_number() == 4.0);
        REQUIRE(call("round", args).as_number() == 4.0);
        REQUIRE(call("trunc", args).as_number() == 3.0);
    }

    // hypot
    {
        const ely::core::Value args[] = { ely::core::Value::number(3.0), ely::core::Value::number(4.0) };
        const auto res = call("hypot", args);
        REQUIRE(res.is_number());
        REQUIRE(res.as_number() == 5.0);
    }

    // min / max
    {
        const ely::core::Value args[] = { ely::core::Value::number(10.0), ely::core::Value::number(20.0) };
        REQUIRE(call("min", args).as_number() == 10.0);
        REQUIRE(call("max", args).as_number() == 20.0);
    }

    // log, log10, log2, exp
    {
        const ely::core::Value exp_arg[] = { ely::core::Value::number(1.0) };
        REQUIRE_THAT(call("exp", exp_arg).as_number(), Catch::Matchers::WithinRel(std::numbers::e, 1e-9));

        const ely::core::Value log_arg[] = { ely::core::Value::number(std::numbers::e) };
        REQUIRE_THAT(call("log", log_arg).as_number(), Catch::Matchers::WithinRel(1.0, 1e-9));

        const ely::core::Value log10_arg[] = { ely::core::Value::number(1000.0) };
        REQUIRE_THAT(call("log10", log10_arg).as_number(), Catch::Matchers::WithinRel(3.0, 1e-9));

        const ely::core::Value log2_arg[] = { ely::core::Value::number(1024.0) };
        REQUIRE_THAT(call("log2", log2_arg).as_number(), Catch::Matchers::WithinRel(10.0, 1e-9));
    }
}

TEST_CASE("Stdlib math invalid arguments return null", "[stdlib][math]") {
    const auto math = ely::stdlib::make_math_module();

    auto call = [&](std::string_view name, std::span<const ely::core::Value> args) -> ely::core::Value {
        for (const auto& [fn_name, fn] : math.functions()) {
            if (fn_name == name) {
                return fn(args);
            }
        }
        return ely::core::Value::null();
    };

    // 0 args passed to sqrt
    REQUIRE(call("sqrt", {}).is_null());

    // Non-numeric passed to sqrt
    const ely::core::Value str_arg[] = { ely::core::Value::null() };
    REQUIRE(call("sqrt", str_arg).is_null());

    // 1 arg passed to pow (requires 2)
    const ely::core::Value one_arg[] = { ely::core::Value::number(2.0) };
    REQUIRE(call("pow", one_arg).is_null());

    // Second arg non-numeric in pow
    const ely::core::Value mixed_args[] = { ely::core::Value::number(2.0), ely::core::Value::null() };
    REQUIRE(call("pow", mixed_args).is_null());
}

TEST_CASE("Stdlib system and io module functions", "[stdlib][system]") {
    const auto sys = ely::stdlib::make_system_module();
    REQUIRE(sys.name() == "system");

    const auto io = ely::stdlib::make_io_module();
    REQUIRE(io.name() == "io");

    auto call = [&](const ely::stdlib::NativeModule& mod, std::string_view name, std::span<const ely::core::Value> args) -> ely::core::Value {
        for (const auto& [fn_name, fn] : mod.functions()) {
            if (fn_name == name) {
                return fn(args);
            }
        }
        return ely::core::Value::null();
    };

    // clock() and time() should return a valid epoch timestamp > 0
    const auto t1 = call(sys, "clock", {});
    REQUIRE(t1.is_number());
    REQUIRE(t1.as_number() > 1700000000.0);

    const auto t2 = call(sys, "time", {});
    REQUIRE(t2.is_number());
    REQUIRE(t2.as_number() >= t1.as_number());

    // io module alias contains the same functions
    const auto t_io = call(io, "clock", {});
    REQUIRE(t_io.is_number());
    REQUIRE(t_io.as_number() > 0.0);

    // sleep(0.001) should sleep and return null
    const ely::core::Value sleep_arg[] = { ely::core::Value::number(0.001) };
    const auto sleep_res = call(sys, "sleep", sleep_arg);
    REQUIRE(sleep_res.is_null());

    // flush() should return null
    const auto flush_res = call(sys, "flush", {});
    REQUIRE(flush_res.is_null());
}
