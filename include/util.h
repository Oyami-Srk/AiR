//
// Created by Shiroko on 2021/4/13.
//

#ifndef AIR_UTIL_H
#define AIR_UTIL_H

#define DEBUG 0

#include <WString.h>

void   feed_dog();
int    evaluate_air(unsigned long co2, float temp, float humd);
String urlDecode(String input);

#endif // AIR_UTIL_H
