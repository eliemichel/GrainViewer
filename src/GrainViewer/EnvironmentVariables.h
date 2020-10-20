#pragma once

#include <string>
#include <memory>

/**
 * Environement variables are used for substitution of patterns like $BASEFILE in input strings
 */
struct EnvironmentVariables {
public:
    // static API
    static std::shared_ptr<EnvironmentVariables> GetInstance() noexcept;
    static std::string Eval(const std::string value) { return GetInstance()->eval(value); }

public:
    EnvironmentVariables() {}
    ~EnvironmentVariables() {}
    EnvironmentVariables& operator=(const EnvironmentVariables&) = delete;
    EnvironmentVariables(const EnvironmentVariables&) = delete;

    std::string eval(const std::string value) const;

public:
	std::string baseFile;

private:
    static std::shared_ptr<EnvironmentVariables> s_instance;
};
