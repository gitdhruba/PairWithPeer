#include <string>
/*
    function --> getip()
    return: the ip as a string
*/
std::string getip(){
    FILE *f = popen("ip a | grep 'scope global' | grep -v ':' | awk '{print $2}' | cut -d '/' -f1", "r");
    char c; std::string ip = "";
    while ((c = getc(f)) != EOF) ip += c;
       
    pclose(f);
    return ip;
}