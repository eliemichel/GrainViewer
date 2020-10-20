#include "EnvironmentVariables.h"

#include <regex>

std::shared_ptr<EnvironmentVariables> EnvironmentVariables::s_instance;

std::shared_ptr<EnvironmentVariables> EnvironmentVariables::GetInstance() noexcept
{
    if (!s_instance) {
        s_instance = std::make_shared<EnvironmentVariables>();
    }
    return s_instance;
}

std::string EnvironmentVariables::eval(const std::string value) const
{
    return std::regex_replace(value, std::regex("\\$BASEFILE"), baseFile);
}
