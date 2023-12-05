#include "Interpreter.h"

Interpreter::Interpreter(std::string folder, JobSystem *copiedJs, json input, std::set<std::string> jobList, std::vector<std::string> folders)
{
    FSFolder = folder;
    js = copiedJs;
    this->input = input;
    defaultJobs = jobList;
    jobFolders = folders;
}

// Lexical and Syntactic Parsing
void Interpreter::Parse()
{
    // Lexical Parsing, break each line in FSFile into tokens and push the tokens into the tokens vector, and push jobs to jobs set
    std::ifstream FSFile(FSFolder);
    std::string aLine;
    int jobID = 0;
    std::string conditionJob;
    bool right = false;
    while (std::getline(FSFile, aLine))
    {
        if (aLine.find("{") == std::string::npos && aLine.find("}") == std::string::npos) // Read in only contents of the flowscript
        {
            aLine.erase(std::remove(aLine.begin(), aLine.end(), ' '), aLine.end()); // erase all spaces
            if (aLine.find("[") != std::string::npos)                               // look for condition declarations
            {
                std::string conditionType = aLine.substr(0, aLine.find("["));
                tokens.push_back(conditionType);
                tokens.push_back("[");
                aLine = aLine.substr(aLine.find("[") + 1, aLine.length());
                std::string shape = aLine.substr(0, aLine.find("="));
                tokens.push_back(shape);
                tokens.push_back("=");
                aLine = aLine.substr(aLine.find("=") + 1, aLine.length());
                tokens.push_back(aLine.substr(0, aLine.find("l")));
                aLine = aLine.substr(aLine.find("l"), aLine.length());
                tokens.push_back(aLine.substr(0, aLine.find("=")));
                tokens.push_back("=");
                aLine = aLine.substr(aLine.find("=") + 1, aLine.length());
                std::string conditionContent = aLine.substr(aLine.find("\""), aLine.find("]"));
                tokens.push_back(conditionContent);
                tokens.push_back("]");
                std::string semicolon(1, aLine[aLine.length() - 1]);
                tokens.push_back(semicolon);
                tokens.push_back("\n");
                conditionMap[conditionType] = conditionContent;
            }
            // look for if/else condition (usecase #3)
            if (aLine.find("->") != std::string::npos && aLine.find("conditionIfElse") != std::string::npos)
            {
                int arrowPos = aLine.find("->");
                std::string firstJob = aLine.substr(0, arrowPos);
                std::string secondJob = aLine.substr(arrowPos + 2, aLine.length() - (arrowPos + 3));
                std::string semicolon(1, aLine[aLine.length() - 1]);
                if (firstJob != "conditionIfElse")
                {
                    input["jobID"] = jobID;
                    int status = js->GetJobStatus(input);
                    std::string condition = conditionMap["conditionIfElse"];
                    condition.erase(std::remove(aLine.begin(), aLine.end(), '\"'), aLine.end());
                    int enteredStatus = std::stoi(condition.substr(condition.find("==") + 2, condition.length()));
                    if (status == enteredStatus)
                    {
                        right = true;
                    }
                    else
                    {
                        right = false;
                        jobFolders[3] = jobFolders[4];
                    }
                    tokens.push_back(firstJob);
                    tokens.push_back("->");
                    jobs.insert(firstJob);
                }
                if (firstJob == "conditionIfElse" && right)
                {
                    tokens.push_back(secondJob);
                    tokens.push_back(semicolon);
                    tokens.push_back("\n");
                    jobs.insert(secondJob);
                    right = false;
                    jobID++;
                    continue;
                }
                right = true;
            }
            // look for while condition (usecase #2)
            if (aLine.find("->") != std::string::npos && aLine.find("conditionAgain") != std::string::npos)
            {
                int arrowPos = aLine.find("->");
                std::string firstJob = aLine.substr(0, arrowPos);
                std::string secondJob = aLine.substr(arrowPos + 2, aLine.length() - (arrowPos + 3));
                std::string semicolon(1, aLine[aLine.length() - 1]);
                if (firstJob != "conditionAgain")
                {
                    input["jobID"] = jobID;
                    int status = js->GetJobStatus(input);
                    std::string condition = conditionMap["conditionAgain"];
                    condition.erase(std::remove(aLine.begin(), aLine.end(), '\"'), aLine.end());
                    int enteredStatus = std::stoi(condition.substr(condition.find("==") + 2, condition.length()));
                    while (!right)
                    {
                        if (status == enteredStatus)
                        {
                            right = false;
                            std::cout << "Forever loop, change while loop condition" << std::endl;
                            return;
                        }
                        else
                        {
                            right = true;
                        }
                    }
                    tokens.push_back(firstJob);
                    tokens.push_back("->");
                    jobs.insert(firstJob);
                    jobID++;
                }
            }
            // look for no condition (usecase #1)
            if (aLine.find("->") != std::string::npos && aLine.find("conditionIfElse") == std::string::npos && aLine.find("conditionAgain") == std::string::npos)
            {
                int arrowPos = aLine.find("->");
                std::string firstJob = aLine.substr(0, arrowPos);
                std::string secondJob = aLine.substr(arrowPos + 2, aLine.length() - (arrowPos + 3));
                std::string semicolon(1, aLine[aLine.length() - 1]);
                jobs.insert(firstJob);
                jobs.insert(secondJob);
                tokens.push_back(firstJob);
                tokens.push_back("->");
                tokens.push_back(secondJob);
                tokens.push_back(semicolon);
                tokens.push_back("\n");
                jobID++;
            }
        }
    }
    FSFile.close();
    // Syntactic Parsing, check through the tokens vector to see if the tokens follows Flowscript rules
    for (int i = 0; i < tokens.size(); i++)
    {
        // Set up rules using if statements
        if (tokens[i] == "\n" && tokens[i - 1] != ";")
        { // rule 1: semicolon needed in end of line
            std::cout << "Missing ; at end of line." << std::endl;
            dependencyMap.clear();
            break;
        }
        if (tokens[i] == "->" && !defaultJobs.count(tokens[i - 1]))
        { // rule 2 : a job need to be at the left of an arrow
            std::cout << "Undefined job: " << tokens[i - 1] << " on the left of -> " << std::endl;
            dependencyMap.clear();
            break;
        }
        if (tokens[i] == "->" && !defaultJobs.count(tokens[i + 1]))
        { // rule 3 : a job need to be at the right of an arrow
            std::cout << "Undefined job: " << tokens[i + 1] << " on the right of -> " << std::endl;
            dependencyMap.clear();
            break;
        }
        // store dependencies in dependencyMap
        if (i > 0 && jobs.count(tokens[i]) && tokens[i - 1] == "->")
        {
            dependencyMap[tokens[i]].insert(tokens[i - 2]);
        }
    }
}

