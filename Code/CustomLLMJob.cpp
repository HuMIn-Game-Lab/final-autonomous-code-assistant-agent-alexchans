#include "CustomLLMJob.h"

CustomLLMJob::CustomLLMJob(json input)
{
    m_jobType = input["jobType"].get<int>();
    m_jobChannels = input["jobChannel"].get<unsigned long>();
    errorJSFolder = input["folder"].get<std::string>();
    jobTypeName = input["newJobType"].get<std::string>();
    prompt = input["prompt"].get<std::string>();
    IPAddress = input["IPAddress"].get<std::string>();
}

Job *CustomLLMJob::CreateCustomJob(json input)
{
    return (Job *)(new CustomLLMJob(input));
}

void CustomLLMJob::Execute()
{
    std::array<char, 128> buffer;
    // set command to run llm.js from node
    std::string command = "node --no-deprecation ./Code/llm \"" + prompt + "\" \"" + errorJSFolder + "\"";
    //  Redirect cerr to cout
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
        this->output.append((buffer.data()));
    }
    // close pipe and get return code
    this->returnCode = pclose(pipe);
    std::cout << "Job " << this->GetUniqueID() << " Has Been Executed" << std::endl;
}

void CustomLLMJob::JobCompleteCallback()
{
    // Write the output to a txt file
    std::ofstream o("Data/correctedCode.json");
    o << output << std::endl;
}
