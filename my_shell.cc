#include <iostream>
#include <sstream>
#include <dirent.h>
#include <vector>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
using namespace std;

// this function will get all the PATH directories in a vector<string>
vector<string> get_PATH_Dirs ()
{
    vector<string> pathDirs;
    string curDir = "";
    unsigned i = 0;
    char* pathVar = getenv("PATH");
    
    while (i != strlen(pathVar))
    {
        if (pathVar[i] == ':')
        {
            pathDirs.push_back(curDir);
            curDir = "";
        }
        
        else 
            curDir +=pathVar[i];
            
        ++i;
    }
    
    pathDirs.push_back(curDir);
    return pathDirs;
}

// this function will search in each directory and find the specified command
// this function will return the path of the specified command
string findPath (string nameOfComm)
{
    vector<string> dirs = get_PATH_Dirs();
    
    const char* command = nameOfComm.c_str();
    for (unsigned i = 0; i < dirs.size(); ++i)
    {
        const char* currentDir = dirs.at(i).c_str();
        DIR* dirStream;
        dirent* directInfo;
        
        if ((dirStream = opendir(currentDir)))
        {
            while ((directInfo = readdir(dirStream))) 
            {
                string nameOfFile = directInfo->d_name;
                if (nameOfFile == command)
                {
                    string pathToReturn = currentDir; 
                    pathToReturn += "/";
                    pathToReturn += command;
                    return pathToReturn;
                }
            }
        }
    }
    cout << "Error! Command not found in your PATH directories" << endl;
    exit(1);
}
        
// this vector parses a whole commandline and seperates each individual command
// into its own slot in a vector of strings
vector<string> parseWholeCommandLine(const string & commandLine)
{
    vector<string> allCommands;
    string currToken , command;
    
    istringstream parse;
    parse.str(commandLine);
    
    while (parse >> currToken)
        command = command + " " + currToken;
    
    
    if (command != "")
        allCommands.push_back(command);
    
    return allCommands;
}

// this function parses an individual command and fills ann array of char*
// that will contain the name and arguments of the command. That array will 
// be put into exec(). The function also returns a vector<string> that will 
// contain each element in the command in its own slot.
vector <string> parseCommand(string commandLine , char*  commands[])
{
    vector<string> operators;
    string currCommand;
    unsigned i;
    int tracker = 0;
    istringstream parse;
    parse.str(commandLine);
    
    while (parse >> currCommand)
    {
        if ((currCommand != "<") && (currCommand != ">"))
        {
            char* arr = new char[currCommand.size()];
            for (i = 0; i < currCommand.size(); ++i)
                arr[i] = currCommand.at(i);
            
            arr[i] = '\0';    
        
            commands[tracker] = arr;
        }
        
        operators.push_back(currCommand);
           
        ++tracker;
    }
    
    commands[tracker] = NULL;
    
    return operators;
}

// this function will actually use dup2 to set file descriptors of files
// this function returns the file descriptor of the file opened 
int redirect_IO ( vector<string> command , int index , string action,
                   int redirVal)
{
    int fileDesc;
    
    if (action == "read")
        fileDesc = open(command.at(index).c_str() , O_RDONLY);
        
    else
        fileDesc = open(command.at(index).c_str() , O_WRONLY);
        
    if (fileDesc < 0)
    {
        cerr << endl << "Error: " << strerror(errno) 
             << " Cannot open file" << endl;
        exit(1);
    }
                        
    if (dup2(fileDesc , redirVal) == -1)
    {
        cerr << endl << "Error: " << strerror(errno) << endl;
        exit(1);
    }
    
    return fileDesc;
}
                
// this function will go through a command and see if there are any > or <
// symbols. If found , this function will call the   
void set_IO_FileDesc(int & fileDesIn, int & fileDesOut, bool inFound , 
                     bool outFound, vector<string> currCommand)
{
    for (unsigned j = 0; j < currCommand.size(); ++j)
    {
        if (currCommand.at(j) == "<")
        {
            if (inFound)
            {
                cerr << "Error! Only one < or > command is allowed!" << endl;
                exit(1);
            }
                    
            fileDesIn = redirect_IO(currCommand , j+1 , "read" , 0);
            inFound = true;
        }
                    
        else if (currCommand.at(j) == ">")
        {
            if (outFound)
            {
                cerr << "Only one < or > command is allowed!" << endl;
                exit(1);
            }
                            
            fileDesOut = redirect_IO(currCommand , j+1 , "write" , 1);
            outFound = true;
        }
    }
}

// this function will execute a single command
void execSingleCommand ( const string & command)
{
    char*  args[1000];
    bool inFound = false;
    bool outFound = false;
    int fileDesIn = 0;
    int fileDesOut = 0;
    
    vector<string> currCommand = parseCommand(command , args);
    
    set_IO_FileDesc(fileDesIn , fileDesOut , inFound , outFound , currCommand);
    
    string path = findPath(currCommand.at(0));
    const char* commandPath = path.c_str();
    
    if (execv(commandPath , args) == -1)
    {
        cerr << "Cannot execute the command: " << strerror(errno)
             << endl;
        exit(1);
    }
    
    if (signal(SIGINT , handleControlC) == SIG_ERR)
    {   
        cout << "Error, could not receive signal! Exiting!" << endl;
        exit(1);
    }
                
    if (inFound)
        close(fileDesIn);
            
    if (outFound)
        close(fileDesOut);
}

// this function is the main function that will run the whole commandline
void runCommandline (const string & commandLine)
{
    vector<string> allComm = parseWholeCommandLine(commandLine);
    
    pid_t process_ID = fork();
        
    if (process_ID == -1)
    {
        cerr << endl << "Error: " << strerror(errno) << " Cannot "
             << "execute command" << endl;
        return;
    }
    
    else if (process_ID == 0)
        execSingleCommand(allComm.at(0));
    
    else
        wait(NULL);
}

// this function gets a commandline from the user
string getCommandLine()
{
    string commandLine;
    cout << endl << "% ";
    getline(cin , commandLine);
    return commandLine;
}
    
int main()
{
    while (true)
    {
        string commandLine = getCommandLine();
        runCommandline(commandLine);
    }
    
    return 0;
}
    
    
    
    
