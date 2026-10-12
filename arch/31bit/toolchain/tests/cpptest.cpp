// CPPTEST CPP -- C++ on VM/370plus CMS (9.21): classes, templates, STL
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

struct Shape {
    virtual ~Shape() {}
    virtual double area() const = 0;
    virtual const char *name() const = 0;
};
struct Rect : Shape {
    double w, h;
    Rect(double a, double b) : w(a), h(b) {}
    double area() const override { return w * h; }
    const char *name() const override { return "rect"; }
};
struct Circle : Shape {
    double r;
    explicit Circle(double a) : r(a) {}
    double area() const override { return 3.14159265358979 * r * r; }
    const char *name() const override { return "circle"; }
};

template <typename T> T sum(const std::vector<T> &v)
{
    T s{};
    for (const auto &x : v) s += x;
    return s;
}

static int counter;
struct Init { Init() { counter = 42; } } init;     // a static constructor

int main(int argc, char **argv)
{
    std::printf("CPPTEST: C++%ld on CMS, %d args, static ctor %d\n",
                (long)__cplusplus / 100 % 100, argc - 1, counter);
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Rect>(3, 4));
    shapes.push_back(std::make_unique<Circle>(1));
    for (auto &s : shapes) std::printf("  %-6s %8.4f\n", s->name(), s->area());

    std::vector<int> v;
    for (int i = 1; i <= 100; i++) v.push_back(i);
    std::printf("  sum 1..100 = %d\n", sum(v));

    std::vector<std::string> w = {"vm370", "plus", "cms", "gcc", "thirteen"};
    std::sort(w.begin(), w.end());
    std::string all;
    for (auto &s : w) all += s + " ";
    std::printf("  sorted: %s\n", all.c_str());

    std::map<std::string, int> m;
    const char *text = "the quick brown fox jumps over the lazy dog the end";
    std::string t(text), cur;
    for (char c : t + " ") {
        if (c == ' ') { if (!cur.empty()) m[cur]++; cur.clear(); }
        else cur += c;
    }
    std::printf("  words %zu, 'the' x%d\n", m.size(), m["the"]);
    for (int i = 1; i < argc; i++) std::printf("  arg %d: %s\n", i, argv[i]);
    return 0;
}
