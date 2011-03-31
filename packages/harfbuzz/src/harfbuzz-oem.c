#include <harfbuzz-external.h>

// VP: I have no idea where to get this info from, but it looks like it's
// enough for arabic glyph substitution....

HB_LineBreakClass HB_GetLineBreakClass(HB_UChar32 ch)
{
    return (HB_LineBreakClass)0;
}

void HB_GetUnicodeCharProperties(HB_UChar32 ch, HB_CharCategory *category, int *combiningClass)
{
    *category       = HB_Letter_Other;
    *combiningClass = 0;
}

HB_CharCategory HB_GetUnicodeCharCategory(HB_UChar32 ch)
{
    return (HB_CharCategory)HB_Letter_Other;
}

int HB_GetUnicodeCharCombiningClass(HB_UChar32 ch)
{
    return 0;
}

HB_UChar16 HB_GetMirroredChar(HB_UChar16 ch)
{
    return ch;
}
