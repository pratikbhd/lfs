#include <ctime>

class CostBenefit {
    public:
        unsigned int segmentNumber;
        std::time_t newest;
        double utilization;
        double age;
        double score;
};