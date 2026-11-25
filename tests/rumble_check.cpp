#include "../Source/KickEngine.h"
#include <iostream>
static std::vector<float> r(float bs,float ru){ KickParams k; k.punch=.6f;k.body=.6f;k.length=.55f;k.drive=.65f;k.rumble=ru;k.bodySplit=bs; return KickEngine::renderOneShot(k,nullptr,44100,0.375);}
int main(){ for(float bs:{0.5f,0.65f,0.75f,0.9f}){ auto a=r(bs,.75f), b=r(bs,0); double e1=0,e2=0; int m=(int)(0.12*44100), n=(int)(0.24*44100), q=(int)a.size();
 for(int i=0;i<m;i++){double d=a[i]-b[i]; e1+=d*d;} for(int i=n;i<q;i++){double d=a[i]-b[i]; e2+=d*d;}
 std::cout<<"bodySplit "<<bs<<": Rumble-Energie 0-120ms="<<std::sqrt(e1/m)<<"  240ms-Ende="<<std::sqrt(e2/(q-n))<<"\n"; } }
