#include <iostream>
#include <string>
#include <windows.h>

using namespace std;

int main(int argc, char * argv[]) {
    string currentPath = argv[0];
    currentPath = currentPath.substr(0, currentPath.find_last_of('\\'));
    string command = "sc create \"Database app server service\" binPath= " + currentPath + "\\Server.exe";
    cout << command << endl;
    system(command.c_str());
    command = "net start \"Database app server service\" /\"" + currentPath + "\\database\"";
    cout << command << endl;
    system(command.c_str());
    Sleep(10000);
    return 0;
}
