#include <fstream>
#include <iostream>
#include <string>
#include <regex>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

using std::wstring;
using std::vector;

// trim from start (in place)
static inline void ltrim(wstring &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(wstring &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(wstring &s) {
    ltrim(s);
    rtrim(s);
}

// split
vector<wstring> split(wstring s, wstring delim) {
    std::wregex re(delim);
    return vector<wstring> {
        std::wsregex_token_iterator(s.begin(), s.end(), re, -1),
        std::wsregex_token_iterator()
    };
}

//==========================


struct Attrib {
    wstring name;
    wstring value;
    int defaultTimes;
    wstring values[20];
};

bool isSectionStart(wstring line) {
    return line[0] == L'[';
}

std::pair<wstring, wstring> LoadAttrib(wstring& line) {
    size_t equal = line.find('=');
    return std::make_pair(line.substr(0, equal), line.substr(equal + 1, line.size()));
}

void outputByBitset(vector<std::wofstream>& fout, wstring str, int times){
    for (int i = 0; i < 20; i++) {
        if (times & (1 << i)) {
            fout[i] << str << std::endl;
        }
    }
}

void outputAttrib(std::wofstream& output, wstring name, wstring value) {
    for (auto s : split(value, L"\\n")) 
        output << name << L"=" << s << std::endl;
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

    std::wifstream fin ("TimesAll.ini");

    vector<std::wofstream> fout;
    for (int i = 0; i < times; i++) {
        sprintf(filename, "Times%d.ini", i + 1);
        std::wofstream fx (filename, std::ofstream::out | std::ofstream::trunc);
        fout.push_back(std::move(fx));
    }

    // There are two types of sections: 
    //  the specified section (applies to a given times)
    //  the default section (applies to every times)
    //  If there is a default section, there should not be any specified sections.
    // In the default section, there are two types of attribs:
    //  the specified attrib (applies to a given times)
    //  the default attrib (applies to every times apart from the specified times)

    wstring line;
    int sectionTimes;

    std::wregex re (L"^\\((\\d+(?:,\\d+)*\\))(.*)$");
    std::wsmatch m;

    vector<Attrib> attribs;
    
    while(getline(fin, line)) {

        int rowTimes = 0;

        if (std::regex_search(line, m, re)) {
            auto splittedTimes = split(m[1], L",");
            for (wstring & s : splittedTimes) {
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
            wstring name = pair.first;
            wstring value = pair.second;

            // Default Section
            if (sectionTimes == (1 << times) - 1) {

                // default attrib
                if (rowTimes == 0) {
                    // Same as Last Row
                    if (attribs.size() > 0 && attribs[attribs.size() - 1].name == name) {
                        attribs[attribs.size() - 1].value += L"\n";
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
                                    if (a.values[i] != L"")
                                        a.values[i] += L"\n";
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