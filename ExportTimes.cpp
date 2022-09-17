#include <fstream>
#include <iostream>
#include <string>
#include <regex>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

using std::vector;

typedef std::string     string;
typedef std::ifstream   ifstream;
typedef std::ofstream   ofstream;
typedef std::regex      regex;
typedef std::smatch     smatch;
typedef std::sregex_token_iterator 
                        sregex_token_iterator;

// trim from start (in place)
static inline void ltrim(string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(string &s) {
    ltrim(s);
    rtrim(s);
}

// split
vector<string> split(string s, string delim) {
    regex re(delim);
    return vector<string> {
        sregex_token_iterator(s.begin(), s.end(), re, -1),
        sregex_token_iterator()
    };
}

//==========================


struct Attrib {
    string name;
    string value;
    int defaultTimes;
    string values[20];
};

bool isSectionStart(string line) {
    return line[0] == '[';
}

std::pair<string, string> LoadAttrib(string& line) {
    size_t equal = line.find('=');
    return std::make_pair(line.substr(0, equal), line.substr(equal + 1, line.size()));
}

void outputByBitset(vector<ofstream>& fout, string str, int times){
    for (int i = 0; i < 20; i++) {
        if (times & (1 << i)) {
            fout[i] << str << std::endl;
        }
    }
}

void outputAttrib(ofstream& output, string name, string value) {
    for (auto s : split(value, "\\n")) 
        output << name << "=" << s << std::endl;
}

int main () {

    int times = 5;
    char filename[20];
    
    // if (argc > 1) {
    //     if (strcmp("-n", argv[1]) == 0) 
    //         sscanf(argv[2], "%d", &times);
    //     else {
    //         printf("Usage: ExportTimes -n times");
    //         return 1;
    //     }
    // }
    
    std::cout << "Enter Total Number of Times (5 by default):" << std::endl ;

    std::cin >> times;

    // ==========================

    ifstream fin ("TimesAll.ini");

    vector<ofstream> fout;
    for (int i = 0; i < times; i++) {
        sprintf(filename, "Times%d.ini", i + 1);
        ofstream fx (filename, std::ofstream::out | std::ofstream::trunc);
        fout.push_back(std::move(fx));
    }

    // There are two types of sections: 
    //  the specified section (applies to a given times)
    //  the default section (applies to every times)
    //  If there is a default section, there should not be any specified sections.
    // In the default section, there are two types of attribs:
    //  the specified attrib (applies to a given times)
    //  the default attrib (applies to every times apart from the specified times)

    string line;
    int sectionTimes;

    regex re ("^\\((\\d+(?:,\\d+)*\\))(.*)$");
    smatch m;

    vector<Attrib> attribs;
    
    while(getline(fin, line)) {

        int rowTimes = 0;

        if (std::regex_search(line, m, re)) {
            auto splittedTimes = split(m[1], ",");
            for (string & s : splittedTimes) {
                trim(s);
                rowTimes |= (1 << (std::stoi(s) - 1));
            }
            line = m[2];
        }

        // Section Start
        if (isSectionStart(line)) {

            // Flush the entire section
            for (Attrib & a : attribs) {
                for (int i = 0; i < times; i++) {
                    if (a.defaultTimes & (1 << i)) {
                        outputAttrib(fout[i], a.name, a.value);
                    } else {
                        outputAttrib(fout[i], a.name, a.values[i]);
                    }
                }
            }
            
            attribs.clear();

            // Move to this section
            // Default Sections apply to every times
            if (rowTimes == 0)
                sectionTimes = (1 << times) - 1;
            // Otherwise it is specified
            else 
                sectionTimes = rowTimes;

            // Immediately output the section head
            outputByBitset(fout, line, sectionTimes);

        // It is an attrib
        } else {
            auto pair = LoadAttrib(line);
            string name = pair.first;
            string value = pair.second;

            // Default Section
            if (sectionTimes == (1 << times) - 1) {

                // default attrib
                if (rowTimes == 0) {
                    // Same as Last Row
                    if (attribs.size() > 0 && attribs[attribs.size() - 1].name == name) {
                        attribs[attribs.size() - 1].value += "\n";
                        attribs[attribs.size() - 1].value += value;
                    } else {
                        Attrib neuer;
                        neuer.name = name;
                        neuer.value = value;
                        neuer.defaultTimes = (1 << times) - 1;
                        attribs.push_back(neuer);
                    }

                // Specified Attrib : find the corresponding one
                } else {
                    for (Attrib & a : attribs) {
                        if (a.name == name) {
                            
                            // Remove from default
                            a.defaultTimes &= (~rowTimes);

                            // Append Value
                            for (int i = 0; i < times; i++) {
                                if (rowTimes & (1 << i)) {
                                    if (a.values[i] != "")
                                        a.values[i] += "\n";
                                    a.values[i] += value;
                                }
                            }
                        }
                    }
                }
                
            // Specified Session: output immediately
            } else {
                outputByBitset(fout, line, sectionTimes);
            }
            
        }

    }

    // flush the last section
    for (Attrib & a : attribs) {
        for (int i = 0; i < times; i++) {
            if (a.defaultTimes & (1 << i)) {
                outputAttrib(fout[i], a.name, a.value);
            } else {
                outputAttrib(fout[i], a.name, a.values[i]);
            }
        }
    }

    for (int i = 0; i < times; i++) {
        fout[i].close();
    }
    
    system("pause");

    return 0;
}