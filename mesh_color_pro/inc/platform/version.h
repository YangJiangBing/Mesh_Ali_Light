#define VERSION_MAJOR            1
#define VERSION_MINOR            0
#define VERSION_REVISION         3
#define VERSION_BUILDNUM         71
#define VERSION_GCID             0x6ad54d90
#define CUSTOMER_NAME            sdk
#define CN_1                     's'
#define CN_2                     'd'
#define CN_3                     'k'
#define CN_4                     '#'
#define CN_5                     '#'
#define CN_6                     '#'
#define CN_7                     '#'
#define CN_8                     '#'
#define BUILDING_TIME            "Tue Jan 29 10:21:03 2019"
#define NAME2STR(a)              #a
#define CUSTOMER_NAME_S          #NAME2STR(CUSTOMER_NAME)
#define NUM4STR(a,b,c,d)         #a "." #b "." #c "." #d
#define VERSIONBUILDSTR(a,b,c,d) NUM4STR(a,b,c,d)
#define VERSION_BUILD_STR        VERSIONBUILDSTR(VERSION_MAJOR,VERSION_MINOR,VERSION_REVISION,VERSION_BUILDNUM)
