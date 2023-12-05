#include "Agent.h"
using json = nlohmann::json;

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
    // Create default jobsystem
    std::cout << "Creating Job System" << std::endl;
    JobSystem *js = JobSystem::CreateOrGet();
    // Create default worker thread
    std::cout << "Creating Worker Thread" << std::endl;
    json input;
    input["threadName"] = "Thread1";
    input["threadChannel"] = 0xF0000000;
    js->CreateWorkerThread(input);
    input["threadName"] = "Thread2";
    input["threadChannel"] = 0x0000000F;
    js->CreateWorkerThread(input);

    // jobList used in interpreter to check if users had entered correct job names
    std::set<std::string> jobList;
    std::vector<std::string> jobFolders;

    // Register Jobs
    // Register compileJob1
    input["newJobType"] = "compileJob1";
    jobList.insert("compileJob1");
    input["jobChannel"] = 0xF0000000;
    input["folder"] = "errorCode";
    jobFolders.push_back("errorCode");
    input["jobType"] = 1;
    js->registerNewJobType(input, CustomJob::CreateCustomJob);

    // Register llmJob
    input["newJobType"] = "llmJob";
    jobList.insert("llmJob");
    input["jobChannel"] = 0xF0000000;
    input["folder"] = "Data/errors.json";
    jobFolders.push_back("Data/errors.json");
    input["prompt"] = "Given a JSON file presenting errors in C++ files, where each error is accompanied by a Code Snippet providing the error line in the middle and 2 lines above/below it, correct the error lines. Provide only the corrected lines in separated lines, and include comments explaining how each error is fixed. The response should not be in JSON format.";
    input["jobType"] = 2;
    input["IPAddress"] = "127.0.0.1";
    js->registerNewJobType(input, CustomLLMJob::CreateCustomJob);

    // Register compileJob2
    input["newJobType"] = "compileJob2";
    jobList.insert("compileJob2");
    input["jobChannel"] = 0xF0000000;
    input["folder"] = "errorCode";
    jobFolders.push_back("errorCode");
    input["jobType"] = 3;
    js->registerNewJobType(input, CustomJob::CreateCustomJob);

    // Create FS File
    CreateFSFile();

    // Use the interpreter to execute/finish the jobs
    Interpreter interpreter("Data/FS.json", js, input, jobList, jobFolders);
    interpreter.Parse();
    interpreter.Run();

    // Destory the job system
    std::cout << "Total jobs completed " << js->totalJobs << std::endl;
    js->Destroy();
    std::cout << "Job system is destroyed" << std::endl;
}