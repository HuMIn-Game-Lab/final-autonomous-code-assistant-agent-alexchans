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
    // Write the output to a json file
    std::ofstream o("Data/correctedCode.json");
    o << output << std::endl;
    // go through and json file with the correctedCodes/comments, get the necessary contents and save to a map called dMap
    std::ifstream file("Data/correctedCode.json");
    std::vector<std::string> fileContent;
    std::string aLine;
    while (std::getline(file, aLine))
    {
        if (aLine.find(':') != std::string::npos)
            fileContent.push_back(aLine);
    }
    std::map<std::string, std::map<int, std::vector<std::string>>> dMap;
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
            dMap[filePath][row].push_back(correctContents);
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
            dMap[filePath][row].push_back(correctContents);
        }
    }
    for (auto pair : dMap)
    {
        std::ifstream errorFile(pair.first);
        std::vector<std::string> errorFileContent;
        std::string errorLine;
        while (std::getline(errorFile, errorLine))
        {
            errorFileContent.push_back(errorLine);
        }
        for (auto i : pair.second)
        {
            for (int j = 0; j < errorFileContent.size(); j++)
            {
                if (j + 1 == i.first)
                {
                    std::string codeWithComments = i.second[0] + " //" + i.second[1];
                    codeWithComments.erase(std::remove(codeWithComments.begin(), codeWithComments.end(), '\\'), codeWithComments.end());
                    errorFileContent[j] = codeWithComments;
                }
            }
        }
        errorFile.close();
        std::ofstream fixedFile(pair.first, std::ofstream::trunc);
        for (int i = 0; i < errorFileContent.size(); i++)
        {
            fixedFile << errorFileContent[i] << std::endl;
        }
        fixedFile.close();
    }
}
