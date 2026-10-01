#include "../native/CapeWorldCollision.h"
#include <cassert>
#include <iostream>
int main(){
    using namespace cape;
    assert(validWorldBounds({-1500,-200,20},{-1496,-197,24}));
    assert(!validWorldBounds({0,0,0},{100,1,1}));
    assert(!validWorldBounds({1,0,0},{0,1,1}));
    assert(!validWorldBounds({NAN,0,0},{1,1,1}));
    // Deliberately non-planar set: a sloping ground face and a vertical wall.
    // Conversion must preserve actual triangles and their world coordinates.
    WorldFace faces[2]{};
    faces[0].vertices[0]={-100,30,3};faces[0].vertices[1]={-98,30,4};faces[0].vertices[2]={-100,32,3};
    faces[1].vertices[0]={-99,30,0};faces[1].vertices[1]={-99,30,4};faces[1].vertices[2]={-99,34,0};
    std::vector<ColliderTriangle> out;
    assert(appendWorldFaces(faces,2,out));assert(out.size()==2);
    for(unsigned i=0;i<2;++i)for(unsigned j=0;j<3;++j){
        assert(length(out[i].current[j]-faces[i].vertices[j])==0);
        assert(length(out[i].previous[j]-faces[i].vertices[j])==0);
    }
    assert(!appendWorldFaces(faces,2,out,3));assert(out.size()==2);
    faces[1].vertices[2].z=NAN;
    assert(!appendWorldFaces(faces,2,out));assert(out.size()==2); // transactional failure
    assert(!appendWorldFaces(nullptr,1,out));
    faces[0].vertices[1]=faces[0].vertices[2]=faces[0].vertices[0];
    assert(appendWorldFaces(faces,1,out));assert(out.size()==2);
    std::cout<<"PASS: cape world geometry layout, walls, slopes, bounds and failure rollback\n";
}
