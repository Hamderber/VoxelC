#pragma once

#define PI_D 3.1415926535897931
#define PI_F 3.1415927F

/// @brief Converts radians to degrees
/// @param rad
/// @return float
static inline float cm_rad2degf(float rad)
{
    const float rad2deg = 180.0F / PI_F;
    return rad * rad2deg;
}

/// @brief Converts radians to degrees
/// @param rad
/// @return double
static inline double cm_rad2degd(double rad)
{
    const double rad2deg = 180.0 / PI_D;
    return rad * rad2deg;
}

/// @brief Converts degrees to radians
/// @param deg
/// @return float
static inline float cm_deg2radf(float deg)
{
    const float deg2rad = PI_F / 180.0F;
    return deg * deg2rad;
}

/// @brief Converts degrees to radians
/// @param deg
/// @return double
static inline double cm_deg2radd(double deg)
{
    const double deg2rad = PI_D / 180.0;
    return deg * deg2rad;
}
