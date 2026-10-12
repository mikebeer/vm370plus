// IOSTEST CPP -- iostreams on VM/370plus CMS (9.21)
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <numeric>

int main(int argc, char **argv)
{
    std::cout << "IOSTEST: iostreams on CMS, " << argc - 1 << " args"
              << std::endl;
    std::ostringstream o;
    o << std::setw(8) << 42 << '|' << std::left << std::setw(6) << "ab" << '|'
      << std::hex << 255 << std::dec << '|' << std::fixed
      << std::setprecision(3) << 3.14159;
    std::cout << "  [" << o.str() << "]" << std::endl;
    std::istringstream in("10 20 30 40 x");
    std::vector<int> v;
    int k;
    while (in >> k) v.push_back(k);
    std::cout << "  read " << v.size() << " numbers, sum "
              << std::accumulate(v.begin(), v.end(), 0) << std::endl;
    std::cout << "  to_string " << std::to_string(123456789) + "!" << ", stod "
              << std::stod("2.5e3") << std::endl;
    std::cerr << "  (cerr works)" << std::endl;
    for (int i = 1; i < argc; i++)
        std::cout << "  arg " << i << ": " << argv[i] << '\n';
    return 0;
}
