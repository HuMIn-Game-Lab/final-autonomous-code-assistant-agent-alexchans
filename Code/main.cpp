#include "Agent.h"

int main()
{
    Agent agent("FlowScript is a Domain Specific Language based on DOT language, where each node represents a single job, and job dependencies are denoted by the '->' arrow. Consider the following example FlowScript program: digraph G { jobA -> jobB; jobB -> jobC; }. In this instance, jobB depends on jobA, and jobC depends on jobB. Now, create a FlowScript program with three jobs: compileJob1, llmJob, and compileJob2. Ensure that llmJob has a dependency on compileJob1, and compileJob2 has a dependency on llmJob.");
    agent.Run();
    return 0;
}