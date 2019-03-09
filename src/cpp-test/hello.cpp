#include <iostream>
#include <string>
#include "Person.h"

int main(){
    std::cout << "type your first name" << std::endl;
    std::string firstname = "";
    std::cin >> firstname;
    std::cout << "type your last name" << std::endl;
    std::string lastname = "";
    std::cin >> lastname;
    Person p1(firstname, lastname);
    std::cout << p1.GetName();
    std::cin >> lastname;

    return 0;
}