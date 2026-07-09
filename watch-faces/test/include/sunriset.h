#pragma once

int __sunriset__(int year, int month, int day, double lon, double lat,
                 double altit, int upper_limb, double *rise, double *set);

#define sun_rise_set(year, month, day, lon, lat, rise, set) \
    __sunriset__(year, month, day, lon, lat, -35.0 / 60.0, 1, rise, set)
