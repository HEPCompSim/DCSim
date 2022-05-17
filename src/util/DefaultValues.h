

#ifndef S_DEFAULTVALUES_H
#define S_DEFAULTVALUES_H

/**
 * @brief Set of default values that can be used in the simulation.
 * ATTENTION: Comparing two float values is never a good idea, 
 * rather check for differences
 */
class DefaultValues {
public:
    static constexpr const int UndefinedInt = -9999;
    static constexpr const float UndefinedFloat = -9999.0f;
    static constexpr const double UndefinedDouble = -9999.0;
};


#endif //S_DEFAULTVALUES_H
