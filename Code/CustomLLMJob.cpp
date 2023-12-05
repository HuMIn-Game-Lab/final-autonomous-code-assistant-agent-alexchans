#include "CustomLLMJob.h"
#include <set>

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
    // Write the output to a json file
    std::ofstream o("Data/correctedCode.json");
    o << output << std::endl;
    // parse the response to a vector
    std::ifstream file("Data/correctedCode.json");
    std::vector<std::string> fileContent;
    std::string aLine;
    while (std::getline(file, aLine))
    {
        if (aLine.find(':') != std::string::npos)
            fileContent.push_back(aLine);
    }
    std::map<std::string, std::map<int, std::set<std::string>>> dMap;
    std::map<int, std::set<std::string>> innerMap;
    std::string filePath;
    for (int i = 0; i < fileContent.size(); i++)
    {
        if (i > 0 && fileContent[i - 1].find("./Code") != std::string::npos)
        {
            filePath = fileContent[i - 1];
            filePath = filePath.substr(filePath.find("./Code"), filePath.find(":") - 8);
            std::string correctContents = fileContent[i];
            size_t lastNonSpace = correctContents.find_last_not_of(" \t\n\r\f\v");
            correctContents.erase(lastNonSpace + 1);
            char num = correctContents[correctContents.find("\"") + 1];
            if (correctContents[correctContents.length() - 1] == ',')
            {
                correctContents = correctContents.substr(correctContents.find(":") + 3, correctContents.length() - (correctContents.find(":") + 5));
            }
            else
            {
                correctContents = correctContents.substr(correctContents.find(":") + 3, correctContents.length() - (correctContents.find(":") + 4));
            }
            int row = num - '0';
            innerMap[row].insert(correctContents);
            dMap[filePath] = innerMap;
        }
        else if (i > 0 && fileContent[i - 1].find(",") != std::string::npos && fileContent[i].find("./Code") == std::string::npos)
        {
            std::string correctContents = fileContent[i];
            size_t lastNonSpace = correctContents.find_last_not_of(" \t\n\r\f\v");
            correctContents.erase(lastNonSpace + 1);
            char num = correctContents[correctContents.find("\"") + 1];
            if (correctContents[correctContents.length() - 1] == ',')
            {
                correctContents = correctContents.substr(correctContents.find(":") + 3, correctContents.length() - (correctContents.find(":") + 5));
            }
            else
            {
                correctContents = correctContents.substr(correctContents.find(":") + 3, correctContents.length() - (correctContents.find(":") + 4));
            }
            int row = num - '0';
            innerMap[row].insert(correctContents);
            dMap[filePath] = innerMap;
        }
    }
    for (auto pair : dMap)
    {
        std::cout << pair.first << ": " << std::endl;
        for (auto i : pair.second)
        {
            std::cout << i.first << ": ";
            for (auto z : i.second)
            {
                std::cout << z << " ";
            }
            std::cout << std::endl;
        }
    }
}
