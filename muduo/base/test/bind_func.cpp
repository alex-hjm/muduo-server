#include <iostream>
#include <functional>

using namespace std;
using namespace std::placeholders;

class Foo
{
  public:
  void memberFunc(double d, int i, int j) {
    cout << d << endl;
    cout << i << endl;     
    cout << j << endl;
  }
};

int main() 
{
  Foo foo;
  function<void(int,int)> fp=bind(&Foo::memberFunc,&foo,0.5,_1,_2);
  fp(100,200);

  function<void(int,int)> fp1=bind(&Foo::memberFunc,std::ref(foo),0.5,_1,_2);
  fp1(55,66);

  return 0;
}