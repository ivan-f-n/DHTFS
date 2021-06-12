#ifndef LOGGER_H_
#define LOGGER_H_

using namespace std;

static string logfile =""; //Here we will save this session's log file path

inline std::string getCurrentDateTime(string s)
{
    time_t now = time(0); //Save current time
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    //Either calculate the timestamp now or of just the date
    if (s == "now")
        strftime(buf, sizeof(buf), "%Y-%m-%d-%T %X", &tstruct);
    else if (s == "date")
        strftime(buf, sizeof(buf), "%Y-%m-%d-%T", &tstruct);
    return std::string(buf);
}

inline void Logger(std::string logMsg)
{
    logfile = (logfile == "") ? "/home/ivan/log/log_" + getCurrentDateTime("date") + ".txt" : logfile; //Save this session's log file path
    std::string filePath = logfile;
    std::string now = getCurrentDateTime("now"); //Get current time from our function

    //Log the message next to the current timestamp in our file
    std::ofstream ofs(filePath.c_str(), std::ios_base::out | std::ios_base::app);
    ofs << now << '\t' << logMsg << '\n';
    ofs.close();
}
#endif