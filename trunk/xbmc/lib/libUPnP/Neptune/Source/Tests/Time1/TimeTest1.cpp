/*****************************************************************
|
|      Time Test Program 1
|
|      (c) 2005-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include "Neptune.h"
#include "NptResults.h"

/*----------------------------------------------------------------------
|       macros
+---------------------------------------------------------------------*/
#define SHOULD_SUCCEED(r)                                        \
    do {                                                         \
        if (NPT_FAILED(r)) {                                     \
            fprintf(stderr, "FAILED: line %d\n", __LINE__);      \
            NPT_ASSERT(0);                                       \
        }                                                        \
    } while(0)                                         

#define SHOULD_FAIL(r)                                           \
    do {                                                         \
        if (NPT_SUCCEEDED(r)) {                                  \
            fprintf(stderr, "should have failed line %d (%d)\n", \
                __LINE__, r);                                    \
            NPT_ASSERT(0);                                       \
        }                                                        \
    } while(0)                                  

#define SHOULD_EQUAL_I(a, b)                                     \
    do {                                                         \
        if ((a) != (b)) {                                        \
            fprintf(stderr, "got %d, expected %d line %d\n",     \
                a, b, __LINE__);                                 \
            NPT_ASSERT(0);                                       \
        }                                                        \
    } while(0)                                  

#define SHOULD_EQUAL_F(a, b)                                     \
    do {                                                         \
        if ((a) != (b)) {                                        \
            fprintf(stderr, "got %f, expected %f line %d\n",     \
                (float)a, (float)b, __LINE__);                   \
            NPT_ASSERT(0);                                       \
        }                                                        \
    } while(0)                                  

#define SHOULD_EQUAL_S(a, b)                                     \
    do {                                                         \
        if (!NPT_StringsEqual(a,b)) {                            \
            fprintf(stderr, "got %s, expected %s line %d\n",     \
                a, b, __LINE__);                                 \
            NPT_ASSERT(0);                                       \
        }                                                        \
    } while(0)     

/*----------------------------------------------------------------------
|   TestSuiteGetTime
+---------------------------------------------------------------------*/
static void
TestSuiteGetTime()
{
    NPT_TimeStamp now;
    NPT_Date      today;

    /* get utc time */
    SHOULD_SUCCEED(NPT_System::GetCurrentTimeStamp(now));

    /* convert utc time to date */
    SHOULD_SUCCEED(NPT_Time::GetGMTDateFromTimeStamp(now, today));

    /* verify utc time has not change */
    SHOULD_EQUAL_I(today.timezone, 0);
}

/*----------------------------------------------------------------------
|   FillRandomDate
+--------------------------------------------------------------------*/
static void
FillRandomDate(NPT_Date& date)
{
    date.local.year         = NPT_System::GetRandomInteger();
    date.local.month        = NPT_System::GetRandomInteger();
    date.local.day          = NPT_System::GetRandomInteger();
    date.local.hours        = NPT_System::GetRandomInteger();
    date.local.minutes      = NPT_System::GetRandomInteger();
    date.local.seconds      = NPT_System::GetRandomInteger();
    date.local.milliseconds = NPT_System::GetRandomInteger();
    date.timezone           = NPT_System::GetRandomInteger();
}

