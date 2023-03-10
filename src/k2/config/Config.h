/*
MIT License

Copyright(c) 2020 Futurewei Cloud

    Permission is hereby granted,
    free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

    The above copyright notice and this permission notice shall be included in all copies
    or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS",
    WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER
    LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

// k2
#include <k2/common/Common.h>

// third-party
#include <boost/program_options.hpp>
#include <boost/spirit/include/qi.hpp>
#include <seastar/core/distributed.hh>  // for distributed<>

namespace k2 {
namespace config {
typedef boost::program_options::variables_map BPOVarMap;
typedef seastar::distributed<BPOVarMap> BPOConfigMapDist_t;
}  // ns config

// for convenient access to globally initialized configuration
inline config::BPOConfigMapDist_t ___config___;
inline config::BPOConfigMapDist_t& ConfigDist() { return ___config___; }
inline const config::BPOVarMap& Config() { return ___config___.local(); }

// Helper class used to read configuration values in code.
// To use, declare a variable for your configuration, e.g.:
// ConfigVar<int> retries("retries", 10);
//
// Then later in the code when you want to read the configured value, just use the variable as a functor
// for(int i = 0; i < retries() << ++i) {
// }
template<typename T>
class ConfigVar {
public:
    ConfigVar(String name, T defaultValue=T{}) {
        if (Config().count(name)) {
            _val = Config()[name].as<T>();
        }
        else {
            _val = std::move(defaultValue);
        }
    }
    ~ConfigVar(){}
    const T& operator()() const {
        return _val;
    }
private:
    T _val;
};

// This class is parseable via BoostProgramOptions to allow users to accept human-readable (think chrono literals)
// durations, e.g. 1ms, 1us, 21h
// The code which adds the config option should add an option of type ParseableDuration
// Then to read these, use the ConfigDuration class below
struct ParseableDuration {
    Duration value;
};

template <class charT>
void validate(boost::any& v, const std::vector<std::basic_string<charT>>& xs, ParseableDuration*, long) {
    boost::program_options::validators::check_first_occurrence(v);
    std::basic_string<charT> s(boost::program_options::validators::get_single_string(xs));

    int magnitude;
    Duration factor;

    namespace qi = boost::spirit::qi;
    qi::symbols<char, Duration> unit;
    unit.add("ns", 1ns)("us", 1us)("??s", 1us)("ms", 1ms)("s", 1s)("m", 1min)("h", 1h);

    if (parse(s.begin(), s.end(), qi::int_ >> unit >> qi::eoi, magnitude, factor))
        v = ParseableDuration{magnitude * factor};
    else
        throw boost::program_options::invalid_option_value(s);
}

// Allows for easy reading of Duration variables. The user registers a ParsedDuration in the program options
// then they create a ConfigDuration variable. e.g.
// ConfigDuration _timeout("timeout", 10ms);
//...
// if (clock.now() - start > _timeout()) throw std::runtime_error("timeout has occurred");
class ConfigDuration: public ConfigVar<ParseableDuration> {
public:
    ConfigDuration(String name, Duration defaultDuration) :
        ConfigVar(std::move(name), ParseableDuration{defaultDuration}) {
    }
    virtual ~ConfigDuration() {}
    const Duration& operator()() const {
        return ConfigVar::operator()().value;
    }
};

} //ns k2
