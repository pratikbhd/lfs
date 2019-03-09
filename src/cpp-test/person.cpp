#include "Person.h"

std::string Person::GetName(){
    return lastname + " " + firstname;
}

Person::Person(std::string firstname, std::string lastname):firstname(firstname), lastname(lastname){

}