/*----------------------------------------------------------------------
|   TestSuiteDateFromTimeString
+---------------------------------------------------------------------*/
static void
TestSuiteDateFromTimeString()
{
    NPT_Date date;

    /* Valid date */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2006-04-14T12:01:10.003Z", date));
    SHOULD_EQUAL_I(date.local.year         , 2006);
    SHOULD_EQUAL_I(date.local.month        , 4);
    SHOULD_EQUAL_I(date.local.day          , 14);
    SHOULD_EQUAL_I(date.local.hours        , 12);
    SHOULD_EQUAL_I(date.local.minutes      , 1);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 3);
    SHOULD_EQUAL_I(date.timezone           , 0);

    /* Valid date, 2 characters milliseconds */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2006-04-14T12:01:10.02Z", date));
    SHOULD_EQUAL_I(date.local.year         , 2006);
    SHOULD_EQUAL_I(date.local.month        , 4);
    SHOULD_EQUAL_I(date.local.day          , 14);
    SHOULD_EQUAL_I(date.local.hours        , 12);
    SHOULD_EQUAL_I(date.local.minutes      , 1);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 20);
    SHOULD_EQUAL_I(date.timezone           , 0);

    /* Valid date, 1 character milliseconds */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2006-04-14T12:01:10.9Z", date));
    SHOULD_EQUAL_I(date.local.year         , 2006);
    SHOULD_EQUAL_I(date.local.month        , 4);
    SHOULD_EQUAL_I(date.local.day          , 14);
    SHOULD_EQUAL_I(date.local.hours        , 12);
    SHOULD_EQUAL_I(date.local.minutes      , 1);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 900);
    SHOULD_EQUAL_I(date.timezone           , 0);

    /* Valid date, no 'Z' */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2006-04-14T12:01:10.003", date));
    SHOULD_EQUAL_I(date.local.year         , 2006);
    SHOULD_EQUAL_I(date.local.month        , 4);
    SHOULD_EQUAL_I(date.local.day          , 14);
    SHOULD_EQUAL_I(date.local.hours        , 12);
    SHOULD_EQUAL_I(date.local.minutes      , 1);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 3);
    SHOULD_EQUAL_I(date.timezone           , 0);

    /* Valid date, Z, but no milliseconds */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2006-04-14T12:01:10Z", date));
    SHOULD_EQUAL_I(date.local.year         , 2006);
    SHOULD_EQUAL_I(date.local.month        , 4);
    SHOULD_EQUAL_I(date.local.day          , 14);
    SHOULD_EQUAL_I(date.local.hours        , 12);
    SHOULD_EQUAL_I(date.local.minutes      , 1);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 0);
    SHOULD_EQUAL_I(date.timezone           , 0);

    /* Valid date with microseconds, no 'Z' */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2005-09-06T17:16:10.003498", date));
    SHOULD_EQUAL_I(date.local.year         , 2005);
    SHOULD_EQUAL_I(date.local.month        , 9);
    SHOULD_EQUAL_I(date.local.day          , 6);
    SHOULD_EQUAL_I(date.local.hours        , 17);
    SHOULD_EQUAL_I(date.local.minutes      , 16);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 3);
    SHOULD_EQUAL_I(date.timezone           , 0);

    /* Valid date with microseconds, 'Z' */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2005-09-06T17:16:10.003498Z", date));
    SHOULD_EQUAL_I(date.local.year         , 2005);
    SHOULD_EQUAL_I(date.local.month        , 9);
    SHOULD_EQUAL_I(date.local.day          , 6);
    SHOULD_EQUAL_I(date.local.hours        , 17);
    SHOULD_EQUAL_I(date.local.minutes      , 16);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 3);
    SHOULD_EQUAL_I(date.timezone           , 0);

    /* Valid date, no milliseconds, with timezone offset */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2006-04-14T12:01:10+03:00", date));
    SHOULD_EQUAL_I(date.local.year         , 2006);
    SHOULD_EQUAL_I(date.local.month        , 4);
    SHOULD_EQUAL_I(date.local.day          , 14);
    SHOULD_EQUAL_I(date.local.hours        , 12);
    SHOULD_EQUAL_I(date.local.minutes      , 1);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 0);
    SHOULD_EQUAL_I(date.timezone           , 180);

    /* Valid date, no milliseconds, with negative timezone offset */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2006-04-14T12:01:10-05:00", date));
    SHOULD_EQUAL_I(date.local.year         , 2006);
    SHOULD_EQUAL_I(date.local.month        , 4);
    SHOULD_EQUAL_I(date.local.day          , 14);
    SHOULD_EQUAL_I(date.local.hours        , 12);
    SHOULD_EQUAL_I(date.local.minutes      , 1);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 0);
    SHOULD_EQUAL_I(date.timezone           , -300);

    /* Valid date, with milliseconds, with positive timezone offset */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2006-04-14T12:01:10.200+03:00", date));
    SHOULD_EQUAL_I(date.local.year         , 2006);
    SHOULD_EQUAL_I(date.local.month        , 4);
    SHOULD_EQUAL_I(date.local.day          , 14);
    SHOULD_EQUAL_I(date.local.hours        , 12);
    SHOULD_EQUAL_I(date.local.minutes      , 1);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 200);
    SHOULD_EQUAL_I(date.timezone           , 180);

    /* Valid date, with milliseconds, with negative timezone offset */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2006-04-14T12:01:10.030-05:00", date));
    SHOULD_EQUAL_I(date.local.year         , 2006);
    SHOULD_EQUAL_I(date.local.month        , 4);
    SHOULD_EQUAL_I(date.local.day          , 14);
    SHOULD_EQUAL_I(date.local.hours        , 12);
    SHOULD_EQUAL_I(date.local.minutes      , 1);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 30);
    SHOULD_EQUAL_I(date.timezone           , -300);

    /* Valid date with microseconds and negative timezone offset */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2005-09-06T17:16:10.001822-05:00", date));
    SHOULD_EQUAL_I(date.local.year         , 2005);
    SHOULD_EQUAL_I(date.local.month        , 9);
    SHOULD_EQUAL_I(date.local.day          , 6);
    SHOULD_EQUAL_I(date.local.hours        , 17);
    SHOULD_EQUAL_I(date.local.minutes      , 16);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 1);
    SHOULD_EQUAL_I(date.timezone           , -300);
    
    /* Valid date with microseconds and positive timezone offset */
    FillRandomDate(date);
    SHOULD_SUCCEED(NPT_Time::GetDateFromString("2005-09-06T17:16:10.001822+05:00", date));
    SHOULD_EQUAL_I(date.local.year         , 2005);
    SHOULD_EQUAL_I(date.local.month        , 9);
    SHOULD_EQUAL_I(date.local.day          , 6);
    SHOULD_EQUAL_I(date.local.hours        , 17);
    SHOULD_EQUAL_I(date.local.minutes      , 16);
    SHOULD_EQUAL_I(date.local.seconds      , 10);
    SHOULD_EQUAL_I(date.local.milliseconds , 1);
    SHOULD_EQUAL_I(date.timezone           , 300);

    /* Invalid date with 3 digit year */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("206-04-14T12:01:10.003Z", date));

    /* Invalid date with 5 digit year */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("20076-04-14T12:01:10.003Z", date));

    /* Invalid date with 5 digit year */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("20076-04-14T12:01:10.003Z", date));

    /* Invalid date with garbage in the end */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2006-04-14T12:01:10.003+69:696", date));

    /* Invalid date with bad month */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2006-010-14T12:01:10.003", date));

    /* Invalid date with bad month, right overall length */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-0--14T12:01:10.003", date));

    /* Invalid date with bad year-month separator */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063Y08-14T12:01:10.003", date));

    /* Invalid date with bad time separator */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14t12:01:10.003", date));

    /* Invalid date with bad hour */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T012:01:10.003", date));

    /* Invalid date with bad GMT indicator */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10.003z", date));

    /* Invalid date with bad GMT indicator */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10.003g", date));

    /* Invalid date with millisecond separator but no digits */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10.", date));

    /* Invalid date with millisecond separator but no digits */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10.Z", date));

    /* Invalid date with millisecond separator but no digits */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10.+10:38", date));

    /* Invalid date with bad timezone offset */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10+10:338", date));

    /* Invalid date with bad timezone offset */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10+001:38", date));

    /* Invalid date with bad timezone offset */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10+10:33Z", date));

    /* Invalid date with bad timezone offset */
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10.08+10:33Z", date));

    /* Invalid date with bad timezone offset with seconds*/
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10.08+10:33:30", date));

    /* Invalid date with timezone offset too big*/
    FillRandomDate(date);
    SHOULD_FAIL(NPT_Time::GetDateFromString("2063-08-14T12:01:10.08+14:33", date));
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int /*argc*/, char** /*argv*/)
{
    TestSuiteGetTime();
    TestSuiteDateFromTimeString();
    return 0;
}
