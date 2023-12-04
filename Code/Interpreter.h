#include "lib/JobSystem.h"
#include <set>
#include <string>

class Interpreter
{
public:
    Interpreter(std::string folder, JobSystem *copiedJs, json input, std::set<std::string> jobList, std::vector<std::string> jobFolders);
    ~Interpreter(){};
    void Parse();
    void Run();

private:
    json input;
    std::string FSFolder;
    JobSystem *js;
    std::vector<std::string> tokens;
    std::set<std::string> defaultJobs;
    std::set<std::string> jobs;
    std::map<std::string, std::set<std::string>> dependencyMap;
    std::map<std::string, std::string> conditionMap;
    std::vector<std::string> jobFolders;
};