void Interpreter::Run()
{
    // if dependencyMap is empty at the start, there has to be an error on FlowScript file, so end the function
    if (dependencyMap.empty())
        return;
    int jobID = 0;
    // first create/execute jobs with no dependencies
    for (auto job : jobs)
    {
        if (dependencyMap.find(job) == dependencyMap.end())
        {
            input["newJobType"] = job;
            input["folder"] = jobFolders[jobID++];
            js->CreateNewJob(input);
            std::this_thread::sleep_for(std::chrono::seconds(3)); // make the main thread wait for the job is processed
            //  erase the dependencies
            for (auto &pair : dependencyMap)
            {
                pair.second.erase(job);
            }
        }
    }
    // keep create/execute the jobs through dependencyMap, erase a job from both the map's key (jobs) and value (dependencies) after it is being executed
    while (!dependencyMap.empty())
    {
        auto it = dependencyMap.begin();
        while (it != dependencyMap.end())
        {
            if (it->second.empty())
            {
                input["newJobType"] = it->first;
                input["folder"] = jobFolders[jobID++];
                js->CreateNewJob(input);

                // Erase the job from value, then key
                for (auto &pair : dependencyMap)
                {
                    pair.second.erase(it->first);
                }
                it = dependencyMap.erase(it);
            }
            else
            {
                ++it;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    // finish jobs being executed
    js->FinishCompletedJobs();
}
