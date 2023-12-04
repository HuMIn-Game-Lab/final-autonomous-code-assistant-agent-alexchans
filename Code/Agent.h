#include "lib/JobSystem.h"
#include "Interpreter.h"

class Agent
{
public:
    Agent(std::string input);
    void CreateFSFile();
    void Run();

private:
    std::string FSPrompt;
    std::string FSOutput;
};