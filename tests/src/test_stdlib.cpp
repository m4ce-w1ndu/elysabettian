#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <ely/stdlib/math.hpp>
#include <ely/stdlib/registry.hpp>
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

TEST_CASE("Stdlib module registry builtin loading and custom modules", "[stdlib][registry]") {
    ely::stdlib::ModuleRegistry registry;
    std::unordered_map<uint32_t, ely::stdlib::CallableNative> natives;
    std::unordered_map<uint32_t, ely::core::Value> globals;

    // Load built-in modules
    REQUIRE(registry.load("math", natives, globals));
    REQUIRE(registry.load("system", natives, globals));
    REQUIRE(registry.load("sys", natives, globals));
    REQUIRE(registry.load("io", natives, globals));
    REQUIRE(registry.load("stdio", natives, globals));
    REQUIRE_FALSE(registry.load("nonexistent_module", natives, globals));

    // Verify symbol hashing consistency
    auto hash_string = [](std::string_view str) noexcept -> uint32_t {
        uint32_t hash = 2166136261u;
        for (char c : str) {
            hash ^= static_cast<uint8_t>(c);
            hash *= 16777619u;
        }
        return hash;
    };

    const auto sqrt_id = hash_string("sqrt");
    REQUIRE(natives.contains(sqrt_id));
    REQUIRE(globals.contains(sqrt_id));

    // Invoke sqrt through the loaded native callable
    const ely::core::Value sqrt_arg[] = { ely::core::Value::number(81.0) };
    const auto sqrt_res = natives.at(sqrt_id).invoke(sqrt_arg);
    REQUIRE(sqrt_res.is_number());
    REQUIRE(sqrt_res.as_number() == 9.0);

    // Verify constants in globals
    const auto pi_id = hash_string("PI");
    REQUIRE(globals.contains(pi_id));
    REQUIRE(globals.at(pi_id).is_number());
    REQUIRE_THAT(globals.at(pi_id).as_number(), Catch::Matchers::WithinRel(std::numbers::pi, 1e-9));

    // Register and load a custom builtin module
    auto custom = ely::stdlib::NativeModule("custom");
    custom.add_fn("double_it", [](std::span<const ely::core::Value> args) {
        if (!args.empty() && args[0].is_number()) {
            return ely::core::Value::number(args[0].as_number() * 2.0);
        }
        return ely::core::Value::null();
    });
    custom.add_const("MAGIC", ely::core::Value::number(42.0));

    registry.register_builtin(std::move(custom));
    REQUIRE(registry.load("custom", natives, globals));

    const auto double_id = hash_string("double_it");
    REQUIRE(natives.contains(double_id));
    const ely::core::Value double_arg[] = { ely::core::Value::number(21.0) };
    REQUIRE(natives.at(double_id).invoke(double_arg).as_number() == 42.0);

    const auto magic_id = hash_string("MAGIC");
    REQUIRE(globals.contains(magic_id));
    REQUIRE(globals.at(magic_id).as_number() == 42.0);
}
