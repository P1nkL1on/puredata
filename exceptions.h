#pragma once

#include <stdexcept>


namespace {

struct constraint_violated : std::runtime_error
{
    constraint_violated(const std::string &msg = "") : std::runtime_error(msg) {}
};

#define EXPECT(condition) \
if (!(condition)) throw constraint_violated(__FILE__ ":" + std::to_string(__LINE__) + ": constraint violated: " #condition)


struct bad_io : std::runtime_error
{
    bad_io(const std::string &msg = "") : std::runtime_error(msg) {}
};


}