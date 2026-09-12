#include "../native/Appearance.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 unsigned cases=0;
 for(const auto& model:raceModels){
  Appearance a;a.race=model.race;a.sex=model.sex;
  // Construct valid examples spanning every skin/face, hairstyle/color and facial option.
  for(const auto& o:bodyOptions)if(o.race==a.race&&o.sex==a.sex){
   if(o.kind==0){a.skin=o.a;a.face=o.b;}
   if(o.kind==1){a.hairStyle=o.a;a.hairColor=o.b;}
   if(o.kind==2)a.facial=o.a;
  }
  assert(a.valid());assert(nativeModel(model.display)==a.model());
  for(const auto& o:bodyOptions)if(o.race==a.race&&o.sex==a.sex){
   Appearance sample=a;
   if(o.kind==0){sample.skin=o.a;sample.face=o.b;}
   if(o.kind==1){sample.hairStyle=o.a;sample.hairColor=o.b;}
   if(o.kind==2)sample.facial=o.a;
   assert(sample.valid());
   std::array<std::uint32_t,91> input;
   for(unsigned j=0;j<input.size();++j)input[j]=0xabc00000+j;
   auto copy=input;sample.compose(copy);
   for(unsigned j=0;j<input.size();++j){
    switch(j){
     case 0:assert(copy[j]==sample.race);break;
     case 1:assert(copy[j]==sample.sex);break;
     case 2:assert(copy[j]==sample.hairColor);break;
     case 3:assert(copy[j]==sample.skin);break;
     case 5:assert(copy[j]==sample.face);break;
     case 6:assert(copy[j]==sample.facial);break;
     case 7:assert(copy[j]==sample.hairStyle);break;
     default:assert(copy[j]==input[j]);
    }
    assert(input[j]==0xabc00000+j);
   }
   ++cases;
  }
  for(unsigned key=0;key<7;++key){
   Appearance invalid=a;
   unsigned Appearance::* fields[]={&Appearance::race,&Appearance::sex,&Appearance::skin,&Appearance::face,&Appearance::hairStyle,&Appearance::hairColor,&Appearance::facial};
   invalid.*fields[key]=255;assert(!invalid.valid());
  }
  for(const auto& source:raceModels){
   const float ratio=model.scale/source.scale;
   assert(std::fabs(source.scale*ratio-model.scale)<.00001f);
   std::array<float,16> matrix;
   for(unsigned i=0;i<16;++i)matrix[i]=i+1;
   const auto original=matrix;scaleBasis(matrix,ratio);
   for(unsigned i=0;i<16;++i){
    const bool basis=i/4<3&&i%4<3;
    assert(matrix[i]==original[i]*(basis?ratio:1));
   }
  }
 }
 assert(!nativeModel(15435));assert(!nativeModel(0));
 std::printf("PASS: %u valid appearance inputs, invalid options, render-reference preservation, 256 visual scale conversions and translation preservation.\n",cases);
}
