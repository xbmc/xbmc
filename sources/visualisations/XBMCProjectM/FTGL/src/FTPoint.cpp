#include "FTPoint.h"


bool operator == ( const FTPoint &a, const FTPoint &b) 
{
    return((a.values[0] == b.values[0]) && (a.values[1] == b.values[1]) && (a.values[2] == b.values[2]));
}

bool operator != ( const FTPoint &a, const FTPoint &b) 
{
    return((a.values[0] != b.values[0]) || (a.values[1] != b.values[1]) || (a.values[2] != b.values[2]));
}


FTPoint operator*( double multiplier, FTPoint& point)
{
    return point * multiplier;
}
        
