#include "Agent.h"

Agent::Agent(std::string input)
{
    FSPrompt = input;
}

// Create FlowScript File based on user input
void Agent::CreateFSFile()
{
    std::array<char, 128> buffer;
    // set command to the directory of the code automatically by using string folder
    std::string command = "node --no-deprecation ./Code/llm \"" + FSPrompt + "\"";
    // Redirect cerr to cout
    command.append(" 2>&1");
    // open pipe and run command
    FILE *pipe = popen(command.c_str(), "r");

    if (!pipe)
    {
        std::cout << "popen failed: failed to open pipe" << std::endl;
        return;
    }

    // read till end of process
    while (fgets(buffer.data(), 128, pipe) != NULL)
    {
        FSOutput.append((buffer.data()));
    }
    pclose(pipe);
    std::ofstream o("Data/FS.json");
    o << FSOutput << std::endl;
}

void Agent::Run()
{
    CreateFSFile();
}
