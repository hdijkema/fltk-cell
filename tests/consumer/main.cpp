#include <fltk_cell.h>
int main()
{
    return fltk_cell::formatNumber("9007199254740993.125") == "9,007,199,254,740,993.13" ? 0 : 1;
}
