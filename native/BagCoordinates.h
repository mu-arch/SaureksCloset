#pragma once
#include <array>
#include <cmath>
using BagMatrix=std::array<float,16>;
static BagMatrix bagMatrixProduct(const BagMatrix& a,const BagMatrix& b){
    BagMatrix out{};
    for(unsigned col=0;col<4;++col)for(unsigned row=0;row<4;++row)for(unsigned k=0;k<4;++k)
        out[col*4+row]+=a[k*4+row]*b[col*4+k];
    return out;
}
static bool bagAffineInverse(const BagMatrix& m,BagMatrix& out){
    for(float v:m)if(!std::isfinite(v))return false;
    if(std::fabs(m[3])>.00001f||std::fabs(m[7])>.00001f||std::fabs(m[11])>.00001f||std::fabs(m[15]-1)>.001f)return false;
    const float a=m[0],b=m[4],c=m[8],d=m[1],e=m[5],f=m[9],g=m[2],h=m[6],i=m[10];
    const float det=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    if(!std::isfinite(det)||std::fabs(det)<.000001f)return false;
    out={{(e*i-f*h)/det,(f*g-d*i)/det,(d*h-e*g)/det,0,
          (c*h-b*i)/det,(a*i-c*g)/det,(b*g-a*h)/det,0,
          (b*f-c*e)/det,(c*d-a*f)/det,(a*e-b*d)/det,0,0,0,0,1}};
    for(unsigned row=0;row<3;++row)out[12+row]=-out[row]*m[12]-out[4+row]*m[13]-out[8+row]*m[14];
    for(float v:out)if(!std::isfinite(v))return false;
    return true;
}
static bool bagWorldUpInModel(const BagMatrix& renderToModel,const BagMatrix& worldToRender,std::array<float,3>& up,
                              std::array<float,3>* verticalMeasure=nullptr){
    // Transform a direction, never a position: scene origin and camera travel
    // must not excite the spring. The scene's third column is world +Z in the
    // render basis. Inverse model/view also handles mounted/tilted characters.
    for(const auto* m:{&renderToModel,&worldToRender}){
        for(float v:*m)if(!std::isfinite(v))return false;
        if(std::fabs((*m)[3])>.00001f||std::fabs((*m)[7])>.00001f||std::fabs((*m)[11])>.00001f||std::fabs((*m)[15]-1)>.001f)return false;
    }
    for(unsigned row=0;row<3;++row)
        up[row]=renderToModel[row]*worldToRender[8]+renderToModel[4+row]*worldToRender[9]+renderToModel[8+row]*worldToRender[10];
    const float length=std::sqrt(up[0]*up[0]+up[1]*up[1]+up[2]*up[2]);
    if(!std::isfinite(length)||length<.000001f)return false;
    for(float& value:up)value/=length;
    if(verticalMeasure){
        // A direction and a height measurement differ under nonuniform actor
        // scale. Use the inverse-transpose relationship so world-horizontal
        // animation cannot excite bounce on a tilted, stretched character.
        BagMatrix modelToWorld;
        if(!bagAffineInverse(bagMatrixProduct(renderToModel,worldToRender),modelToWorld))return false;
        for(unsigned axis=0;axis<3;++axis)(*verticalMeasure)[axis]=modelToWorld[axis*4+2]*length;
        for(float v:*verticalMeasure)if(!std::isfinite(v))return false;
    }
    return true;
}
