#include <string>

class Person {
    private:
        std::string lastname;
        std::string firstname;

    public:
        std::string GetName();
        Person(std::string firstname, std::string lastname);
        Person() = default;
};