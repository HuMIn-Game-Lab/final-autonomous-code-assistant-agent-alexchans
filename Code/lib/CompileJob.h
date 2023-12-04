#include "Job.h"
#include <iostream>
#include <string>
#include <array>
#include <fstream>
#include <vector>
#include "nlohmann/json.hpp"
using json = nlohmann::json;

class CompileJob : public Job
{
public:
    CompileJob(unsigned long jobChannels, int jobType) : Job(jobChannels, jobType){};
    ~CompileJob(){};
    std::string output;
    int returnCode;
    std::string folder;
    void Execute();
    void JobCompleteCallback();
};