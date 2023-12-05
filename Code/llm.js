import { OpenAI } from "langchain/llms/openai";
import * as dotenv from "dotenv";
import { readFileSync } from 'fs';
dotenv.config();

const llm = new OpenAI({
    temperature: 0.7,
    model: "gpt-4",
});

// Get command line arguments excluding the first two (which are 'node' and the script filename)
const args = process.argv.slice(2);
const prompt = args[0];
const errorJsonFile = args[1];
let newPrompt = prompt;

// Check if errorJsonFile is provided
if (errorJsonFile) {
    // Read the file content if provided
    const fileContent = readFileSync(errorJsonFile, 'utf8');
    const exFile = readFileSync('Data/example.json', 'utf8');
    newPrompt += "\n" + fileContent + exFile;
}

// Call the OpenAI API with the extracted values
const res = await llm.call(newPrompt);

console.log(res);
