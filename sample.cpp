#include <typeinfo>
#include <iostream>

struct kalam
{
   const std::type_info& t;
   int v;
};


int main()
{
    kalam k{typeid(double)};

    if (k.t == typeid(4.0))
    {
        std::cout << "Hello!\n";
        std::cout << k.t.name() << "\n";
    }

    const auto& q = typeid(kalam);
    std::cout << q.hash_code() << "\n";

}