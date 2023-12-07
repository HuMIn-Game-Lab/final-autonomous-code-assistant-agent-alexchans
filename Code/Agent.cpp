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
    // Create default worker thread, 2 threads used for compilejob and llmjob
    std::cout << "Creating Worker Thread" << std::endl;
    json input;
    input["threadName"] = "Thread1";
    input["threadChannel"] = 0xF0000000; // compilejob thread
    js->CreateWorkerThread(input);
    input["threadName"] = "Thread2";
    input["threadChannel"] = 0x0000000F; // llmjob thread
    js->CreateWorkerThread(input);
    while (true) // continue create/run jobs until code is fixed
    {
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
        input["jobChannel"] = 0x0000000F;
        input["folder"] = "Data/errors.json";
        jobFolders.push_back("Data/errors.json");
        input["prompt"] = "Given a JSON file called errors.json representing errors in C++ code, where each error is accompanied by a Code Snippet providing the error line along with 2 lines above/below it, correct the error lines. Provide a JSON file with two objects: correctedCodes and comments. correctedCodes should have one corrected line as an index, and comments should include a comment for each corrected line, explaining how it was fixed. I will show you errors.json first, then the json format that I would like you to return. Your final output should be a properly formatted json object with no additional information. ";
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
        // Continue run flowscript until code is fixed
        Interpreter interpreter("Data/FS.json", js, input, jobList, jobFolders);
        interpreter.Parse();
        interpreter.Run();
        std::ifstream file("Data/result.json");
        if (file.is_open()) // check whether the second compile job return no errors
        {
            break;
        }
    }
    // Destory the job system
    std::cout << "Total jobs completed " << js->totalJobs << std::endl;
    js->Destroy();
    std::cout << "Job system is destroyed" << std::endl;
}
