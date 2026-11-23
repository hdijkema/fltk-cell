#include "fltk_cell.h"
#include <iostream>
#include <stdexcept>
using namespace fltk_cell;
void expect(const std::string& actual, const std::string& expected)
{
    if (actual != expected) throw std::runtime_error("Expected [" + expected + "], got [" + actual + "]");
}
int main()
{
    try {
        NumberFormat f;
        f.decimal_separator = ',';
        f.group_separator = '.';
        expect(formatNumber("1234.5678", f), "1.234,57");
        expect(formatNumber("9007199254740993.125", f), "9.007.199.254.740.993,13");
        expect(formatNumber("-999.995", f), "-1.000,00");
        expect(formatNumber("-0.001", f), "0,00");
        expect(formatNumber("1e-3", f), "0,00");
        expect(formatNumber("9.999e3", f), "9.999,00");
        expect(formatNumber("invalid", f), "invalid");
        expect(formatNumber("1e99999", f), "1e99999");
        f.percent = true;
        expect(formatNumber("0.125", f), "12,50%");
        f.percent = false;
        f.decimals = -1;
        expect(formatNumber("0.000123400", f), "0,000123400");
        expect(formatHex(Bytes{0, 31, 163, 255}, 2), "00 1F ...");
        expect(formatHex(Bytes{0, 31, 163, 255}, 10, false, false), "001fa3ff");
        TableCellNumberDrawer number;
        expect(number.displayText(Decimal{"12345678901234567890.125"}), "12,345,678,901,234,567,890.13");
        expect(number.displayText(std::monostate{}), "NULL");
        TableCellHexDrawer hex;
        hex.preview_bytes = 1;
        expect(hex.displayText(Bytes{0, 255}), "00 ...");
        Bytes borrowed{0,255};expect(hex.displayText(BytesView{borrowed.data(),borrowed.size()}), "00 ...");
        std::cout << "Cell formatting contracts passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
