#include "lib/Job.h"
#include "lib/nlohmann/json.hpp"
#include <iostream>
#include <string>
#include <array>
#include <fstream>
#include <vector>
using json = nlohmann::json;

class CustomLLMJob : public Job
{
public:
    CustomLLMJob(json input);
    ~CustomLLMJob(){};
    void Execute();
    static Job *CreateCustomJob(json input);
    void JobCompleteCallback();
    std::string errorJSFolder;
    std::string jobTypeName;
    std::string IPAddress;
    std::string prompt;
    std::string output;
    int returnCode;
};
