#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>

using std::cin;
using std::endl;
using std::vector;

typedef std::string     string;
typedef std::ifstream   ifstream;
typedef std::ofstream   ofstream;
typedef std::regex      regex;
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


struct Attrib {
    string name;
    string value;
    string values[20];
    int valuesMapping = 0;
};

struct Section {
    string title = "";
    string id = "";
    vector<Attrib> lines;
    int existingTimes;

    Section() : existingTimes(0){};
};

vector<Section> sections;

string line;

bool readline(ifstream& input) {
    if (std::getline(input, line)) {
        for (int i = 0; i < line.size(); i++) {
            if (line[i] == ';'){
                line[i] = '\0';
                break;
            }
        }
        return true;
    }
    return false;
}

string LoadSectionHead() {
    size_t begin = line.find('[') + 1;
    size_t end = line.find(']');
    return line.substr(begin, end - begin);
}

std::pair<string, string> LoadAttrib() {
    size_t equal = line.find('=');
    return std::make_pair(line.substr(0, equal), line.substr(equal + 1, line.size()));
}

vector<string> split(string s, string delim) {
    regex re(delim);
    return vector<string> {
        sregex_token_iterator(s.begin(), s.end(), re, -1),
        sregex_token_iterator()
    };
}

bool CheckAttrib(string left, string right) {
    auto lsplit = split(left, "(\\s+|,)");
    auto rsplit = split(right, "(\\s+|,)");
    if (lsplit.size() && lsplit.size() != rsplit.size())
        return false;
    for (int i = 0; i < lsplit.size(); i++) {
        if (lsplit[i] != rsplit[i])
            return false;
    }
    return true;
}

void endOfSection(Section*& section, int times) {
    // Just add in to first one
    if (times == 1) {
        section->existingTimes |= 1;
        sections.push_back(*section);

    // Compare with existing ones
    } else {
        bool sectionFound = false;
        for (Section & rsection : sections) {
            if (rsection.title == section->title && rsection.id == section->id) {
                // Section Found
                // O(n^2) to search for each line 
                for (Attrib & row : section->lines) {

                    // Loop thru existing records; if found different, attach to it
                    for (Attrib & rrow : rsection.lines) {
                        if (rrow.name == row.name) {
                            if (!CheckAttrib(row.value, rrow.value)) {
                                rrow.values[times] = row.value;
                                rrow.valuesMapping |= (1 << (times - 1));
                            }

                            break;
                        }
                    }
                }

                // One Section found, no loop required
                sectionFound = true;
                rsection.existingTimes |= (1 << (times - 1));
                break;
            }
        }
        
        if (!sectionFound) {
            section->existingTimes = (1 << (times - 1));
            sections.push_back(*section);
        }
    }
    
    delete section;
    section = new Section;
}

void LoadTimes(ifstream& input, int times) {
    Section* section = new Section;
    bool firstSection = true;
    while(readline(input)) {
        
        //std::cout << "linein:\t" << string(line) << std::endl;

        // New Section
        if (line[0] == '[') {
            if (!firstSection) {
                endOfSection(section, times);
            } else {
                firstSection = false;
            }

            section->title = LoadSectionHead();
            for (auto & c: section->title) c = toupper(c);

        // Add to existing section
        } else {
            auto pair = LoadAttrib();
            string name = pair.first;
            string value = pair.second;
            //std::cout << "parsed:\t" << name << ", " << value << std::endl;

            for (auto & c: name) c = toupper(c);

            if (section->title == "GENERAL" && name == "SEQUENCE") 
                section->id = value;
            
            bool linefound = false;
            for (Attrib & rline : section->lines) {
                if (rline.name == name) {
                    rline.value += "\n";
                    rline.value += value;
                    linefound = true;
                }
            }

            if (!linefound) {
                Attrib row;
                row.name = name;
                row.value = value;
                section->lines.push_back(row);
            }
        }
    }
    
    endOfSection(section, times);
    delete section;
}

void outputRows(ofstream& fout, int times, string name, string value) {
    trim(name);
    trim(value);
    if (name == "") return;
    
    auto linesplit = split(value, "[\r\n]");
    for (int i = 0; i < linesplit.size(); i++) {
        if (times == 0)
            fout << name << "=" << linesplit[i] << std::endl;
        else
            fout << "(" << times << ")" << name << "=" << linesplit[i] << std::endl;
    }
}


int getSectionOrder(const Section& l) {
    if (l.title == "SYSTEM") return 0;
    if (l.title.substr(0, 4) == "KING") return 1;
    if (l.title.substr(0, 4) == "CITY") return 2;
    if (l.title.substr(0, 4) == "PATH") return 3;
    return 4;
};

bool compareSection(const Section& l, const Section& r) {
    int lorder = getSectionOrder(l), rorder = getSectionOrder(r);
    if (lorder == rorder) {
        if (l.title == r.title)
            return l.id < r.id;
        return l.title < r.title;
    }
    return lorder < rorder;
};

int main () {
    int times = 5;
    
    // if (argc > 1) {
    //     if (strcmp("-n", argv[1]) == 0) 
    //         sscanf(argv[2], "%d", &times);
    //     else {
    //         printf("Usage: LoadTimes -n times");
    //         return 1;
    //     }
    // }

    std::cout << "Enter Total Number of Times (5 by default):" << std::endl ;

    std::cin >> times;

    // Load 
    for (int i = 1; i <= times; i++) {
        char filename[20];
        sprintf(filename, "Times%d.ini", i);
        ifstream fin(filename);
        LoadTimes(fin, i);
        fin.close();
    }

    std::sort(sections.begin(), sections.end(), compareSection);

    ofstream fout ("TimesAll.ini");
    for (Section & section : sections) {
        if (section.existingTimes != (1 << times) - 1) {
            for (int i = 1; i <= times; i++) {
                if (section.existingTimes & (1 << (i - 1))) {
                    fout << "(" << i << ")[" << section.title << "]" << std::endl;
                    for (Attrib & row : section.lines) {
                        if (!(row.valuesMapping & (1 << (i - 1)))) {
                            outputRows(fout, 0, row.name, row.value);
                        } else {
                            outputRows(fout, 0, row.name, row.values[i]);
                        }
                    }
                }
            }

        } else {
            fout << "[" <<section.title << "]" << std::endl;
            for (Attrib & row : section.lines) {
                outputRows(fout, 0, row.name, row.value);
                for (int i = 1; i <= times; i++) {
                    if (row.valuesMapping & (1 << (i - 1)))
                        outputRows(fout, i, row.name, row.values[i]);
                }
            }
        }
    }
    fout.close();

    system("pause");

    return 0;
}