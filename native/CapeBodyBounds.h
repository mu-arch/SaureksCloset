#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>
#include "CapeCloth.h"

namespace cape {
// Mesh-derived, closed collision volumes. Fitting is done only when the visible
// mesh changes. Posing transforms small per-bone boxes, never body triangles.
// Each complete triangle belongs to one convex volume, including joint blends:
// enclosing its three skinned vertices also encloses the triangle interior.
class BodyBounds {
    using Matrix=std::array<float,16>;
    struct Range {
        Vec3 low{1.e30f,1.e30f,1.e30f},high{-1.e30f,-1.e30f,-1.e30f};
        void add(Vec3 p){low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}
        Vec3 center() const{return (low+high)*.5f;}
        Vec3 half() const{return (high-low)*.5f;}
    };
    struct Influence {
        unsigned bone=0,vertices=0;
        Range points,weighted;
        float lowWeight=1,highWeight=0;
    };
    struct Group {unsigned bone=0;bool central=false;Vec3 reference{};std::vector<unsigned> vertices,faces;std::vector<Influence> influences;};
    std::vector<Group> groups_;
    std::vector<ColliderBox> previous_;
    bool ready_=false;
    static Vec3 transform(const Matrix& m,Vec3 p){return {m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};}
    static Vec3 direction(const Matrix& m,Vec3 p){return {m[0]*p.x+m[4]*p.y+m[8]*p.z,m[1]*p.x+m[5]*p.y+m[9]*p.z,m[2]*p.x+m[6]*p.y+m[10]*p.z};}
    static Matrix compose(const Matrix& a,const Matrix& b){Matrix out{};for(unsigned c=0;c<4;++c)for(unsigned r=0;r<4;++r)for(unsigned k=0;k<4;++k)out[c*4+r]+=a[k*4+r]*b[c*4+k];return out;}
    static float axis(Vec3 p,unsigned i){return i==0?p.x:(i==1?p.y:p.z);}
    static void setAxis(Vec3& p,unsigned i,float value){if(i==0)p.x=value;else if(i==1)p.y=value;else p.z=value;}
    static bool matrixFinite(const Matrix& matrix){for(float value:matrix)if(!std::isfinite(value))return false;return true;}
    static void unique(std::vector<unsigned>& vertices){std::sort(vertices.begin(),vertices.end());vertices.erase(std::unique(vertices.begin(),vertices.end()),vertices.end());}
public:
    unsigned count() const{return static_cast<unsigned>(groups_.size());}
    unsigned influenceCount() const{unsigned n=0;for(const auto& group:groups_)n+=static_cast<unsigned>(group.influences.size());return n;}
    bool ready() const{return ready_;}
    bool central(unsigned index) const{return index<groups_.size()&&groups_[index].central;}
    void clearHistory(){previous_.clear();}
    // Vertex only needs position, weights[4], bones[4]. The caller filters the
    // triangle list to visible body/equipment sections and excludes the cape.
    template<class Vertex>
    bool fit(const std::vector<Vertex>& vertices,const std::vector<Triangle>& triangles,unsigned maxBounds=24){
        groups_.clear();previous_.clear();ready_=false;
        if(!maxBounds||maxBounds>32||vertices.size()>65536||triangles.size()>65536)return false;
        for(unsigned face=0;face<triangles.size();++face){
            const auto& triangle=triangles[face];const unsigned indices[]={triangle.a,triangle.b,triangle.c};std::array<unsigned,256> weights{};
            for(unsigned index:indices){
                if(index>=vertices.size()||!finite(vertices[index].position))return false;
                unsigned total=0;
                for(unsigned k=0;k<4;++k){const unsigned bone=vertices[index].bones[k],weight=vertices[index].weights[k];if(bone>=weights.size())return false;weights[bone]+=weight;total+=weight;}
                if(total!=255)return false;
            }
            unsigned dominant=0;for(unsigned i=1;i<weights.size();++i)if(weights[i]>weights[dominant])dominant=i;
            auto found=std::find_if(groups_.begin(),groups_.end(),[&](const Group& group){return group.bone==dominant;});
            if(found==groups_.end()){groups_.push_back({});found=groups_.end()-1;found->bone=dominant;}
            for(unsigned index:indices)found->vertices.push_back(index);
            found->faces.push_back(face);
        }
        for(auto& group:groups_){unique(group.vertices);Range bounds;for(unsigned index:group.vertices)bounds.add(vertices[index].position);group.reference=bounds.center();}
        // Very small finger/face groups share their nearest existing volume
        // when the model has more bones than the fixed collision budget.
        const unsigned boneBudget=std::max(1u,maxBounds/2);
        while(groups_.size()>boneBudget){
            unsigned small=0;for(unsigned i=1;i<groups_.size();++i)if(groups_[i].vertices.size()<groups_[small].vertices.size())small=i;
            unsigned nearest=small?0:1;float distance=std::numeric_limits<float>::max();
            for(unsigned i=0;i<groups_.size();++i)if(i!=small){const Vec3 d=groups_[i].reference-groups_[small].reference;const float dd=dot(d,d);if(dd<distance){nearest=i;distance=dd;}}
            auto& target=groups_[nearest];target.faces.insert(target.faces.end(),groups_[small].faces.begin(),groups_[small].faces.end());target.vertices.insert(target.vertices.end(),groups_[small].vertices.begin(),groups_[small].vertices.end());unique(target.vertices);
            Range bounds;for(unsigned index:target.vertices)bounds.add(vertices[index].position);target.reference=bounds.center();groups_.erase(groups_.begin()+small);
        }
        // Reserve half the budget for shape, not just skeleton partitions.
        // Neck/torso taper and concave shoulders otherwise become one broad
        // box that needlessly pushes the cape far off the visible skin.
        const auto faceRange=[&](unsigned face){Range r;const auto& t=triangles[face];for(unsigned v:{t.a,t.b,t.c})r.add(vertices[v].position);return r;};
        const auto cost=[](const Range& r){const Vec3 d=r.high-r.low;return d.x*d.y*d.z+.001f*(d.x*d.y+d.y*d.z+d.z*d.x);};
        while(groups_.size()<maxBounds){
            float bestGain=1.e-8f;unsigned bestGroup=0,bestSplit=0;std::vector<unsigned> bestOrder;
            for(unsigned index=0;index<groups_.size();++index){const auto& group=groups_[index];if(group.faces.size()<4)continue;
                Range full;for(unsigned vertex:group.vertices)full.add(vertices[vertex].position);const float fullCost=cost(full);
                for(unsigned dimension=0;dimension<3;++dimension){
                    auto order=group.faces;std::sort(order.begin(),order.end(),[&](unsigned a,unsigned b){return axis(faceRange(a).center(),dimension)<axis(faceRange(b).center(),dimension);});
                    std::vector<Range> suffix(order.size());Range high;
                    for(unsigned i=static_cast<unsigned>(order.size());i>0;--i){const Range r=faceRange(order[i-1]);high.add(r.low);high.add(r.high);suffix[i-1]=high;}
                    Range low;
                    for(unsigned split=1;split<order.size();++split){const Range r=faceRange(order[split-1]);low.add(r.low);low.add(r.high);
                        const float gain=fullCost-cost(low)-cost(suffix[split]);
                        if(gain>bestGain){bestGain=gain;bestGroup=index;bestSplit=split;bestOrder=order;}
                    }
                }
            }
            if(bestOrder.empty())break;
            Group other;other.bone=groups_[bestGroup].bone;
            other.faces.assign(bestOrder.begin()+bestSplit,bestOrder.end());groups_[bestGroup].faces.assign(bestOrder.begin(),bestOrder.begin()+bestSplit);
            groups_.push_back(std::move(other));
            for(unsigned index:{bestGroup,static_cast<unsigned>(groups_.size()-1)}){auto& group=groups_[index];group.vertices.clear();Range r;
                for(unsigned face:group.faces){const auto& t=triangles[face];for(unsigned v:{t.a,t.b,t.c})group.vertices.push_back(v);}unique(group.vertices);
                for(unsigned vertex:group.vertices)r.add(vertices[vertex].position);
                group.reference=r.center();
            }
        }
        for(auto& group:groups_){
            Range restBounds;for(unsigned index:group.vertices)restBounds.add(vertices[index].position);
            // Only centerline-spanning body pieces need a preferred back side.
            // Arms and side strips should resolve onto their nearest safe face.
            group.central=restBounds.low.y<=.001f&&restBounds.high.y>=-.001f;
            for(unsigned index:group.vertices){const auto& vertex=vertices[index];std::array<bool,256> seen{};
                for(unsigned k=0;k<4;++k){if(!vertex.weights[k])continue;const unsigned bone=vertex.bones[k];
                    // Duplicate influence entries are unusual but valid. Fold
                    // them so the weight interval still describes the vertex.
                    if(seen[bone])continue;
                    seen[bone]=true;unsigned weight=0;
                    for(unsigned j=k;j<4;++j)if(vertex.bones[j]==bone)weight+=vertex.weights[j];
                    auto found=std::find_if(group.influences.begin(),group.influences.end(),[&](const Influence& influence){return influence.bone==bone;});
                    if(found==group.influences.end()){group.influences.push_back({});found=group.influences.end()-1;found->bone=bone;}
                    const float w=weight/255.f;found->points.add(vertex.position);found->weighted.add((vertex.position-group.reference)*w);
                    found->lowWeight=std::min(found->lowWeight,w);found->highWeight=std::max(found->highWeight,w);++found->vertices;
                }
            }
            for(auto& influence:group.influences)if(influence.vertices<group.vertices.size()){influence.weighted.add({});influence.lowWeight=0;}
            // Source vertex lists are no longer needed by animation updates.
            std::vector<unsigned>().swap(group.vertices);std::vector<unsigned>().swap(group.faces);
        }
        if(influenceCount()>512){groups_.clear();return false;}
        ready_=true;return true;
    }
    bool pose(const std::vector<Matrix>& bones,const Matrix& renderToWorld,std::vector<ColliderBox>& output,bool resetHistory=false){
        output.clear();if(!ready_||!matrixFinite(renderToWorld))return false;
        std::vector<ColliderBox> current;current.reserve(groups_.size());
        for(const auto& group:groups_){
            if(group.bone>=bones.size()||!matrixFinite(bones[group.bone]))return false;
            const Matrix mount=compose(renderToWorld,bones[group.bone]);
            ColliderBox box;const Vec3 origin=transform(mount,group.reference);
            box.currentAxes[0]=normalized(direction(mount,{1,0,0}),{1,0,0});
            const Vec3 y=direction(mount,{0,1,0});
            box.currentAxes[1]=normalized(y-box.currentAxes[0]*dot(y,box.currentAxes[0]),{0,1,0});
            box.currentAxes[2]=normalized(cross(box.currentAxes[0],box.currentAxes[1]));
            box.currentAxes[1]=normalized(cross(box.currentAxes[2],box.currentAxes[0]),{0,1,0});
            if(std::fabs(dot(box.currentAxes[0],box.currentAxes[1]))>.001f)return false;
            Range envelope;Vec3 weightedLow{},weightedHigh{};
            for(const auto& influence:group.influences){
                if(influence.bone>=bones.size()||!matrixFinite(bones[influence.bone]))return false;
                const Matrix matrix=compose(renderToWorld,bones[influence.bone]);
                const Vec3 center=transform(matrix,influence.points.center())-origin,half=influence.points.half();
                const Vec3 weightedCenter=direction(matrix,influence.weighted.center()),weightedHalf=influence.weighted.half();
                const Vec3 reference=transform(matrix,group.reference)-origin;
                Vec3 lo{},hi{};
                for(unsigned k=0;k<3;++k){const Vec3 normal=box.currentAxes[k];
                    const Vec3 coefficients{dot(normal,{matrix[0],matrix[1],matrix[2]}),dot(normal,{matrix[4],matrix[5],matrix[6]}),dot(normal,{matrix[8],matrix[9],matrix[10]})};
                    const auto radius=[&](Vec3 h){return std::fabs(coefficients.x)*h.x+std::fabs(coefficients.y)*h.y+std::fabs(coefficients.z)*h.z;};
                    const float c=dot(center,normal),r=radius(half);setAxis(lo,k,c-r);setAxis(hi,k,c+r);
                    const float offset=dot(reference,normal),a=offset*influence.lowWeight,b=offset*influence.highWeight;
                    const float wc=dot(weightedCenter,normal),wr=radius(weightedHalf);
                    setAxis(weightedLow,k,axis(weightedLow,k)+wc-wr+std::min(a,b));setAxis(weightedHigh,k,axis(weightedHigh,k)+wc+wr+std::max(a,b));
                }
                envelope.add(lo);envelope.add(hi);
            }
            // Both ranges independently enclose every true weighted vertex.
            // Their intersection retains that guarantee while reducing joint
            // over-inflation from either interval approximation alone.
            for(unsigned k=0;k<3;++k){setAxis(envelope.low,k,std::max(axis(envelope.low,k),axis(weightedLow,k)));setAxis(envelope.high,k,std::min(axis(envelope.high,k),axis(weightedHigh,k)));}
            const Vec3 center=envelope.center();box.currentCenter=origin;
            for(unsigned k=0;k<3;++k)box.currentCenter+=box.currentAxes[k]*axis(center,k);
            box.currentHalf=envelope.half()+Vec3{.003f,.003f,.003f};
            if(!finite(box.currentCenter)||!finite(box.currentHalf)||box.currentHalf.x<0||box.currentHalf.y<0||box.currentHalf.z<0)return false;
            const unsigned index=static_cast<unsigned>(current.size());
            if(!resetHistory&&previous_.size()==groups_.size()){box.previousCenter=previous_[index].currentCenter;box.previousHalf=previous_[index].currentHalf;for(unsigned k=0;k<3;++k)box.previousAxes[k]=previous_[index].currentAxes[k];}
            else{box.previousCenter=box.currentCenter;box.previousHalf=box.currentHalf;for(unsigned k=0;k<3;++k)box.previousAxes[k]=box.currentAxes[k];}
            current.push_back(box);
        }
        previous_=current;output=std::move(current);return true;
    }
};
}
